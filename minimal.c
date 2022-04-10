// compile with
// gcc minimal.c -lavformat -lavcodec -lavutil -lswscale
// Add -L"/path/to/external/ffmpeg/libraries" to command above if those libraries are not in the default path

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void save_rgb(const AVFrame* f, enum AVPixelFormat px, int width, int height, const char* fn) {
    struct SwsContext* sws = sws_getContext(f->width, f->height, px,
            width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    size_t imbufsize = width * height * 3;
    unsigned char* im_buf = (unsigned char*) malloc(imbufsize);
    uint8_t *rdata[4] = {im_buf, 0, 0, 0};
    int rlinesize[4] = {width, 0, 0, 0};
    sws_scale(sws, (const uint8_t* const*)(f->data), f->linesize, 0, f->height,
              rdata, rlinesize);
    FILE* fobj = fopen(fn, "wb");
    if (fobj) {
        fwrite(im_buf, 1, imbufsize, fobj);
        fclose(fobj);
    }
    free(im_buf);
    sws_freeContext(sws);
}

void save_yuv(const char *filename, const struct AVFrame *pic) {
    FILE *fp = 0;
    int i, j, shift;

    fp = fopen(filename, "wb");
    if(fp) {
        for(i = 0; i < 3; i++) {
            shift = (i == 0 ? 0 : 1);
            uint8_t* yuv_factor = pic->data[i];
            for(j = 0; j < (pic->height >> shift); j++) {
                fwrite(yuv_factor, (pic->width >> shift), 1, fp);
                yuv_factor += pic->linesize[i];
            }
        }
        fclose(fp);
    }
}

int main(int argc, char** argv) {
    if (argc < 4)
        return 1;
    const char *path_in = argv[1], *path_yuv = argv[2], *path_rgb = argv[3];
    struct AVFormatContext* fmt_ctx = avformat_alloc_context();
    int ret = avformat_open_input(&fmt_ctx, path_in, NULL, NULL);
    if (ret < 0) {
        avformat_free_context(fmt_ctx);
        return 2;
    }
    avformat_find_stream_info(fmt_ctx, NULL);
    // av_dump_format(fmt_ctx, 0, path_in, 0);

    // first stream (video in this example)
    struct AVStream* stream = fmt_ctx->streams[0];
    const struct AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    struct AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, stream->codecpar);
    avcodec_open2(codec_ctx, codec, NULL);

    struct AVPacket* pkt = av_packet_alloc();
    struct AVFrame* frame = av_frame_alloc();

    for (;;) {
        ret = av_read_frame(fmt_ctx, pkt);
        if (ret < 0)
            exit(4);
        // select first stream (video in this example)
        if (pkt->stream_index != 0) {
            av_packet_unref(pkt);
            continue;
        }
        avcodec_send_packet(codec_ctx, pkt);
        av_packet_unref(pkt);

        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret < 0) {
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                // we need to read more data
            } else {
                exit(3);
            }
        } else {
            save_yuv(path_yuv, frame);
            printf("Play with \"ffplay -f rawvideo -pix_fmt yuv420p -video_size 852x480 %s\"\n", path_yuv);
            save_rgb(frame, codec_ctx->pix_fmt, 1280, 720, path_rgb);
            printf("Play with \"ffplay -f rawvideo -pix_fmt rgb24 -video_size 1280x720 %s\"\n", path_rgb);
            av_frame_unref(frame);
            break;
        }
    }
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_close(codec_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return 0;
}

