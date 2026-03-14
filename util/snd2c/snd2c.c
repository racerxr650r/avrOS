#include <stdio.h>
#include <stdlib.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input_sound_file> <output_c_file>\n", argv[0]);
    return 1;
  }

  const char *input_filename = argv[1];
  const char *output_filename = argv[2];

  // Open input file
  AVFormatContext *format_ctx = NULL;
  if (avformat_open_input(&format_ctx, input_filename, NULL, NULL) != 0) {
    fprintf(stderr, "Error opening input file.\n");
    return 1;
  }

  // Find audio stream
  int audio_stream_index = -1;
  for (int i = 0; i < format_ctx->nb_streams; i++) {
    if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index = i;
      break;
    }
  }
  if (audio_stream_index == -1) {
    fprintf(stderr, "No audio stream found.\n");
    return 1;
  }

  // Get audio codec parameters
  AVCodecParameters *codecpar = format_ctx->streams[audio_stream_index]->codecpar;

  // Find audio decoder
  const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
  if (codec == NULL) {
    fprintf(stderr, "No decoder found.\n");
    return 1;
  }

  // Create codec context
  AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
  if (codec_ctx == NULL) {
    fprintf(stderr, "Error allocating codec context.\n");
    return 1;
  }
  if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
    fprintf(stderr, "Error copying codec parameters to context.\n");
    return 1;
  }
  if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
    fprintf(stderr, "Error opening codec.\n");
    return 1;
  }

  // Create resampler context
  SwrContext *swr_ctx = swr_alloc();
  if (swr_ctx == NULL) {
    fprintf(stderr, "Error allocating resampler context.\n");
    return 1;
  }

  // Set resampler parameters
  av_opt_set_int(swr_ctx, "in_channel_count", codec_ctx->ch_layout.nb_channels, 0);
  av_opt_set_int(swr_ctx, "in_sample_rate", codec_ctx->sample_rate, 0);
  av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codec_ctx->sample_fmt, 0);
  av_opt_set_int(swr_ctx, "out_channel_count", 1, 0); // Mono output
  av_opt_set_int(swr_ctx, "out_sample_rate", 8000, 0); // 8kHz output
  av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_U8, 0); // 8-bit PCM output
  if (swr_init(swr_ctx) < 0) {
    fprintf(stderr, "Error initializing resampler.\n");
    return 1;
  }

  // Open output file
  FILE *output_file = fopen(output_filename, "w");
  if (output_file == NULL) {
    fprintf(stderr, "Error opening output file.\n");
    return 1;
  }

  // Write C header
  fprintf(output_file, "#include <stdint.h>\n\n");
  fprintf(output_file, "const uint8_t sound_data[] = {\n");

  // Decode audio frames and convert to 8-bit PCM
  AVPacket packet;
  AVFrame *frame = av_frame_alloc();
  uint8_t *output_buffer = NULL;
  int output_buffer_size = 0;
  int samples_count = 0;
  while (av_read_frame(format_ctx, &packet) >= 0) {
    if (packet.stream_index == audio_stream_index) {
      if (avcodec_send_packet(codec_ctx, &packet) < 0) {
        fprintf(stderr, "Error sending packet to decoder.\n");
        return 1;
      }

      while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
        // Calculate output buffer size
        int output_linesize;
        output_buffer_size = av_samples_get_buffer_size(&output_linesize, 1, frame->nb_samples, AV_SAMPLE_FMT_U8, 0);
        output_buffer = (uint8_t *)realloc(output_buffer, output_buffer_size);

        // Resample audio frame
        int out_samples = swr_convert(swr_ctx, &output_buffer, frame->nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
        if (out_samples < 0) {
          fprintf(stderr, "Error resampling audio frame.\n");
          return 1;
        }

        // Write samples to output file
        for (int i = 0; i < out_samples; i++) {
          fprintf(output_file, "0x%02x, ", output_buffer[i]);
          samples_count++;
          if (samples_count % 12 == 0) {
            fprintf(output_file, "\n");
          }
        }
      }
    }
    av_packet_unref(&packet);
  }

  // Write C footer
  fprintf(output_file, "\n};\n\n");
  fprintf(output_file, "const int sound_data_size = %d;\n", samples_count);

  // Clean up
  free(output_buffer);
  av_frame_free(&frame);
  swr_free(&swr_ctx);
  avcodec_close(codec_ctx);
  avcodec_free_context(&codec_ctx);
  avformat_close_input(&format_ctx);

  fclose(output_file);

  return 0;
}
