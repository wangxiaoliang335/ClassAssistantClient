/*
* Module:   GenerateTestUserSig
*
* Function: 用于获取 TIMLogin 接口所必须的 UserSig，腾讯云使用 UserSig 进行安全校验，保护您的 IM 功能不被盗用
*           现在从服务器获取 UserSig，而不是在客户端计算
*/
#include "GenerateTestUserSig.h"
#include "CommonInfo.h"
#include "TAHttpHandler.h"
#include "./zlib/zlib.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

#include <time.h>
#include <stdio.h>
#include <memory>
#include <assert.h>
#include <Windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

class GenerateTestUserSigImpl
{
public:
    GenerateTestUserSigImpl(uint32_t SDKAppID, const std::string& secretKey, uint32_t currTime, uint32_t expireTime);
    ~GenerateTestUserSigImpl();

    std::string genTestUserSig(const std::string& userId);
private:
    std::string genHMACSHA256(const std::string& userId);
    void base64Enc(char* dest, const void* src, uint32_t length);
    char bit6ToAscii(uint8_t a);
private:
    const uint32_t          m_SDKAppID;
    const uint32_t          m_currTime;
    const uint32_t          m_expireTime;
    const std::string       m_secretKey;
};

GenerateTestUserSigImpl::GenerateTestUserSigImpl(uint32_t SDKAppID, const std::string& secretKey, uint32_t currTime, uint32_t expireTime)
    : m_SDKAppID(SDKAppID)
    , m_currTime(currTime)
    , m_expireTime(expireTime)
    , m_secretKey(secretKey)
{
    // 参数检测，请先给 GenerateTestUserSig 类的 _SDKAppID 、_EXPIRETIME 和 _SECRETKEY 成员变量赋值，避免触发 DEBUG 版本的断言框。

    assert(m_SDKAppID > 0);
    assert(m_currTime > 0);
    assert(m_expireTime > 0);
    assert(!m_secretKey.empty());
}

GenerateTestUserSigImpl::~GenerateTestUserSigImpl()
{

}

std::string GenerateTestUserSigImpl::genTestUserSig(const std::string& userId)
{
    // 参数检测，请传入非空的 userId 参数。
    assert(!userId.empty());

    std::string sig = genHMACSHA256(userId);

    size_t dataBufferLen = userId.size() + 256; // userId长度 + 其他字段的长度（不超过256）
    std::unique_ptr<char[]> dataBuffer(new char[dataBufferLen]);

    // 为避免引入 json 库，直接拼接 json 形式的字符串。

    int count = ::sprintf_s(dataBuffer.get(), dataBufferLen,
        "{"
        "\"TLS.ver\":\"2.0\","
        "\"TLS.identifier\":\"%s\","
        "\"TLS.sdkappid\":%lu,"
        "\"TLS.expire\":%lu,"
        "\"TLS.time\":%lu,"
        "\"TLS.sig\":\"%s\""
        "}", userId.c_str(), m_SDKAppID, m_expireTime, m_currTime, sig.c_str());
    if (count == -1)
    {
        return "";
    }

    // json 数据压缩

    uLong upperBound = compressBound(count);
    std::unique_ptr<Bytef[]> zipDest(new Bytef[count]);
    uLongf zipDestLen = count;
    int ret = compress2(zipDest.get(), &zipDestLen, (const Bytef*)dataBuffer.get(), count, Z_BEST_SPEED);
    if (ret != Z_OK)
    {
        return "";
    }

    // base64 编码

    size_t base64Len = (zipDestLen / 3 + 1) * 4 + 1;
    std::unique_ptr<char[]> base64Buffer(new char[base64Len]);
    base64Enc(base64Buffer.get(), zipDest.get(), zipDestLen);

    std::string result(base64Buffer.get());
    for (auto it = result.begin(); it != result.end(); ++it)
    {
        switch (*it)
        {
        case '+':
            *it = '*';
            break;
        case '/':
            *it = '-';
            break;
        case '=':
            *it = '_';
            break;
        default:
            break;
        }
    }

    return result;
}

// 资源清理函数

static void closeAlgHandle(BCRYPT_ALG_HANDLE* p)
{
    if (*p)
    {
        BCryptCloseAlgorithmProvider(*p, 0);
        *p = NULL;
    }
}

static void closeHashHandle(BCRYPT_HASH_HANDLE* p)
{
    if (*p)
    {
        BCryptDestroyHash(*p);
        *p = NULL;
    }
}

std::string GenerateTestUserSigImpl::genHMACSHA256(const std::string& userId)
{
    // 拼接用于生成 HMACSHA256 的字符串。

    size_t dataBufferLen = userId.size() + 256;
    std::unique_ptr<char[]> dataBuffer(new char[dataBufferLen]);

    int count = ::sprintf_s(dataBuffer.get(), dataBufferLen,
        "TLS.identifier:%s\n"
        "TLS.sdkappid:%lu\n"
        "TLS.time:%lu\n"
        "TLS.expire:%lu\n", userId.c_str(), m_SDKAppID, m_currTime, m_expireTime);
    if (count == -1)
    {
        return "";
    }

    std::string data(dataBuffer.get(), count);

    // 打开加密算法的句柄。
    BCRYPT_ALG_HANDLE alg = NULL;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &alg,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (!BCRYPT_SUCCESS(status))
    {
        printf("[Error] BCryptOpenAlgorithmProvider failed: 0x%x\n", status);
        return "";
    }

    // 资源托管，自动释放
    std::unique_ptr<BCRYPT_ALG_HANDLE, decltype(closeAlgHandle)*> algAutoDel(&alg, closeAlgHandle);

    // 计算存放 hashBuffer 对象的 dataBuffer 大小。
    DWORD cbHashObject = 0;
    DWORD cbData = 0;
    status = BCryptGetProperty(
        alg,
        BCRYPT_OBJECT_LENGTH,
        (PBYTE)&cbHashObject,
        sizeof(DWORD),
        &cbData,
        0);
    if (!BCRYPT_SUCCESS(status))
    {
        printf("[Error] BCryptGetProperty failed: 0x%x\n", status);
        return "";
    }

    // 分配 hashBuffer 对象的内存
    std::unique_ptr<BYTE[]> hashObject(new BYTE[cbHashObject]);
    if (!hashObject.get())
    {
        printf("[Error] memory allocation failed\n");
        return "";
    }

    // 计算 hashBuffer 的长度。
    DWORD cbHash = 0;
    status = BCryptGetProperty(
        alg,
        BCRYPT_HASH_LENGTH,
        (PBYTE)&cbHash,
        sizeof(DWORD),
        &cbData,
        0);
    if (!BCRYPT_SUCCESS(status))
    {
        printf("[Error] BCryptGetProperty: 0x%x\n", status);
        return "";
    }

    // 分配 hashBuffer dataBuffer 的内存
    std::unique_ptr<BYTE[]> hashBuffer(new BYTE[cbHash]);
    if (!hashBuffer.get())
    {
        printf("[Error] memory allocation failed\n");
        return "";
    }

    // 创建 hashBuffer 对象。
    BCRYPT_HASH_HANDLE hash = NULL;
    status = BCryptCreateHash(
        alg,
        &hash,
        hashObject.get(),
        cbHashObject,
        (PBYTE)m_secretKey.c_str(),
        m_secretKey.size(),
        0);
    if (!BCRYPT_SUCCESS(status))
    {
        printf("[Error]BCryptCreateHash: 0x%x\n", status);
        return "";
    }

    // 资源托管，自动释放
    std::unique_ptr<BCRYPT_HASH_HANDLE, decltype(closeHashHandle)*> hashAutoDel(&hash, closeHashHandle);

    // 生成 hashBuffer 值。
    status = BCryptHashData(
        hash,
        (PBYTE)data.c_str(),
        data.size(),
        0);
    if (!BCRYPT_SUCCESS(status))
    {
        printf("[Error]BCryptHashData: 0x%x\n", status);
        return "";
    }

    // 关闭 hashBuffer 对象
    status = BCryptFinishHash(
        hash,
        hashBuffer.get(),
        cbHash,
        0);
    if (!BCRYPT_SUCCESS(status))
    {
        printf("[Error]BCryptFinishHash: 0x%x\n", status);
        return "";
    }

    // base64 编码
    size_t base64Len = (cbHash / 3 + 1) * 4 + 1;
    std::unique_ptr<char[]> base64Buffer(new char[base64Len + 1]);
    base64Enc(base64Buffer.get(), hashBuffer.get(), cbHash);

    return std::string(base64Buffer.get());
}

void GenerateTestUserSigImpl::base64Enc(char *dest, const void *src, uint32_t length)
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint8_t a[4] = { 0 };
    for (i = 0; i < length / 3; ++i)
    {
        a[0] = (((uint8_t*)src)[i * 3 + 0]) >> 2;
        a[1] = (((((uint8_t*)src)[i * 3 + 0]) << 4) | ((((uint8_t*)src)[i * 3 + 1]) >> 4)) & 0x3F;
        a[2] = (((((uint8_t*)src)[i * 3 + 1]) << 2) | ((((uint8_t*)src)[i * 3 + 2]) >> 6)) & 0x3F;
        a[3] = (((uint8_t*)src)[i * 3 + 2]) & 0x3F;
        for (j = 0; j < 4; ++j)
        {
            *dest++ = bit6ToAscii(a[j]);
        }
    }

    switch (length % 3)
    {
    case 0:
        break;
    case 1:
        a[0] = (((uint8_t*)src)[i * 3 + 0]) >> 2;
        a[1] = ((((uint8_t*)src)[i * 3 + 0]) << 4) & 0x3F;
        *dest++ = bit6ToAscii(a[0]);
        *dest++ = bit6ToAscii(a[1]);
        *dest++ = '=';
        *dest++ = '=';
        break;
    case 2:
        a[0] = (((uint8_t*)src)[i * 3 + 0]) >> 2;
        a[1] = (((((uint8_t*)src)[i * 3 + 0]) << 4) | ((((uint8_t*)src)[i * 3 + 1]) >> 4)) & 0x3F;
        a[2] = ((((uint8_t*)src)[i * 3 + 1]) << 2) & 0x3F;
        *dest++ = bit6ToAscii(a[0]);
        *dest++ = bit6ToAscii(a[1]);
        *dest++ = bit6ToAscii(a[2]);
        *dest++ = '=';
        break;
    default:
        assert(false);
        break;
    }

    // 尾部添加结束符
    *dest = '\0';
}

char GenerateTestUserSigImpl::bit6ToAscii(uint8_t a)
{
    a &= (uint8_t)0x3F;

    if (a <= 25)
    {
        return a + 'A';
    }

    if (a <= 51)
    {
        return a - 26 + 'a';
    }

    if (a <= 61)
    {
        return a - 52 + '0';
    }

    if (a == 62)
    {
        return '+';
    }

    return '/'; // a == 63
}

GenerateTestUserSig::GenerateTestUserSig()
    : m_httpHandler(nullptr), m_eventLoop(nullptr), m_requestCompleted(false), m_requestSuccess(false)
{
    m_httpHandler = new TAHttpHandler(this);
    if (m_httpHandler) {
        connect(m_httpHandler, &TAHttpHandler::success, this, &GenerateTestUserSig::onHttpSuccess);
        connect(m_httpHandler, &TAHttpHandler::failed, this, &GenerateTestUserSig::onHttpFailed);
    }
}

GenerateTestUserSig::~GenerateTestUserSig()
{
    if (m_eventLoop) {
        delete m_eventLoop;
        m_eventLoop = nullptr;
    }
}

GenerateTestUserSig& GenerateTestUserSig::instance()
{
    static GenerateTestUserSig aInstance;
    return aInstance;
}

uint32_t GenerateTestUserSig::getSDKAppID() const
{
    return SDKAPPID;
}

void GenerateTestUserSig::onHttpSuccess(const QString& response)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8());
    if (jsonDoc.isObject()) {
        QJsonObject obj = jsonDoc.object();
        if (obj["data"].isObject()) {
            QJsonObject dataObj = obj["data"].toObject();
            if (dataObj.contains("user_sig")) {
                m_receivedUserSig = dataObj["user_sig"].toString();
                m_requestSuccess = true;
            } else if (dataObj.contains("usersig")) {
                m_receivedUserSig = dataObj["usersig"].toString();
                m_requestSuccess = true;
            } else if (dataObj.contains("sig")) {
                m_receivedUserSig = dataObj["sig"].toString();
                m_requestSuccess = true;
            }
        } else if (obj.contains("user_sig")) {
            m_receivedUserSig = obj["user_sig"].toString();
            m_requestSuccess = true;
        } else if (obj.contains("usersig")) {
            m_receivedUserSig = obj["usersig"].toString();
            m_requestSuccess = true;
        } else if (obj.contains("sig")) {
            m_receivedUserSig = obj["sig"].toString();
            m_requestSuccess = true;
        }
    }
    m_requestCompleted = true;
    if (m_eventLoop) {
        m_eventLoop->quit();
    }
}

void GenerateTestUserSig::onHttpFailed(const QString& error)
{
    m_requestSuccess = false;
    m_requestCompleted = true;
    if (m_eventLoop) {
        m_eventLoop->quit();
    }
}

std::string GenerateTestUserSig::genTestUserSig(const std::string& userId)
{
    if (!m_httpHandler) {
        return "";
    }

    // 重置状态
    m_receivedUserSig.clear();
    m_requestCompleted = false;
    m_requestSuccess = false;

    // 创建事件循环用于同步等待
    QEventLoop loop;
    m_eventLoop = &loop;

    // 设置超时定时器（30秒）
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    connect(&timeoutTimer, &QTimer::timeout, [&]() {
        m_requestCompleted = true;
        m_requestSuccess = false;
        loop.quit();
    });
    timeoutTimer.start(30000);

    // 发送请求到服务器
    QMap<QString, QString> params;
    params["user_id"] = QString::fromStdString(userId);
    m_httpHandler->post(QString("http://47.100.126.194:5000/getUserSig"), params);

    // 等待响应
    loop.exec();

    // 清理
    m_eventLoop = nullptr;
    timeoutTimer.stop();

    if (m_requestSuccess && !m_receivedUserSig.isEmpty()) {
        return m_receivedUserSig.toStdString();
    }

    // 如果服务器请求失败，返回空字符串
    return "";
}

std::string GenerateTestUserSig::getAdminUserId() const
{
    // 从CommonInfo获取当前登录用户的teacher_unique_id作为管理员账号
    UserInfo userInfo = CommonInfo::GetData();
    if (!userInfo.classId.isEmpty()) {
        return userInfo.classId.toStdString();
    }
    return std::string();  // 如果未设置，返回空字符串
}