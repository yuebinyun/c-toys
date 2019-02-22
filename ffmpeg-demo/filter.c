/*
 * Copyright (c) 2001 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * video decoding with libavcodec API example
 *
 * @example decode_video.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include "libavutil/opt.h"

//const char *filter_descr = "color=c=black@1:s=1920x1080[x0];[in1]scale=400:225[ine1];[x0][ine1]overlay=640*1/2:360*1/2[out]";
const char *filter_descr = "[in1]scale=400:225[ine1];color=c=black:s=1280x720,fps=fps=60[x0];[x0][ine1]overlay=1280*1/2:720*1/2[out]";
//const char *filter_descr = "[in1]scale=400:225[ine1];[ine1]scale=400*3:225*3[out]";

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename) {
    FILE *f;
    int i;

    f = fopen(filename, "w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}


int main(int argc, char **argv) {

    char buf[1024];

//    printf("%s %d \n", INPUT_FILE, argc);

    AVFormatContext *avFormatContext = avformat_alloc_context();
    if (avformat_open_input(&avFormatContext, argv[1], NULL, NULL) != 0) {
        printf("打开文件失败\n");
        exit(222);
    }

    AVCodec *avCodec;

    int videoStreamIndex = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &avCodec, 0);
    if (videoStreamIndex < 0) {
        printf("失败 ret = %d, line: %d\n", videoStreamIndex, __LINE__);
        printf("%s", av_err2str(videoStreamIndex));
        return -1;
    }

    AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(avCodecContext, avFormatContext->streams[videoStreamIndex]->codecpar);
    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
        printf("失败 %d\n", __LINE__);
        return -1;
    }

    AVFrame *avFrame = av_frame_alloc();
    AVFrame *avFilteredFrame = av_frame_alloc();
    AVPacket *avPacket = av_packet_alloc();

    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;

    const AVFilter *avFilterSrc = avfilter_get_by_name("buffer");
    const AVFilter *avFilterSink = avfilter_get_by_name("buffersink");

    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();

    AVFilterGraph *graph = avfilter_graph_alloc();
    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             avCodecContext->width, avCodecContext->height, AV_PIX_FMT_YUV420P,
             avFormatContext->streams[videoStreamIndex]->time_base.num,
             avFormatContext->streams[videoStreamIndex]->time_base.den,
             avFormatContext->streams[videoStreamIndex]->sample_aspect_ratio.num,
             avFormatContext->streams[videoStreamIndex]->sample_aspect_ratio.den);

    printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, args);

    int ret = avfilter_graph_create_filter(&buffersrc_ctx, avFilterSrc, "in1",
                                           args, NULL, graph);
    if (ret < 0) {
        printf("error %d\n", __LINE__);
        exit(22);
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx, avFilterSink, "out",
                                       NULL, NULL, graph);
    if (ret < 0) {
        printf("error %d\n", __LINE__);
    }

//    enum AVPixelFormat fmt = AV_PIX_FMT_YUV420P;
//    ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
//                         (uint8_t *) fmt,
//                         sizeof(fmt),
//                         AV_OPT_SEARCH_CHILDREN);

    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};
    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

    if (ret < 0) {
        exit(222);
    }

//    ret = av_opt_set_q(buffersink_ctx, "frame_rate", (AVRational) {1, 60},
//                       AV_OPT_SEARCH_CHILDREN);
//
//    if (ret < 0) {
//        printf("error %d\n %s", __LINE__, av_err2str(ret));
//        exit(222);
//    }

    outputs->name = av_strdup("in1");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if ((avfilter_graph_parse_ptr(graph, filter_descr,
                                  &inputs, &outputs, NULL)) < 0)
        return -1;

    if ((avfilter_graph_config(graph, NULL)) < 0)
        return -1;

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    int a = 0, b = 0;
    while (av_read_frame(avFormatContext, avPacket) >= 0) {
        if (avPacket->stream_index == videoStreamIndex) {
            ret = avcodec_send_packet(avCodecContext, avPacket);
            if (ret < 0) {
                printf("error: %s", av_err2str(ret));
                break;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(avCodecContext, avFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    ret = 0;
                    break;
                }
                if (ret < 0) {
                    break;
                }
                printf("frame %d pts %" PRId64 "  width=%d, height=%d\n",
                       ++a, avFrame->pts, avFrame->width, avFrame->height);

                avFrame->pts = avFrame->best_effort_timestamp;

                // push the decoded frame into the filter graph
                if (av_buffersrc_add_frame_flags(buffersrc_ctx, avFrame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    exit(-1);
                }

                while (1) {
                    ret = av_buffersink_get_frame_flags(buffersink_ctx, avFilteredFrame, 0);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        ret = 0;
                        break;
                    }
                    if (ret < 0) {
                        exit(-2);
                    }
                    printf("filtered frame %d pts %" PRId64 "  width=%d, height=%d\n",
                           ++b, avFilteredFrame->pts, avFilteredFrame->width,
                           avFilteredFrame->height);

                    printf("get frame rate : %d / %d\n", av_buffersink_get_frame_rate(buffersink_ctx).den,
                           av_buffersink_get_frame_rate(buffersink_ctx).num);

                    if (b % 100 == 0) {
                        snprintf(buf, sizeof(buf), "%s-%d.pgm", "name", b);
                        pgm_save(avFilteredFrame->data[0], avFilteredFrame->linesize[0],
                                 avFilteredFrame->width, avFilteredFrame->height, buf);
                    }

                    av_frame_unref(avFilteredFrame);
                }

                // pull filtered frames from the filter graph
                av_frame_unref(avFrame);
            }
            if (ret < 0) {
                break;
            }
        }
        av_packet_unref(avPacket);
    }

    avfilter_graph_free(&graph);
    avformat_close_input(&avFormatContext);
    avformat_free_context(avFormatContext);
    av_packet_free(&avPacket);
    av_frame_free(&avFrame);
    av_frame_free(&avFilteredFrame);
    avformat_free_context(avFormatContext);
    return 0;
}
