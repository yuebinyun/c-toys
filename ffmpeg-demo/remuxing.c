//
// Created by ban on 2019/1/25.
//

#include <stdio.h>
#include <stdarg.h>
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"

// print out the steps and errors
void logging(const char *fmt, ...);

void logging(const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "LOG: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}


int main(int args, const char **argv) {

    logging("hello...");

    // 1.
    AVFormatContext *formatContext = avformat_alloc_context();
    if (NULL == formatContext) {
        logging("%d", __LINE__);
        return 1;
    }

    if (avformat_open_input(&formatContext, argv[1], NULL, NULL) != 0) {
        logging("%d", __LINE__);
        return 1;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        logging("%d", __LINE__);
        return 1;
    }

    int number_of_stream = formatContext->nb_streams;
    int *streams_list = av_mallocz_array((size_t) number_of_stream, sizeof(*streams_list));

    AVCodecParameters *parameters = NULL;
    AVCodec *codec = NULL;
    unsigned int video_index = 0;
    unsigned int stream_index = 0;

    AVFormatContext *outFormatContext = NULL;
    avformat_alloc_output_context2(&outFormatContext, NULL, NULL, "4.ts");
    if (NULL == outFormatContext) {
        logging("%d", __LINE__);
        return 1;
    }


    for (int i = 0; i < formatContext->nb_streams; ++i) {
        parameters = formatContext->streams[i]->codecpar;
        codec = avcodec_find_decoder(parameters->codec_id);
        if (NULL == codec) {
            logging("%d", __LINE__);
            return 1;
        }

//        if(parameters->codec_type == AVMEDIA_TYPE_VIDEO){
//            video_index = i;
//            logging("视频文件总帧数 %d \n"
//                    "已经分流的帧数 %d \n"
//                    "视频时长 %d（s）",
//                    formatContext->streams[i]->nb_frames,
//                    formatContext->streams[i]->codec_info_nb_frames,
//                    formatContext->streams[i]->duration *
//                    formatContext->streams[i]->time_base.num /
//                    formatContext->streams[i]->time_base.den);
//            break;
//        }

        streams_list[i] = stream_index++;
        AVStream *out_stream = avformat_new_stream(outFormatContext, NULL);
        if (NULL == out_stream) {
            logging("%d", __LINE__);
            return 1;
        }
        avcodec_parameters_copy(out_stream->codecpar, parameters);
    }

    av_dump_format(outFormatContext, 0, "4.ts", 1);

    if (!(outFormatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outFormatContext->pb, "4.ts", AVIO_FLAG_WRITE) < 0) {
            logging("%d", __LINE__);
            return 1;
        }
    }

    AVDictionary *opts = NULL;

    // https://developer.mozilla.org/en-US/docs/Web/API/Media_Source_Extensions_API/Transcoding_assets_for_MSE
    av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);

    int ret = avformat_write_header(outFormatContext, NULL);
    if (ret < 0) {
        logging("%d", __LINE__);
        return 1;
    }


    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    if (NULL == codecContext) {
        logging("%d", __LINE__);
        return 1;
    }

    if (avcodec_parameters_to_context(codecContext, parameters) < 0) {
        logging("%d", __LINE__);
        return 1;
    }

    if (0 != avcodec_open2(codecContext, codec, NULL)) {
        logging("%d", __LINE__);
        return 1;
    }

    AVFrame *frame = av_frame_alloc();
    if (NULL == frame) {
        logging("%d", __LINE__);
        return 1;
    }

    AVPacket *packet = av_packet_alloc();
    if (NULL == packet) {
        logging("%d", __LINE__);
        return 1;
    }

//    int index = 0;

    while (av_read_frame(formatContext, packet) >= 0) {

        AVStream *in_stream, *out_stream;
        in_stream = formatContext->streams[packet->stream_index];
        packet->stream_index = streams_list[packet->stream_index];
        out_stream = outFormatContext->streams[packet->stream_index];
        packet->pts = av_rescale_q_rnd(packet->pts, in_stream->time_base, out_stream->time_base,
                                       AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        packet->dts = av_rescale_q_rnd(packet->dts, in_stream->time_base, out_stream->time_base,
                                       AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        packet->duration = av_rescale_q(packet->duration, in_stream->time_base, out_stream->time_base);
        packet->pos = -1;

        av_interleaved_write_frame(outFormatContext, packet);

//        if(packet->stream_index == video_index){
//            avcodec_send_packet(codecContext, packet);
//            avcodec_receive_frame(codecContext, frame);
//            logging("Frame %d,\tpkg->size %d", index, frame->pkt_size);
//            av_frame_unref(frame);
//            ++index;
//        }
        av_packet_unref(packet);
    }

    av_write_trailer(outFormatContext);
    if (outFormatContext && !(outFormatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&outFormatContext->pb);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);

    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);

    avformat_free_context(outFormatContext);


    return 0;
}


#pragma clang diagnostic pop