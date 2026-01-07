#include "RtmpMediaStreamer.h"

#include <QDebug>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

namespace {

QString avErrorToString(int error)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(error, errbuf, sizeof(errbuf));
    return QString::fromUtf8(errbuf);
}

}  // namespace

RtmpMediaStreamer::RtmpMediaStreamer(QObject* parent)
    : QObject(parent)
{
    static std::once_flag ffmpegInitFlag;
    std::call_once(ffmpegInitFlag, []() {
        avformat_network_init();
    });
}

RtmpMediaStreamer::~RtmpMediaStreamer()
{
    stop();
}

void RtmpMediaStreamer::setSrsServer(const QString& host, quint16 port)
{
    m_host = host;
    m_port = port;
}

void RtmpMediaStreamer::setStreamKey(const QString& streamKey)
{
    m_streamKey = streamKey;
}

void RtmpMediaStreamer::setAudioFormat(int sampleRate, int channels)
{
    m_sampleRate = sampleRate;
    m_channels = channels <= 0 ? 1 : channels;
}

bool RtmpMediaStreamer::start()
{
    if (m_isRunning) {
        emitError(QStringLiteral("RTMP 推流已在进行中"));
        return false;
    }

    if (m_streamKey.isEmpty()) {
        emitError(QStringLiteral("RTMP 流名称为空，无法开始推流"));
        return false;
    }

    cleanup();
    m_pts = 0;
    m_pcmBuffer.clear();

    if (!initOutputContext()) {
        cleanup();
        return false;
    }
    if (!initEncoder()) {
        cleanup();
        return false;
    }
    if (!initSwr()) {
        cleanup();
        return false;
    }

    int ret = avformat_write_header(m_outputCtx, nullptr);
    if (ret < 0) {
        emitError(QStringLiteral("写入 RTMP 头部失败: %1").arg(avErrorToString(ret)));
        cleanup();
        return false;
    }

    emit started();
    emitLog(QStringLiteral("RTMP 推流已启动，目标: %1").arg(buildRtmpUrl()));
    m_isRunning = true;
    return true;
}

void RtmpMediaStreamer::stop()
{
    if (!m_isRunning) {
        cleanup();
        return;
    }

    // Flush encoder
    if (m_codecCtx) {
        avcodec_send_frame(m_codecCtx, nullptr);
        while (avcodec_receive_packet(m_codecCtx, m_packet) == 0) {
            m_packet->stream_index = m_audioStream->index;
            av_packet_rescale_ts(m_packet, {1, m_sampleRate}, m_audioStream->time_base);
            av_interleaved_write_frame(m_outputCtx, m_packet);
            av_packet_unref(m_packet);
        }
    }

    if (m_outputCtx) {
        av_write_trailer(m_outputCtx);
    }

    cleanup();
    m_isRunning = false;
    emit stopped();
    emitLog(QStringLiteral("RTMP 推流已停止"));
}

bool RtmpMediaStreamer::isRunning() const
{
    return m_isRunning;
}

void RtmpMediaStreamer::pushPcm(const QByteArray& pcm)
{
    if (!m_isRunning || pcm.isEmpty()) {
        return;
    }

    m_pcmBuffer.append(pcm);
    const int bytesPerSample = 2; // S16LE
    const int frameSamples = m_frame->nb_samples;
    const int frameBytes = frameSamples * m_channels * bytesPerSample;

    while (m_pcmBuffer.size() >= frameBytes) {
        QByteArray chunk = m_pcmBuffer.left(frameBytes);
        m_pcmBuffer.remove(0, frameBytes);

        const uint8_t* inData[1] = { reinterpret_cast<const uint8_t*>(chunk.constData()) };
        int ret = swr_convert(m_swrCtx,
                              m_frame->data,
                              frameSamples,
                              inData,
                              frameSamples);
        if (ret < 0) {
            emitError(QStringLiteral("音频重采样失败: %1").arg(avErrorToString(ret)));
            continue;
        }

        m_frame->pts = m_pts;
        m_pts += ret;

        ret = avcodec_send_frame(m_codecCtx, m_frame);
        if (ret < 0) {
            emitError(QStringLiteral("编码音频帧失败: %1").arg(avErrorToString(ret)));
            continue;
        }

        while ((ret = avcodec_receive_packet(m_codecCtx, m_packet)) == 0) {
            m_packet->stream_index = m_audioStream->index;
            av_packet_rescale_ts(m_packet, {1, m_sampleRate}, m_audioStream->time_base);
            ret = av_interleaved_write_frame(m_outputCtx, m_packet);
            av_packet_unref(m_packet);
            if (ret < 0) {
                emitError(QStringLiteral("写入 RTMP 帧失败: %1").arg(avErrorToString(ret)));
                break;
            }
        }

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            continue;
        } else if (ret < 0) {
            emitError(QStringLiteral("接收编码包失败: %1").arg(avErrorToString(ret)));
        }
    }
}

QString RtmpMediaStreamer::buildRtmpUrl() const
{
    return QStringLiteral("rtmp://%1:%2/live/%3")
        .arg(m_host)
        .arg(m_port)
        .arg(m_streamKey);
}

bool RtmpMediaStreamer::initOutputContext()
{
    QString url = buildRtmpUrl();
    QByteArray urlUtf8 = url.toUtf8();
    int ret = avformat_alloc_output_context2(&m_outputCtx, nullptr, "flv", urlUtf8.constData());
    if (ret < 0 || !m_outputCtx) {
        emitError(QStringLiteral("创建输出上下文失败: %1").arg(avErrorToString(ret)));
        return false;
    }

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) {
        emitError(QStringLiteral("未找到 AAC 编码器"));
        return false;
    }

    m_audioStream = avformat_new_stream(m_outputCtx, nullptr);
    if (!m_audioStream) {
        emitError(QStringLiteral("创建音频流失败"));
        return false;
    }
    m_audioStream->time_base = AVRational{1, m_sampleRate};

    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        emitError(QStringLiteral("创建编码上下文失败"));
        return false;
    }

    m_codecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    m_codecCtx->sample_rate = m_sampleRate;
    m_codecCtx->channel_layout = av_get_default_channel_layout(m_channels);
    m_codecCtx->channels = m_channels;
    m_codecCtx->bit_rate = 128000;
    m_codecCtx->sample_fmt = codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    m_codecCtx->time_base = AVRational{1, m_sampleRate};

    if (m_outputCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        m_codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    ret = avcodec_open2(m_codecCtx, codec, nullptr);
    if (ret < 0) {
        emitError(QStringLiteral("打开 AAC 编码器失败: %1").arg(avErrorToString(ret)));
        return false;
    }

    ret = avcodec_parameters_from_context(m_audioStream->codecpar, m_codecCtx);
    if (ret < 0) {
        emitError(QStringLiteral("复制编码参数失败: %1").arg(avErrorToString(ret)));
        return false;
    }

    if (!(m_outputCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open2(&m_outputCtx->pb, urlUtf8.constData(), AVIO_FLAG_WRITE, nullptr, nullptr);
        if (ret < 0) {
            emitError(QStringLiteral("打开 RTMP 输出失败: %1").arg(avErrorToString(ret)));
            return false;
        }
    }

    return true;
}

bool RtmpMediaStreamer::initEncoder()
{
    m_frame = av_frame_alloc();
    if (!m_frame) {
        emitError(QStringLiteral("分配音频帧失败"));
        return false;
    }
    m_frame->format = m_codecCtx->sample_fmt;
    m_frame->channel_layout = m_codecCtx->channel_layout;
    m_frame->sample_rate = m_codecCtx->sample_rate;
    m_frame->nb_samples = m_codecCtx->frame_size > 0 ? m_codecCtx->frame_size : 1024;

    int ret = av_frame_get_buffer(m_frame, 0);
    if (ret < 0) {
        emitError(QStringLiteral("为音频帧分配缓冲区失败: %1").arg(avErrorToString(ret)));
        return false;
    }

    m_packet = av_packet_alloc();
    if (!m_packet) {
        emitError(QStringLiteral("分配编码包失败"));
        return false;
    }

    return true;
}

bool RtmpMediaStreamer::initSwr()
{
    m_swrCtx = swr_alloc_set_opts(nullptr,
        m_codecCtx->channel_layout,
        m_codecCtx->sample_fmt,
        m_codecCtx->sample_rate,
        av_get_default_channel_layout(m_channels),
        AV_SAMPLE_FMT_S16,
        m_sampleRate,
        0,
        nullptr);
    if (!m_swrCtx) {
        emitError(QStringLiteral("创建音频重采样上下文失败"));
        return false;
    }

    int ret = swr_init(m_swrCtx);
    if (ret < 0) {
        emitError(QStringLiteral("初始化音频重采样失败: %1").arg(avErrorToString(ret)));
        return false;
    }
    return true;
}

void RtmpMediaStreamer::cleanup()
{
    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_swrCtx) {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }
    if (m_outputCtx) {
        if (!(m_outputCtx->oformat->flags & AVFMT_NOFILE) && m_outputCtx->pb) {
            avio_closep(&m_outputCtx->pb);
        }
        avformat_free_context(m_outputCtx);
        m_outputCtx = nullptr;
    }
    m_audioStream = nullptr;
    m_pcmBuffer.clear();
}

void RtmpMediaStreamer::emitLog(const QString& msg)
{
    emit logMessage(msg);
}

void RtmpMediaStreamer::emitError(const QString& msg)
{
    emit errorOccurred(msg);
}

