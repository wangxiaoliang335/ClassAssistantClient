#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVPacket;
struct SwrContext;

/**
 * 使用 FFmpeg API 推流，将本地音频通过 SRT 推送到服务器（MPEG-TS over SRT）。
 */
class RtmpMediaStreamer : public QObject
{
    Q_OBJECT
public:
    explicit RtmpMediaStreamer(QObject* parent = nullptr);
    ~RtmpMediaStreamer() override;

    // 兼容旧接口名：现在使用 SRT 推流，port 默认 10080
    void setSrsServer(const QString& host, quint16 port = 10080);
    void setStreamKey(const QString& streamKey);
    void setAudioFormat(int sampleRate, int channels);

    bool start();
    void stop();
    bool isRunning() const;
    void pushPcm(const QByteArray& pcm);

signals:
    void started();
    void stopped();
    void logMessage(const QString& message);
    void errorOccurred(const QString& error);

private:
    QString buildSrtUrl() const;
    bool initOutputContext();
    bool initEncoder();
    bool initSwr();
    void cleanup();
    void emitLog(const QString& msg);
    void emitError(const QString& msg);

    QString m_host = QStringLiteral("47.100.126.194");
    quint16 m_port = 10080; // SRT 默认端口（按服务器实际配置调整）
    QString m_streamKey;
    int m_sampleRate = 48000;
    int m_channels = 1;
    bool m_isRunning = false;
    int64_t m_pts = 0;
    QByteArray m_pcmBuffer;

    AVFormatContext* m_outputCtx = nullptr;
    AVCodecContext* m_codecCtx = nullptr;
    AVStream* m_audioStream = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    SwrContext* m_swrCtx = nullptr;
};

