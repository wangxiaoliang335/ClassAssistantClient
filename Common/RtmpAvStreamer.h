#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QImage>
#include <QVideoFrame>
#include <QMutex>

#include <atomic>

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVPacket;
struct SwrContext;
struct SwsContext;

class QThread;

/**
 * 内部 worker：放在 header 里让 QtMoc 能生成 metaobject（不要在 .cpp 里写 Q_OBJECT）。
 */
class RtmpAvStreamerWorker final : public QObject
{
    Q_OBJECT
public:
    explicit RtmpAvStreamerWorker(QObject* parent = nullptr);
    ~RtmpAvStreamerWorker() override;

    void configure(const QString& host,
                   quint16 port,
                   const QString& streamKey,
                   int outW,
                   int outH,
                   int fps,
                   bool audioEnabled,
                   int sampleRate,
                   int channels);

signals:
    void started();
    void stopped();
    void logMessage(const QString& message);
    void errorOccurred(const QString& error);

public slots:
    void startInternal();
    void stopInternal();
    void enqueueVideo(const QImage& img);
    void enqueueAudio(const QByteArray& pcm);

private:
    QString buildSrtUrl() const;
    bool initOutputContext();
    bool initVideoEncoder();
    bool initAudioEncoder();
    bool initSwr();
    void ensureSws(const QImage& img);
    void encodeVideo(const QImage& imgIn);
    void encodeAudioFromBuffer();
    void cleanup();

private:
    QString m_host;
    quint16 m_port = 10080; // SRT 默认端口（按服务器实际配置调整）
    QString m_streamKey;

    int m_outWidth = 640;
    int m_outHeight = 480;
    int m_fps = 25;

    bool m_audioEnabled = true;
    int m_sampleRate = 44100;
    int m_channels = 1;

    bool m_running = false;
    int64_t m_audioPts = 0;
    int64_t m_videoPts = 0;
    QByteArray m_pcmBuffer;

    AVFormatContext* m_outputCtx = nullptr;
    AVCodecContext* m_vCodecCtx = nullptr;
    AVCodecContext* m_aCodecCtx = nullptr;
    AVStream* m_videoStream = nullptr;
    AVStream* m_audioStream = nullptr;
    AVFrame* m_vFrame = nullptr;
    AVFrame* m_aFrame = nullptr;
    AVPacket* m_packet = nullptr;
    SwrContext* m_swrCtx = nullptr;
    SwsContext* m_swsCtx = nullptr;
    int m_srcW = 0;
    int m_srcH = 0;
};

/**
 * 使用 FFmpeg API 推流：采集到的画面(QImage) + 可选音频PCM -> RTMP(FLV)。
 *
 * 说明：
 * - 视频：H.264 (YUV420P)
 * - 音频：AAC（如启用）
 * - 线程：内部使用 worker 线程进行编码/写包，避免阻塞 UI
 */
class RtmpAvStreamer : public QObject
{
    Q_OBJECT
public:
    explicit RtmpAvStreamer(QObject* parent = nullptr);
    ~RtmpAvStreamer() override;

    // 兼容旧接口名：现在使用 SRT 推流，port 默认 10080
    void setSrsServer(const QString& host, quint16 port = 10080);
    void setStreamKey(const QString& streamKey);

    void setVideoFormat(int width, int height, int fps);
    void setAudioFormat(int sampleRate, int channels);
    void setAudioEnabled(bool enabled);

    bool start();
    void stop();
    bool isRunning() const;

    // 可在 UI 线程调用：内部会排队到 worker 线程
    void pushVideoFrame(const QImage& frame);
    // 直接推送 QVideoFrame（支持 NV21/NV12/YUYV 等无法直接转 QImage 的像素格式）
    void pushVideoFrame(const QVideoFrame& frame);
    void pushPcm(const QByteArray& pcm);

signals:
    void started();
    void stopped();
    void logMessage(const QString& message);
    void errorOccurred(const QString& error);

private:
    QString buildSrtUrl() const;

    QString m_host = QStringLiteral("47.100.126.194");
    quint16 m_port = 10080; // SRT 端口（请按服务器实际配置调整）
    QString m_streamKey;

    int m_outWidth = 640;
    int m_outHeight = 480;
    int m_fps = 25;

    int m_sampleRate = 44100;
    int m_channels = 1;
    bool m_audioEnabled = true;

    RtmpAvStreamerWorker* m_worker = nullptr;
    QThread* m_thread = nullptr;
    std::atomic_bool m_running{false};

    // QVideoFrame -> BGRA(QImage) 转换（在 UI 线程做，避免修改 worker 输入接口）
    SwsContext* m_inSwsCtx = nullptr;
    int m_inW = 0;
    int m_inH = 0;
    int m_inFmt = -1; // AVPixelFormat

    // 启动阶段可能会出现“首帧到达但 worker 尚未 started，enqueueVideo 直接丢帧”的窗口。
    // 这里缓存最近一帧，等 started() 后立刻发送，避免服务端 SRT 超时。
    QMutex m_pendingMutex;
    QImage m_pendingVideoFrame;
    bool m_hasPendingVideoFrame = false;
};
