#pragma execution_character_set("utf-8")
#include "RtmpAvStreamer.h"

#include <QDebug>
#include <QThread>
#include <QRegularExpression>
#include <QVideoFrame>
#include <QAbstractVideoBuffer>

#include <mutex>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace {

// 统一输出到 Windows 的 OutputDebugString（便于用 DebugView / VS 输出窗口查看）
static inline void OdsLine(const QString& s)
{
#ifdef _WIN32
    ::OutputDebugStringW(reinterpret_cast<LPCWSTR>(s.utf16()));
    ::OutputDebugStringW(L"\r\n");
#else
    qWarning().noquote() << s;
#endif
}

static inline void OdsWarning(const QString& s)
{
#ifdef _WIN32
    QString msg = QStringLiteral("[WARNING] ") + s;
    ::OutputDebugStringW(reinterpret_cast<LPCWSTR>(msg.utf16()));
    ::OutputDebugStringW(L"\r\n");
#else
    qWarning().noquote() << s;
#endif
}

QString avErrorToString(int error)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(error, errbuf, sizeof(errbuf));
    return QString::fromUtf8(errbuf);
}

} // namespace

RtmpAvStreamerWorker::RtmpAvStreamerWorker(QObject* parent) : QObject(parent)
{
    static std::once_flag ffmpegInitFlag;
    std::call_once(ffmpegInitFlag, []() {
        avformat_network_init();
    });
}

RtmpAvStreamerWorker::~RtmpAvStreamerWorker()
{
    stopInternal();
    cleanup();
}

void RtmpAvStreamerWorker::configure(const QString& host,
                                     quint16 port,
                                     const QString& streamKey,
                                     int outW,
                                     int outH,
                                     int fps,
                                     bool audioEnabled,
                                     int sampleRate,
                                     int channels)
{
    m_host = host;
    m_port = port;
    m_streamKey = streamKey;
    m_outWidth = outW;
    m_outHeight = outH;
    m_fps = fps <= 0 ? 25 : fps;
    m_audioEnabled = audioEnabled;
    m_sampleRate = sampleRate <= 0 ? 44100 : sampleRate;
    m_channels = channels <= 0 ? 1 : channels;
}

QString RtmpAvStreamerWorker::buildSrtUrl() const
{
    // SRS/SRT 常见发布格式：
    // srt://host:port?streamid=#!::r=live/<streamKey>,m=publish
    // 说明：streamKey 这里仅做基础拼接，非法字符请在上层 sanitizeId。
    return QStringLiteral("srt://%1:%2?streamid=#!::r=live/%3,m=publish")
        .arg(m_host)
        .arg(m_port)
        .arg(m_streamKey);
}

void RtmpAvStreamerWorker::startInternal()
{
    if (m_running) {
        emit errorOccurred(QStringLiteral("推流已在进行中"));
        return;
    }
    if (m_streamKey.isEmpty()) {
        emit errorOccurred(QStringLiteral("SRT 流名称为空，无法开始推流"));
        return;
    }

    cleanup();
    m_audioPts = 0;
    m_videoPts = 0;
    m_pcmBuffer.clear();
    m_running = false;

    if (!initOutputContext()) {
        cleanup();
        return;
    }
    if (!initVideoEncoder()) {
        cleanup();
        return;
    }
    if (m_audioEnabled) {
        if (!initAudioEncoder()) {
            cleanup();
            return;
        }
        if (!initSwr()) {
            cleanup();
            return;
        }
    }

    int ret = avformat_write_header(m_outputCtx, nullptr);
    if (ret < 0) {
        emit errorOccurred(QStringLiteral("写入 SRT 头部失败: %1").arg(avErrorToString(ret)));
        cleanup();
        return;
    }

    m_running = true;
    emit started();
    emit logMessage(QStringLiteral("推流已启动，目标: %1").arg(buildSrtUrl()));
}

void RtmpAvStreamerWorker::stopInternal()
{
    if (!m_running) {
        cleanup();
        return;
    }

    if (m_vCodecCtx) {
        avcodec_send_frame(m_vCodecCtx, nullptr);
        while (avcodec_receive_packet(m_vCodecCtx, m_packet) == 0) {
            m_packet->stream_index = m_videoStream ? m_videoStream->index : 0;
            av_packet_rescale_ts(m_packet, m_vCodecCtx->time_base, m_videoStream->time_base);
            av_interleaved_write_frame(m_outputCtx, m_packet);
            av_packet_unref(m_packet);
        }
    }

    if (m_audioEnabled && m_aCodecCtx) {
        avcodec_send_frame(m_aCodecCtx, nullptr);
        while (avcodec_receive_packet(m_aCodecCtx, m_packet) == 0) {
            m_packet->stream_index = m_audioStream->index;
            av_packet_rescale_ts(m_packet, AVRational{1, m_sampleRate}, m_audioStream->time_base);
            av_interleaved_write_frame(m_outputCtx, m_packet);
            av_packet_unref(m_packet);
        }
    }

    if (m_outputCtx) {
        av_write_trailer(m_outputCtx);
    }

    cleanup();
    m_running = false;
    emit stopped();
    emit logMessage(QStringLiteral("推流已停止"));
}

void RtmpAvStreamerWorker::enqueueVideo(const QImage& img)
{
    if (!m_running || img.isNull()) return;
    encodeVideo(img);
}

void RtmpAvStreamerWorker::enqueueAudio(const QByteArray& pcm)
{
    if (!m_running || !m_audioEnabled || pcm.isEmpty()) return;
    m_pcmBuffer.append(pcm);
    encodeAudioFromBuffer();
}

bool RtmpAvStreamerWorker::initOutputContext()
{
    QString url = buildSrtUrl();
    QByteArray urlUtf8 = url.toUtf8();
    // SRT 通常承载 MPEG-TS（更适配低延迟推流）
    int ret = avformat_alloc_output_context2(&m_outputCtx, nullptr, "mpegts", urlUtf8.constData());
    if (ret < 0 || !m_outputCtx) {
        emit errorOccurred(QStringLiteral("创建输出上下文失败: %1").arg(avErrorToString(ret)));
        return false;
    }

    if (!(m_outputCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open2(&m_outputCtx->pb, urlUtf8.constData(), AVIO_FLAG_WRITE, nullptr, nullptr);
        if (ret < 0) {
            emit errorOccurred(QStringLiteral("打开 SRT 输出失败: %1").arg(avErrorToString(ret)));
            return false;
        }
    }

    return true;
}

bool RtmpAvStreamerWorker::initVideoEncoder()
{
    const AVCodec* vcodec = avcodec_find_encoder_by_name("libx264");
    if (!vcodec) {
        vcodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    if (!vcodec) {
        emit errorOccurred(QStringLiteral("未找到 H264 编码器"));
        return false;
    }

    m_videoStream = avformat_new_stream(m_outputCtx, nullptr);
    if (!m_videoStream) {
        emit errorOccurred(QStringLiteral("创建视频流失败"));
        return false;
    }

    m_vCodecCtx = avcodec_alloc_context3(vcodec);
    if (!m_vCodecCtx) {
        emit errorOccurred(QStringLiteral("创建视频编码上下文失败"));
        return false;
    }

    m_vCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    m_vCodecCtx->width = m_outWidth;
    m_vCodecCtx->height = m_outHeight;
    m_vCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_vCodecCtx->time_base = AVRational{1, m_fps};
    m_vCodecCtx->framerate = AVRational{m_fps, 1};
    m_vCodecCtx->gop_size = m_fps * 2;
    m_vCodecCtx->max_b_frames = 0;
    m_vCodecCtx->bit_rate = 1200 * 1000;

    if (m_outputCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        m_vCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    AVDictionary* vopts = nullptr;
    av_dict_set(&vopts, "preset", "ultrafast", 0);
    av_dict_set(&vopts, "tune", "zerolatency", 0);
    int ret = avcodec_open2(m_vCodecCtx, vcodec, &vopts);
    av_dict_free(&vopts);
    if (ret < 0) {
        emit errorOccurred(QStringLiteral("打开 H264 编码器失败: %1").arg(avErrorToString(ret)));
        return false;
    }

    m_videoStream->time_base = m_vCodecCtx->time_base;
    ret = avcodec_parameters_from_context(m_videoStream->codecpar, m_vCodecCtx);
    if (ret < 0) {
        emit errorOccurred(QStringLiteral("复制视频编码参数失败: %1").arg(avErrorToString(ret)));
        return false;
    }

    m_vFrame = av_frame_alloc();
    if (!m_vFrame) {
        emit errorOccurred(QStringLiteral("分配视频帧失败"));
        return false;
    }
    m_vFrame->format = m_vCodecCtx->pix_fmt;
    m_vFrame->width = m_vCodecCtx->width;
    m_vFrame->height = m_vCodecCtx->height;
    ret = av_frame_get_buffer(m_vFrame, 32);
    if (ret < 0) {
        emit errorOccurred(QStringLiteral("为视频帧分配缓冲区失败: %1").arg(avErrorToString(ret)));
        return false;
    }

    m_packet = av_packet_alloc();
    if (!m_packet) {
        emit errorOccurred(QStringLiteral("分配编码包失败"));
        return false;
    }
    return true;
}

bool RtmpAvStreamerWorker::initAudioEncoder()
{
    const AVCodec* acodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!acodec) {
        emit errorOccurred(QStringLiteral("未找到 AAC 编码器"));
        return false;
    }

    m_audioStream = avformat_new_stream(m_outputCtx, nullptr);
    if (!m_audioStream) {
        emit errorOccurred(QStringLiteral("创建音频流失败"));
        return false;
    }
    m_audioStream->time_base = AVRational{1, m_sampleRate};

    m_aCodecCtx = avcodec_alloc_context3(acodec);
    if (!m_aCodecCtx) {
        emit errorOccurred(QStringLiteral("创建音频编码上下文失败"));
        return false;
    }

    m_aCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    m_aCodecCtx->sample_rate = m_sampleRate;
    m_aCodecCtx->channel_layout = av_get_default_channel_layout(m_channels);
    m_aCodecCtx->channels = m_channels;
    m_aCodecCtx->bit_rate = 128000;
    m_aCodecCtx->sample_fmt = acodec->sample_fmts ? acodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    m_aCodecCtx->time_base = AVRational{1, m_sampleRate};

    if (m_outputCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        m_aCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    int ret = avcodec_open2(m_aCodecCtx, acodec, nullptr);
    if (ret < 0) {
        emit errorOccurred(QStringLiteral("打开 AAC 编码器失败: %1").arg(avErrorToString(ret)));
        return false;
    }

    ret = avcodec_parameters_from_context(m_audioStream->codecpar, m_aCodecCtx);
    if (ret < 0) {
        emit errorOccurred(QStringLiteral("复制音频编码参数失败: %1").arg(avErrorToString(ret)));
        return false;
    }

    m_aFrame = av_frame_alloc();
    if (!m_aFrame) {
        emit errorOccurred(QStringLiteral("分配音频帧失败"));
        return false;
    }
    m_aFrame->format = m_aCodecCtx->sample_fmt;
    m_aFrame->channel_layout = m_aCodecCtx->channel_layout;
    m_aFrame->sample_rate = m_aCodecCtx->sample_rate;
    m_aFrame->nb_samples = m_aCodecCtx->frame_size > 0 ? m_aCodecCtx->frame_size : 1024;

    ret = av_frame_get_buffer(m_aFrame, 0);
    if (ret < 0) {
        emit errorOccurred(QStringLiteral("为音频帧分配缓冲区失败: %1").arg(avErrorToString(ret)));
        return false;
    }
    return true;
}

bool RtmpAvStreamerWorker::initSwr()
{
    m_swrCtx = swr_alloc_set_opts(nullptr,
                                  m_aCodecCtx->channel_layout,
                                  m_aCodecCtx->sample_fmt,
                                  m_aCodecCtx->sample_rate,
                                  av_get_default_channel_layout(m_channels),
                                  AV_SAMPLE_FMT_S16,
                                  m_sampleRate,
                                  0,
                                  nullptr);
    if (!m_swrCtx) {
        emit errorOccurred(QStringLiteral("创建音频重采样上下文失败"));
        return false;
    }
    int ret = swr_init(m_swrCtx);
    if (ret < 0) {
        emit errorOccurred(QStringLiteral("初始化音频重采样失败: %1").arg(avErrorToString(ret)));
        return false;
    }
    return true;
}

void RtmpAvStreamerWorker::ensureSws(const QImage& img)
{
    int srcW = img.width();
    int srcH = img.height();
    if (m_swsCtx && srcW == m_srcW && srcH == m_srcH) return;

    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }

    m_srcW = srcW;
    m_srcH = srcH;
    m_swsCtx = sws_getContext(srcW, srcH, AV_PIX_FMT_BGRA,
                              m_outWidth, m_outHeight, AV_PIX_FMT_YUV420P,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);
}

void RtmpAvStreamerWorker::encodeVideo(const QImage& imgIn)
{
    if (!m_vCodecCtx || !m_videoStream || !m_vFrame) return;

    QImage img = imgIn;
    if (img.format() != QImage::Format_ARGB32 && img.format() != QImage::Format_RGB32) {
        img = img.convertToFormat(QImage::Format_ARGB32);
    }

    ensureSws(img);
    if (!m_swsCtx) return;

    av_frame_make_writable(m_vFrame);

    const uint8_t* srcSlice[1] = { img.bits() };
    int srcStride[1] = { img.bytesPerLine() };

    sws_scale(m_swsCtx, srcSlice, srcStride, 0, img.height(), m_vFrame->data, m_vFrame->linesize);

    m_vFrame->pts = m_videoPts++;

    int ret = avcodec_send_frame(m_vCodecCtx, m_vFrame);
    if (ret < 0) {
        emit errorOccurred(QStringLiteral("编码视频帧失败: %1").arg(avErrorToString(ret)));
        return;
    }

    while ((ret = avcodec_receive_packet(m_vCodecCtx, m_packet)) == 0) {
        m_packet->stream_index = m_videoStream->index;
        av_packet_rescale_ts(m_packet, m_vCodecCtx->time_base, m_videoStream->time_base);
        int wr = av_interleaved_write_frame(m_outputCtx, m_packet);
        av_packet_unref(m_packet);
        if (wr < 0) {
            emit errorOccurred(QStringLiteral("写入 SRT 视频帧失败: %1").arg(avErrorToString(wr)));
            break;
        }
    }
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return;
    if (ret < 0) {
        emit errorOccurred(QStringLiteral("接收视频编码包失败: %1").arg(avErrorToString(ret)));
    }
}

void RtmpAvStreamerWorker::encodeAudioFromBuffer()
{
    if (!m_audioEnabled || !m_aCodecCtx || !m_audioStream || !m_aFrame || !m_swrCtx) return;

    const int bytesPerSample = 2;
    const int frameSamples = m_aFrame->nb_samples;
    const int frameBytes = frameSamples * m_channels * bytesPerSample;

    while (m_pcmBuffer.size() >= frameBytes) {
        QByteArray chunk = m_pcmBuffer.left(frameBytes);
        m_pcmBuffer.remove(0, frameBytes);

        const uint8_t* inData[1] = { reinterpret_cast<const uint8_t*>(chunk.constData()) };
        int ret = swr_convert(m_swrCtx, m_aFrame->data, frameSamples, inData, frameSamples);
        if (ret < 0) {
            emit errorOccurred(QStringLiteral("音频重采样失败: %1").arg(avErrorToString(ret)));
            continue;
        }

        m_aFrame->pts = m_audioPts;
        m_audioPts += ret;

        ret = avcodec_send_frame(m_aCodecCtx, m_aFrame);
        if (ret < 0) {
            emit errorOccurred(QStringLiteral("编码音频帧失败: %1").arg(avErrorToString(ret)));
            continue;
        }

        while ((ret = avcodec_receive_packet(m_aCodecCtx, m_packet)) == 0) {
            m_packet->stream_index = m_audioStream->index;
            av_packet_rescale_ts(m_packet, AVRational{1, m_sampleRate}, m_audioStream->time_base);
            int wr = av_interleaved_write_frame(m_outputCtx, m_packet);
            av_packet_unref(m_packet);
            if (wr < 0) {
                emit errorOccurred(QStringLiteral("写入 SRT 音频帧失败: %1").arg(avErrorToString(wr)));
                break;
            }
        }

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            continue;
        } else if (ret < 0) {
            emit errorOccurred(QStringLiteral("接收音频编码包失败: %1").arg(avErrorToString(ret)));
        }
    }
}

void RtmpAvStreamerWorker::cleanup()
{
    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
    if (m_vFrame) {
        av_frame_free(&m_vFrame);
        m_vFrame = nullptr;
    }
    if (m_aFrame) {
        av_frame_free(&m_aFrame);
        m_aFrame = nullptr;
    }
    if (m_swrCtx) {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    m_srcW = 0;
    m_srcH = 0;
    if (m_vCodecCtx) {
        avcodec_free_context(&m_vCodecCtx);
        m_vCodecCtx = nullptr;
    }
    if (m_aCodecCtx) {
        avcodec_free_context(&m_aCodecCtx);
        m_aCodecCtx = nullptr;
    }
    if (m_outputCtx) {
        if (!(m_outputCtx->oformat->flags & AVFMT_NOFILE) && m_outputCtx->pb) {
            avio_closep(&m_outputCtx->pb);
        }
        avformat_free_context(m_outputCtx);
        m_outputCtx = nullptr;
    }
    m_videoStream = nullptr;
    m_audioStream = nullptr;
    m_pcmBuffer.clear();
}

RtmpAvStreamer::RtmpAvStreamer(QObject* parent)
    : QObject(parent)
{
    m_worker = new RtmpAvStreamerWorker();
    m_thread = new QThread(this);
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);
    connect(m_thread, &QThread::started, this, [this]() {
        emit logMessage(QStringLiteral("RtmpAvStreamer worker thread started"));
    });

    // forward logs
    connect(m_worker, &RtmpAvStreamerWorker::started, this, [this]() {
        m_running.store(true);
        emit started();

        // flush pending frame (if any)
        QImage pending;
        {
            QMutexLocker locker(&m_pendingMutex);
            if (m_hasPendingVideoFrame && !m_pendingVideoFrame.isNull()) {
                pending = m_pendingVideoFrame;
                m_pendingVideoFrame = QImage();
                m_hasPendingVideoFrame = false;
            }
        }
        if (!pending.isNull()) {
            // now running==true, this will be queued to worker and不会再被丢弃
            pushVideoFrame(pending);
        }
    });
    connect(m_worker, &RtmpAvStreamerWorker::stopped, this, [this]() {
        m_running.store(false);
        emit stopped();
    });
    connect(m_worker, &RtmpAvStreamerWorker::logMessage, this, &RtmpAvStreamer::logMessage);
    connect(m_worker, &RtmpAvStreamerWorker::errorOccurred, this, &RtmpAvStreamer::errorOccurred);

    m_thread->start();
}

RtmpAvStreamer::~RtmpAvStreamer()
{
    stop();
    if (m_inSwsCtx) {
        sws_freeContext(m_inSwsCtx);
        m_inSwsCtx = nullptr;
    }
}

void RtmpAvStreamer::setSrsServer(const QString& host, quint16 port)
{
    m_host = host;
    m_port = port;
}

void RtmpAvStreamer::setStreamKey(const QString& streamKey)
{
    m_streamKey = streamKey;
}

void RtmpAvStreamer::setVideoFormat(int width, int height, int fps)
{
    if (width > 0) m_outWidth = width;
    if (height > 0) m_outHeight = height;
    if (fps > 0) m_fps = fps;
}

void RtmpAvStreamer::setAudioFormat(int sampleRate, int channels)
{
    if (sampleRate > 0) m_sampleRate = sampleRate;
    if (channels > 0) m_channels = channels;
}

void RtmpAvStreamer::setAudioEnabled(bool enabled)
{
    m_audioEnabled = enabled;
}

bool RtmpAvStreamer::start()
{
    if (!m_worker) return false;
    if (m_streamKey.isEmpty()) {
        emit errorOccurred(QStringLiteral("SRT 流名称为空"));
        return false;
    }

    QMetaObject::invokeMethod(m_worker, [this]() {
        m_worker->configure(m_host, m_port, m_streamKey, m_outWidth, m_outHeight, m_fps,
                            m_audioEnabled, m_sampleRate, m_channels);
        m_worker->startInternal();
    }, Qt::QueuedConnection);
    return true;
}

void RtmpAvStreamer::stop()
{
    if (!m_worker) return;
    QMetaObject::invokeMethod(m_worker, &RtmpAvStreamerWorker::stopInternal, Qt::QueuedConnection);
}

bool RtmpAvStreamer::isRunning() const
{
    return m_running.load();
}

void RtmpAvStreamer::pushVideoFrame(const QImage& frame)
{
    if (!m_worker) return;
    if (frame.isNull()) return;

    // 如果还没 started，先缓存一帧，等 started() 后再发（避免 enqueueVideo 因 m_running=false 直接 return）
    if (!isRunning()) {
        QMutexLocker locker(&m_pendingMutex);
        m_pendingVideoFrame = frame;
        m_hasPendingVideoFrame = true;
        return;
    }

    QImage copy = frame;
    QMetaObject::invokeMethod(m_worker, [this, copy]() {
        m_worker->enqueueVideo(copy);
    }, Qt::QueuedConnection);
}

void RtmpAvStreamer::pushVideoFrame(const QVideoFrame& frame)
{
    if (!m_worker) return;

    QVideoFrame f(frame);
    if (!f.isValid()) return;

    // 尝试走 Qt 原生可转换路径
    QImage::Format imgFmt = QVideoFrame::imageFormatFromPixelFormat(f.pixelFormat());
    if (imgFmt != QImage::Format_Invalid) {
        if (!f.map(QAbstractVideoBuffer::ReadOnly)) return;
        QImage img = QImage(f.bits(), f.width(), f.height(), f.bytesPerLine(), imgFmt).copy();
        f.unmap();
        if (!img.isNull()) {
            pushVideoFrame(img);
        }
        return;
    }

    // 处理常见 YUV/packed 格式：用 swscale 转到 BGRA，再复用现有 encode 流程
    AVPixelFormat srcFmt = AV_PIX_FMT_NONE;
    bool swapUV = false; // YV12: Y, V, U

    switch (f.pixelFormat()) {
    case QVideoFrame::Format_NV21:
        srcFmt = AV_PIX_FMT_NV21;
        break;
    case QVideoFrame::Format_NV12:
        srcFmt = AV_PIX_FMT_NV12;
        break;
    case QVideoFrame::Format_YUYV:
        srcFmt = AV_PIX_FMT_YUYV422;
        break;
    case QVideoFrame::Format_UYVY:
        srcFmt = AV_PIX_FMT_UYVY422;
        break;
    case QVideoFrame::Format_YV12:
        // YV12: Y, V, U (planar 420)
        srcFmt = AV_PIX_FMT_YUV420P;
        swapUV = true;
        break;
    case QVideoFrame::Format_YUV420P:
        srcFmt = AV_PIX_FMT_YUV420P;
        break;
    default:
        OdsWarning(QStringLiteral("RtmpAvStreamer::pushVideoFrame(QVideoFrame) unsupported pixelFormat=%1")
                   .arg(int(f.pixelFormat())));
        return;
    }

    // 添加调试信息：YUYV等格式无法直接转换为QImage，需要使用swscale转换
    if (f.pixelFormat() == QVideoFrame::Format_YUYV || 
        f.pixelFormat() == QVideoFrame::Format_UYVY ||
        f.pixelFormat() == QVideoFrame::Format_NV21 ||
        f.pixelFormat() == QVideoFrame::Format_NV12) {
        // 这些格式无法直接转换为QImage，需要使用swscale，这是正常的
        // 只在首次遇到时输出一次日志，避免日志过多
        static bool logged = false;
        if (!logged) {
            OdsLine(QStringLiteral("SRT_CAM 检测到YUV格式(pixelFormat=%1)，将使用swscale转换为BGRA，这是正常处理流程")
                    .arg(int(f.pixelFormat())));
            logged = true;
        }
    }

    if (!f.map(QAbstractVideoBuffer::ReadOnly)) return;

    const int w = f.width();
    const int h = f.height();
    if (w <= 0 || h <= 0) {
        f.unmap();
        return;
    }

    // sws_getCachedContext 复用上下文
    m_inSwsCtx = sws_getCachedContext(
        m_inSwsCtx,
        w, h, srcFmt,
        w, h, AV_PIX_FMT_BGRA,
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    m_inW = w;
    m_inH = h;
    m_inFmt = int(srcFmt);

    if (!m_inSwsCtx) {
        f.unmap();
        OdsWarning(QStringLiteral("sws_getCachedContext failed for srcFmt=%1 w=%2 h=%3")
                   .arg(int(srcFmt)).arg(w).arg(h));
        return;
    }

    QImage out(w, h, QImage::Format_ARGB32);
    if (out.isNull()) {
        f.unmap();
        return;
    }

    const uint8_t* srcSlice[4] = { nullptr, nullptr, nullptr, nullptr };
    int srcStride[4] = { 0, 0, 0, 0 };

    // QVideoFrame 支持多平面：优先用 plane API，避免手算偏移
    const int planes = f.planeCount();
    if (srcFmt == AV_PIX_FMT_NV21 || srcFmt == AV_PIX_FMT_NV12) {
        // NV12/NV21: Y + UV/VU
        // - 常见情况：planes==2（plane0: Y, plane1: UV/VU）
        // - 某些 Windows/DirectShow 驱动：planes==1，但内存仍是连续的 Y 后接 UV/VU
        const uint8_t* p0 = reinterpret_cast<const uint8_t*>(f.bits(0));
        const int s0 = f.bytesPerLine(0);
        if (!p0 || s0 <= 0) {
            f.unmap();
            OdsWarning(QStringLiteral("NV12/NV21 invalid plane0 ptr/stride. planes=%1").arg(planes));
            return;
        }

        const uint8_t* p1 = nullptr;
        int s1 = 0;
        if (planes >= 2 && f.bits(1)) {
            p1 = reinterpret_cast<const uint8_t*>(f.bits(1));
            s1 = f.bytesPerLine(1);
        } else {
            // fallback: contiguous buffer
            const int64_t yBytes = int64_t(s0) * h;
            const int64_t uvBytes = int64_t(s0) * (h / 2); // UV plane height is h/2
            const int64_t need = yBytes + uvBytes;
            const int64_t mapped = f.mappedBytes();
            if (mapped > 0 && mapped >= need) {
                p1 = p0 + yBytes;
                s1 = s0;
            } else {
                f.unmap();
                OdsWarning(QStringLiteral("NV12/NV21 planeCount<2 and not enough mapped bytes. planes=%1 mapped=%2 need>=%3 w=%4 h=%5 bpl0=%6")
                           .arg(planes).arg(mapped).arg(need).arg(w).arg(h).arg(s0));
                return;
            }
        }

        srcSlice[0] = p0;
        srcStride[0] = s0;
        srcSlice[1] = p1;
        srcStride[1] = (s1 > 0) ? s1 : s0;
    } else if (srcFmt == AV_PIX_FMT_YUYV422 || srcFmt == AV_PIX_FMT_UYVY422) {
        const uint8_t* p0 = reinterpret_cast<const uint8_t*>(f.bits(0));
        int s0 = f.bytesPerLine(0);
        if (!p0 || s0 <= 0) {
            f.unmap();
            OdsWarning(QStringLiteral("YUYV/UYVY invalid plane0 ptr/stride. planes=%1 pixelFormat=%2")
                       .arg(planes).arg(int(f.pixelFormat())));
            return;
        }
        srcSlice[0] = p0;
        srcStride[0] = s0;
    } else if (srcFmt == AV_PIX_FMT_YUV420P) {
        const uint8_t* pY = reinterpret_cast<const uint8_t*>(f.bits(0));
        int sY = f.bytesPerLine(0);
        const uint8_t* pU = (planes >= 2) ? reinterpret_cast<const uint8_t*>(f.bits(1)) : nullptr;
        const uint8_t* pV = (planes >= 3) ? reinterpret_cast<const uint8_t*>(f.bits(2)) : nullptr;
        int sU = (planes >= 2) ? f.bytesPerLine(1) : 0;
        int sV = (planes >= 3) ? f.bytesPerLine(2) : 0;

        if (!pY || sY <= 0) {
            f.unmap();
            OdsWarning(QStringLiteral("YUV420P invalid plane0 ptr/stride. planes=%1").arg(planes));
            return;
        }

        // fallback: contiguous I420/YV12 buffer (planes==1)
        if (planes < 3 || !pU || !pV) {
            // 对于 planes==1 的情况，我们只能做一个“尽力而为”的解析：
            // 假设布局为：Y (stride=sY, height=h) + U (stride=sY/2, height=h/2) + V (stride=sY/2, height=h/2)
            const int sUV = sY / 2;
            const int64_t yBytes = int64_t(sY) * h;
            const int64_t uBytes = int64_t(sUV) * (h / 2);
            const int64_t vBytes = int64_t(sUV) * (h / 2);
            const int64_t need = yBytes + uBytes + vBytes;
            const int64_t mapped = f.mappedBytes();
            if (sUV > 0 && mapped > 0 && mapped >= need) {
                pU = pY + yBytes;
                pV = pU + uBytes;
                sU = sUV;
                sV = sUV;
            } else {
                f.unmap();
                OdsWarning(QStringLiteral("YUV420P planeCount<3 and not enough mapped bytes. planes=%1 mapped=%2 need>=%3 w=%4 h=%5 bpl0=%6")
                           .arg(planes).arg(mapped).arg(need).arg(w).arg(h).arg(sY));
                return;
            }
        }

        if (swapUV) {
            std::swap(pU, pV);
            std::swap(sU, sV);
        }
        srcSlice[0] = pY; srcStride[0] = sY;
        srcSlice[1] = pU; srcStride[1] = sU;
        srcSlice[2] = pV; srcStride[2] = sV;
    }

    uint8_t* dstSlice[4] = { reinterpret_cast<uint8_t*>(out.bits()), nullptr, nullptr, nullptr };
    int dstStride[4] = { out.bytesPerLine(), 0, 0, 0 };

    sws_scale(m_inSwsCtx, srcSlice, srcStride, 0, h, dstSlice, dstStride);
    f.unmap();

    pushVideoFrame(out);
}

void RtmpAvStreamer::pushPcm(const QByteArray& pcm)
{
    if (!m_worker) return;
    QByteArray copy = pcm;
    QMetaObject::invokeMethod(m_worker, [this, copy]() {
        m_worker->enqueueAudio(copy);
    }, Qt::QueuedConnection);
}

QString RtmpAvStreamer::buildSrtUrl() const
{
    return QStringLiteral("srt://%1:%2?streamid=#!::r=live/%3,m=publish")
        .arg(m_host)
        .arg(m_port)
        .arg(m_streamKey);
}

// (removed) sanitizeId moved to caller-side sanitize helper


