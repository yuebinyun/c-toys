
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>

// print out the steps and errors
void logging(const char *fmt, ...);

// decode packets into frames
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);

// save a frame into a .pgm file
static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename);


int main(int argc, const char *argv[]) {

    logging("initializing all the containers, codecs and protocols");

    // ========= 1 =========
    // AVFormatContext holds the header information from the format (Container)
    // Allocating memory for this component
    AVFormatContext *pFormatContext = avformat_alloc_context();
    if(!pFormatContext){
        logging("Error could not allocate memory for format Context");
        logging("❌，申请内存失败。avformat_alloc_context()");
        return -1;
    }

    logging("opening the input file (%s) and loading format (container) header", argv[1]);
    if(avformat_open_input(&pFormatContext, argv[1], NULL, NULL) != 0){
        logging("打开文件失败。");
        return -1;
    }

    logging("format %s, number of stream %d, duration %lld us, bit_rate %lld",
            pFormatContext->iformat->name, pFormatContext->nb_streams,
            pFormatContext->duration, pFormatContext->bit_rate);

    logging("finding stream info from format");

    if(avformat_find_stream_info(pFormatContext, NULL) < 0){
        logging("ERROR could not get the stream info");
        return -1;
    }

    AVCodec *pCodec = NULL;
    AVCodecParameters *pCodecParameters = NULL;
    int video_stream_index = -1;

    for(int i = 0; i < pFormatContext->nb_streams; ++i){
        AVCodecParameters *pTempCodecParameters = NULL;
        pTempCodecParameters =  pFormatContext->streams[i]->codecpar;
        logging("AVStream->time_base before open coded %d/%d", pFormatContext->streams[i]->time_base.num,
                pFormatContext->streams[i]->time_base.den);
        logging("AVStream->r_frame_rate before open coded %d/%d", pFormatContext->streams[i]->r_frame_rate.num,
                pFormatContext->streams[i]->r_frame_rate.den);
        logging("AVStream->start_time %" PRId64, pFormatContext->streams[i]->start_time);
        logging("AVStream->duration %" PRId64, pFormatContext->streams[i]->duration);

        logging("finding the proper decoder (CODEC)");

        AVCodec *pTempCodec = NULL;
        pTempCodec = avcodec_find_decoder(pTempCodecParameters->codec_id);

        if(pTempCodec==NULL){
            logging("ERROR unsupported coded!");
            return -1;
        }

        if(pTempCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_index = i;
            pCodec = pTempCodec;
            pCodecParameters = pTempCodecParameters;
            logging("video codec: resolution %d x %d", pTempCodecParameters->width, pTempCodecParameters->height);
        }else if(pTempCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO){
            logging("audio codec: %d channels, sample rate %d", pTempCodecParameters->channels,
                    pTempCodecParameters->sample_rate);
        }

        logging("\tCodec %s ID %d bit_rate %lld", pTempCodec->name, pTempCodec->id, pCodecParameters->bit_rate);
    }

    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if(!pCodecContext){
        logging("failed to allocated memory for AVCodecContext");
        return -1;
    }

    if(avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0){
        logging("failed to copy codec params to codec context");
        return -1;
    }

    if(avcodec_open2(pCodecContext, pCodec, NULL) < 0){
        logging("failed to open codec through avcodec_open2");
        return -1;
    }

    AVFrame *pFrame = av_frame_alloc();
    if(!pFrame){
        logging("failed to allocated memory for AVFrame");
        return -1;
    }

    AVPacket *pPacket = av_packet_alloc();
    if(!pPacket){
        logging("failed to allocated memory for AVPacket");
        return -1;
    }

    int response = 0;
    int how_many_packets_to_process = 8;

    while (av_read_frame(pFormatContext, pPacket) >= 0){
        if(pPacket->stream_index == video_stream_index){
            logging("AVPacket->pts %" PRId64, pPacket->pts);
            response = decode_packet(pPacket, pCodecContext, pFrame);
            if(response < 0) break;
            if(--how_many_packets_to_process <= 0) break;
        }
        av_packet_unref(pPacket);
    }

    logging("releasing all the resources");

    avformat_close_input(&pFormatContext);
    avformat_free_context(pFormatContext);
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecContext);
    return 0;
}




static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame){
    int response = avcodec_send_packet(pCodecContext, pPacket);
    if(response < 0){
        logging("Error while sending a packet to the decoder: %s", av_err2str(response));
        return response;
    }

    while (response >= 0){
        response = avcodec_receive_frame(pCodecContext, pFrame);
        if(response == AVERROR(EAGAIN) || response == AVERROR_EOF){
            break;
        } else if(response < 0){
            logging("Error while receiving a frame from the decoder: %s", av_err2str(response));
            return response;
        }

        if(response >= 0){
            logging("Frame %d (type=%c, size=%d, bytes) pts %d dts %d key_frame %d [DTS %d]",
                    pCodecContext->frame_number,
                    av_get_picture_type_char(pFrame->pict_type),
                    pFrame->pkt_size,
                    pFrame->pts,
                    pFrame->pkt_dts,
                    pFrame->key_frame,
                    pFrame->coded_picture_number);

            char frame_filename[1024];
            snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecContext->frame_number);
            save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);
            av_frame_unref(pFrame);
        }
    }
    return 0;
}

static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename){
    FILE *f;
    int i;
    f = fopen(filename, "w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    for(i = 0; i < ysize; ++i){
        fwrite(buf + i*wrap, 1, (size_t) xsize, f);
    }
    fclose(f);
}

void logging(const char *fmt, ...){
    va_list args;
    fprintf(stderr, "LOG: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}