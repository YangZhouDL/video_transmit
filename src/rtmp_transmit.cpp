#include <iostream>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/time.h"
}

using namespace std;

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

int ff_Error(int errNum)
{
    char buf[1024] = {0};
    av_strerror(errNum, buf, sizeof(buf));
    cout << buf << endl;

    return -1;
}

static double r2d(AVRational r)
{
    return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

int main()
{
    char *inUrl = "/home/redwall/视频/日落与渔.flv";
    char *outUrl = "rtmp://centos:7788/videotest";

    av_register_all();

    //初始化网络库
    avformat_network_init();

    AVFormatContext *ictx = NULL;

    //打开文件，解封文件头
    int re = avformat_open_input(&ictx, inUrl, 0, 0);
    if (re != 0)
    {
        return ff_Error(re);
    }

    cout << "open file " << inUrl << " success..." << endl;

    //获取音频视频流信息
    re = avformat_find_stream_info(ictx, 0);
    if (re != 0)
    {
        return ff_Error(re);
    }

    cout << "打印输入文件信息" << endl;
    av_dump_format(ictx, 0, inUrl, 0);

    //输出流
    //创建输出流上下文
    AVFormatContext *octx = NULL;
    re = avformat_alloc_output_context2(&octx, 0, "flv", outUrl);
    if (!octx)
    {
        return ff_Error(re);
    }

    cout << "输出上下文创建成功" << endl;

    //配置输出流
    //遍历输入的AVStream
    for (int i = 0; i < ictx->nb_streams; i++)
    {
        //创建输出流
        AVStream *out = avformat_new_stream(octx, ictx->streams[i]->codec->codec);
        if (!out)
        {
            return ff_Error(0);
        }
        //复制配置信息
        re = avcodec_parameters_copy(out->codecpar, ictx->streams[i]->codecpar);
        out->codec->codec_tag = 0;
    }

    cout << "打印输出流信息" << endl;
    av_dump_format(octx, 0, outUrl, 1);

    // rtmp推流
    //打开io
    re = avio_open(&octx->pb, outUrl, AVIO_FLAG_WRITE);

    if (!octx->pb)
    {
        return ff_Error(re);
    }

    //写入头信息
    re = avformat_write_header(octx, 0);
    if (re < 0)
    {
        return ff_Error(re);
    }

    AVPacket pkt;
    long long startTime = av_gettime();
    while(true)
    {
        re = av_read_frame(ictx, &pkt);
        if (re != 0)
        {
            break;
        }

        cout << pkt.pts << " " << flush;

        //计算转换pts dts
        AVRational itime = ictx->streams[pkt.stream_index]->time_base;
        AVRational otime = octx->streams[pkt.stream_index]->time_base;
        pkt.pts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        pkt.dts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        pkt.duration = av_rescale_q_rnd(pkt.duration, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        pkt.pos = -1;

        //视频帧推送速度
        if (ictx->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            AVRational tb = ictx->streams[pkt.stream_index]->time_base;
            //已经过去的时间
            long long now = av_gettime() - startTime;
            long long dts = 0;
            dts = pkt.dts * (1000 * 1000 * r2d(tb));

            if (dts > now)
                av_usleep(dts - now);
        }

        //推送帧数据
        re = av_interleaved_write_frame(octx, &pkt);
        if (re < 0)
        {
            return ff_Error(re);
        }
    }

    cout << "rtmp 推流结束" << endl;

    return 0;
}