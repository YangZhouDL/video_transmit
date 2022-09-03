#include <stdio.h>
#define __STDC_CONSTANT_MACROS
extern "C"{
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

//@brief
// return
static AVFormatContext *open_dev()
{
    int ret = 0;
    char errors[1024] = {
        0,
    };

    // ctx
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *options = NULL;

    //[[video device]:[audio device]]
    char *devicename = "/dev/video2";

    // register audio device
    avdevice_register_all();

    // get format
    AVInputFormat *iformat = av_find_input_format("video4linux2");

    av_dict_set(&options, "video_size", "2560x720", 0);
    av_dict_set(&options, "pixel_format", "yuyv422", 0);

    // open device
    if ((ret = avformat_open_input(&fmt_ctx, devicename, iformat, &options)) < 0)
    {
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        return NULL;
    }

    return fmt_ctx;
}

void rec_video()
{
    int ret = 0;
    AVFormatContext *fmt_ctx = NULL;
    int count = 0;

    // pakcet
    AVPacket pkt;

    // set log level
    av_log_set_level(AV_LOG_DEBUG);

    // create file
    char *out = "/home/redwall/视频/out.yuv";
    FILE *outfile = fopen(out, "wb+");

    //打开设备
    fmt_ctx = open_dev();

    // read data from device
    while ((ret = av_read_frame(fmt_ctx, &pkt)) == 0 &&
           count++ < 100)
    {
        av_log(NULL, AV_LOG_INFO,
               "packet size is %d(%p)\n",
               pkt.size, pkt.data);

        fwrite(pkt.data, 1, pkt.size, outfile); // 614400
        fflush(outfile);
        av_packet_unref(&pkt); // release pkt
    }

__ERROR:
    if (outfile)
    {
        // close file
        fclose(outfile);
    }

    // close device and release ctx
    if (fmt_ctx)
    {
        avformat_close_input(&fmt_ctx);
    }

    av_log(NULL, AV_LOG_DEBUG, "finish!\n");
    return;
}

int main(int argc, char *argv[])
{
    rec_video();
    return 0;
}