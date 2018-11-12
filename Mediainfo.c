//
// Created by 刘伟 on 2018/11/10.
//
#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>

#define ERROR_STR_SIZE 1024
int pringLog(char *msg) {
    av_log(NULL, AV_LOG_ERROR, msg);

}

int main(int argc, char *argv[]) {

    int err_code;
    char errors[1024];

    char *src_fileName = NULL;
    char *dst_fileName = NULL;

    FILE *dst_fd = NULL;

    int audio_stream_index = -1;
    int len;

    AVFormatContext *ofmt_ctx = NULL;
    AVOutputFormat *output_fmt = NULL;

    AVStream *in_stream = NULL;
    AVStream *out_stream = NULL;

    AVFormatContext *fmt_ctx = NULL;

    AVPacket packet;


    av_log_set_level(AV_LOG_INFO);

    av_register_all();

    if (argc < 3) {
        av_log(NULL, AV_LOG_ERROR, "参数错误！");
        return -1;
    }

    src_fileName = argv[1];
    dst_fileName = argv[2];

    if (!src_fileName || !dst_fileName) {
        av_log(NULL, AV_LOG_ERROR, "参数错误！");
        return -1;
    }


    err_code = avformat_open_input(&fmt_ctx, src_fileName, NULL, NULL);

    if (err_code < 0) {
        av_log(NULL, AV_LOG_ERROR, "cant open file:%s\n", av_err2str(err_code));
        return -1;
    }


    av_dump_format(fmt_ctx, 0, src_fileName, 0);

    in_stream = fmt_ctx->streams[1];
    AVCodecParameters *in_codecpar = in_stream->codecpar;

    if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
        av_log(NULL, AV_LOG_ERROR, "The Codec type is invalid!\n");
        exit(1);
    }

    //outfile
    ofmt_ctx = avformat_alloc_context();
    output_fmt = av_guess_format(NULL,dst_fileName,NULL);
    if(!output_fmt){
        av_log(NULL, AV_LOG_DEBUG, "Cloud not guess file format \n");
        exit(1);
    }

    ofmt_ctx->oformat = output_fmt;

    out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if(!out_stream){
        av_log(NULL, AV_LOG_DEBUG, "Failed to create out stream!\n");
        exit(1);
    }

    if(fmt_ctx->nb_streams<2){
        av_log(NULL, AV_LOG_ERROR, "the number of stream is too less!\n");
        exit(1);
    }

    if((err_code = avcodec_parameters_copy(out_stream->codecpar, in_codecpar)) < 0 ){
        av_strerror(err_code, errors, ERROR_STR_SIZE);
        av_log(NULL, AV_LOG_ERROR,
               "Failed to copy codec parameter, %d(%s)\n",
               err_code, errors);
    }

    out_stream->codecpar->codec_tag = 0;

    if((err_code = avio_open(&ofmt_ctx->pb, dst_fileName, AVIO_FLAG_WRITE)) < 0) {
        av_strerror(err_code, errors, 1024);
        av_log(NULL, AV_LOG_DEBUG, "Could not open file %s, %d(%s)\n",
               dst_fileName,
               err_code,
               errors);
        exit(1);
    }

    av_dump_format(ofmt_ctx, 0, dst_fileName, 1);

    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    /*find best audio stream*/
    audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audio_stream_index < 0){
        av_log(NULL, AV_LOG_DEBUG, "Could not find %s stream in input file %s\n",
               av_get_media_type_string(AVMEDIA_TYPE_AUDIO),
               src_fileName);
        return AVERROR(EINVAL);
    }

    if (avformat_write_header(ofmt_ctx, NULL) < 0) {
        av_log(NULL, AV_LOG_DEBUG, "Error occurred when opening output file");
        exit(1);
    }

    /*read frames from media file*/
    while(av_read_frame(fmt_ctx, &packet) >=0 ){
        if(packet.stream_index == audio_stream_index){
            packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            packet.dts = packet.pts;
            packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
            packet.pos = -1;
            packet.stream_index = 0;
            av_interleaved_write_frame(ofmt_ctx, &packet);
            av_packet_unref(&packet);
        }
    }

    av_write_trailer(ofmt_ctx);

    /*close input media file*/
    avformat_close_input(&fmt_ctx);
    if(dst_fd) {
        fclose(dst_fd);
    }

    avio_close(ofmt_ctx->pb);

    return 0;
}
