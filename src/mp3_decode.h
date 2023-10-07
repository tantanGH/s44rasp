#ifndef __H_MP3_DECODE__
#define __H_MP3_DECODE__

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <mad.h>

typedef struct mad_stream MAD_STREAM;
typedef struct mad_synth MAD_SYNTH;
typedef struct mad_header MAD_HEADER;
typedef struct mad_pcm MAD_PCM;
typedef struct mad_frame MAD_FRAME;
typedef mad_timer_t MAD_TIMER;

typedef struct {

  int16_t up_sampling;

  void* mp3_data;
  size_t mp3_data_len;
  int16_t mp3_quality;

  uint8_t* mp3_title;
  uint8_t* mp3_artist;
  uint8_t* mp3_album;

  int32_t mp3_sample_rate;
  int32_t mp3_channels;
  size_t resample_counter;

  int32_t mp3_frame_options;

  MAD_STREAM mad_stream;
  MAD_FRAME mad_frame;
  MAD_SYNTH mad_synth;
  MAD_TIMER mad_timer;

  MAD_PCM* current_mad_pcm;

} MP3_DECODE_HANDLE;

int32_t wav_decode_open(WAV_DECODE_HANDLE* wav, int16_t up_sampling);
void wav_decode_close(WAV_DECODE_HANDLE* wav);
int32_t wav_decode_parse_header(WAV_DECODE_HANDLE* wav, FILE* fp);
size_t wav_decode_exec(WAV_DECODE_HANDLE* wav, int16_t* output_buffer, int16_t* source_buffer, size_t source_buffer_len);

int32_t ym2608_decode_open(YM2608_DECODE_HANDLE* ym2608, int32_t sample_rate, int16_t channels, int16_t up_sampling);
void ym2608_decode_close(YM2608_DECODE_HANDLE* ym2608);
size_t ym2608_decode_exec(YM2608_DECODE_HANDLE* ym2608, int16_t* output_buffer, uint8_t* source_buffer, size_t source_buffer_len);

int32_t mp3_decode_open(MP3_DECODE_HANDLE* decode, int16_t up_sampling);
void mp3_decode_close(MP3_DECODE_HANDLE* decode);
int32_t mp3_decode_parse_tags(MP3_DECODE_HANDLE* decode, FILE* fp);
int32_t mp3_decode_setup(MP3_DECODE_HANDLE* decode, void* mp3_data, size_t mp3_data_len, int16_t mp3_quality);
size_t mp3_decode_exec(MP3_DECODE_HANDLE* decode, int16_t* output_buffer, size_t output_buffer_bytes);

#endif