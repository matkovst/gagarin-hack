#include <iostream>
extern "C" {
	#include <libavformat/avformat.h>
	#include <libavformat/avio.h>
	#include <libavcodec/avcodec.h>
}

inline bool check(int e, int iLine, const char* szFile)
{
    if (e < 0)
	{
		char errorStr[AV_ERROR_MAX_STRING_SIZE] = {0};
		av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, e);

        std::cerr << "[FFmpeg] General error " << e << " " 
				  << errorStr << " " 
				  << " at line " << iLine 
				  << " in file " << szFile << std::endl;
        return false;
    }
    return true;
}

#define CHECK(call) check(call, __LINE__, __FILE__)


class FFmpegH264Demuxer final
{
public:
    static int ReadPacket(void *opaque, uint8_t *pBuf, int nBuf) {
        return ((DataProvider *)opaque)->GetData(pBuf, nBuf);
    }

    class DataProvider {
    public:
        virtual ~DataProvider() {}
        virtual int GetData(uint8_t *pBuf, int nBuf) = 0;
    };

public:
    FFmpegH264Demuxer(const char* szFilePath) : FFmpegH264Demuxer(CreateFormatContext(szFilePath)) {}
    FFmpegH264Demuxer(DataProvider* pDataProvider) : FFmpegH264Demuxer(CreateFormatContext(pDataProvider)) {}
    ~FFmpegH264Demuxer()
	{
        if (m_pkt.data)
            av_packet_unref(&m_pkt);
        if (m_pktFiltered.data)
            av_packet_unref(&m_pktFiltered);

        avformat_close_input(&m_fmtc);
        if (m_avioc)
		{
            av_freep(&m_avioc->buffer);
            av_freep(&m_avioc);
        }
    }
    AVCodecID GetVideoCodec() { return m_eVideoCodec; }
    int gopSize() const noexcept { return m_gopSize; }
    int GetWidth() { return m_width; }
    int GetHeight() { return m_height; }
    int GetBitDepth() { return m_bitDepth; }
    int GetFrameSize()
    {
        return m_bitDepth == 8 ? m_width * m_height * 3 / 2: m_width * m_height * 3;
    }
    bool Demux(uint8_t** pktData, int* pktSize, int64_t* pktPts, int* pktFlags)
	{
        if (!m_fmtc)
            return false;

        *pktSize = 0;

        if (m_pkt.data)
            av_packet_unref(&m_pkt);

        int e = 0;
        while ((e = av_read_frame(m_fmtc, &m_pkt)) >= 0 && m_pkt.stream_index != m_iVideoStream) {
            av_packet_unref(&m_pkt);
        }
        if (e < 0)
            return false;

        if (m_applyFilter)
        {
            if (m_pktFiltered.data)
                av_packet_unref(&m_pktFiltered);
            CHECK(av_bsf_send_packet(m_bsfc, &m_pkt));
            CHECK(av_bsf_receive_packet(m_bsfc, &m_pktFiltered));
            *pktData = m_pktFiltered.data;
            *pktSize = m_pktFiltered.size;
            *pktPts = static_cast<int64_t>(m_pktFiltered.pts * m_userTimeScale * m_timeBase);
            *pktFlags = m_pktFiltered.flags;
        }
        else
        {
            *pktData = m_pkt.data;
            *pktSize = m_pkt.size;
            *pktPts = static_cast<int64_t>(m_pkt.pts * m_userTimeScale * m_timeBase);
            *pktFlags = m_pkt.flags;
        }

        return true;
    }

private:
    FFmpegH264Demuxer(AVFormatContext* m_fmtc) : m_fmtc(m_fmtc)
    {
        if (!m_fmtc)
        {
            std::cerr << "[Demuxer] No AVFormatContext provided." << std::endl;
            return;
        }

        std::cout << "[Demuxer] Media format: " << m_fmtc->iformat->long_name << " (" << m_fmtc->iformat->name << ")\n";

        CHECK(avformat_find_stream_info(m_fmtc, NULL));
        m_iVideoStream = av_find_best_stream(m_fmtc, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (m_iVideoStream < 0) {
            std::cerr << "[Demuxer] FFmpeg error: " << __FILE__ << " " << __LINE__ << " " 
                      << "Could not find stream in input file" << std::endl;
            return;
        }

        //m_fmtc->streams[m_iVideoStream]->need_parsing = AVSTREAM_PARSE_NONE;
        m_eVideoCodec = m_fmtc->streams[m_iVideoStream]->codecpar->codec_id;
        m_width = m_fmtc->streams[m_iVideoStream]->codecpar->width;
        m_height = m_fmtc->streams[m_iVideoStream]->codecpar->height;
        m_bitDepth = 8;
        if (m_fmtc->streams[m_iVideoStream]->codecpar->format == AV_PIX_FMT_YUV420P10LE)
            m_bitDepth = 10;
        if (m_fmtc->streams[m_iVideoStream]->codecpar->format == AV_PIX_FMT_YUV420P12LE)
            m_bitDepth = 12;

        auto codec = m_fmtc->streams[m_iVideoStream]->codec;
        m_gopSize = codec->gop_size;
        AVRational rTimeBase = m_fmtc->streams[m_iVideoStream]->time_base;
        m_timeBase = av_q2d(rTimeBase);
        m_userTimeScale = 1000;
        std::cout << "[Demuxer] (codec) GOP = " << codec->gop_size << std::endl;

        m_applyFilter = m_eVideoCodec == AV_CODEC_ID_H264 && (
                !strcmp(m_fmtc->iformat->long_name, "QuickTime / MOV") 
                || !strcmp(m_fmtc->iformat->long_name, "FLV (Flash Video)") 
                || !strcmp(m_fmtc->iformat->long_name, "Matroska / WebM")
            );

        av_init_packet(&m_pkt);
        m_pkt.data = NULL;
        m_pkt.size = 0;
        av_init_packet(&m_pktFiltered);
        m_pktFiltered.data = NULL;
        m_pktFiltered.size = 0;

        if (m_applyFilter) {
            const AVBitStreamFilter *bsf = av_bsf_get_by_name("h264_mp4toannexb");
            if (!bsf) {
                std::cerr << "[Demuxer] FFmpeg error: " << __FILE__ << " " << __LINE__ << " " 
                          << "av_bsf_get_by_name() failed" << std::endl;
                return;
            }
            CHECK(av_bsf_alloc(bsf, &m_bsfc));
            m_bsfc->par_in = m_fmtc->streams[m_iVideoStream]->codecpar;
            CHECK(av_bsf_init(m_bsfc));
        }
    }

    AVFormatContext *CreateFormatContext(DataProvider *pDataProvider)
    {
        // av_register_all();

        AVFormatContext *ctx = NULL;
        if (!(ctx = avformat_alloc_context())) {
            std::cerr << "[Demuxer] FFmpeg error: " << __FILE__ << " " << __LINE__ << std::endl;
            return NULL;
        }

        uint8_t *avioc_buffer = NULL;
        int avioc_buffer_size = 8 * 1024 * 1024;
        avioc_buffer = (uint8_t *)av_malloc(avioc_buffer_size);
        if (!avioc_buffer) {
            std::cerr << "[Demuxer] FFmpeg error: " << __FILE__ << " " << __LINE__ << std::endl;
            return NULL;
        }
        m_avioc = avio_alloc_context(avioc_buffer, avioc_buffer_size,
            0, pDataProvider, &ReadPacket, NULL, NULL);
        if (!m_avioc) {
            std::cerr << "[Demuxer] FFmpeg error: " << __FILE__ << " " << __LINE__ << std::endl;
            return NULL;
        }
        ctx->pb = m_avioc;

        CHECK(avformat_open_input(&ctx, NULL, NULL, NULL));
        return ctx;
    }

    AVFormatContext *CreateFormatContext(const char* szFilePath)
	{
        // av_register_all();
        avformat_network_init();

        AVFormatContext* ctx = NULL;
        CHECK(avformat_open_input(&ctx, szFilePath, NULL, NULL));
        return ctx;
    }

private:
    AVFormatContext* m_fmtc{nullptr};
    AVIOContext* m_avioc{nullptr};
    AVPacket m_pkt;
	AVPacket m_pktFiltered;
    AVBSFContext* m_bsfc{nullptr};

    int m_iVideoStream;
    bool m_applyFilter;
    AVCodecID m_eVideoCodec;
    int m_width, m_height, m_bitDepth;
    int m_gopSize{0};
    double m_timeBase{0.0};
    int64_t m_userTimeScale{0};
};

class FFmpegH264Parser final
{
public:
    FFmpegH264Parser(AVCodecContext* ctx)
        : m_ctx(ctx)
    {
        m_parser = av_parser_init(m_ctx->codec_id);
        if (!m_parser)
        {
            std::cerr << "[Parser] Parser not found" << std::endl;
        }
    }
    ~FFmpegH264Parser() = default;

    void parse()
    {
        if (!m_parser)
            return;
    }
private:
    AVCodecContext* m_ctx;
    AVCodecParserContext* m_parser{nullptr};
};
