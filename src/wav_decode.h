#ifndef __H_WAV_DECODE__
#define __H_WAV_DECODE__

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {

  int32_t sample_rate;
  int16_t channels;
  int32_t byte_rate;
  int16_t block_align;
  int16_t bits_per_sample;
  int32_t duration;
  
  int32_t resample_rate;
  size_t resample_counter;
  int16_t up_sampling;

} WAV_DECODE_HANDLE;

int32_t wav_decode_open(WAV_DECODE_HANDLE* wav, int16_t up_sampling);
void wav_decode_close(WAV_DECODE_HANDLE* wav);
int32_t wav_decode_parse_header(WAV_DECODE_HANDLE* wav, FILE* fp);
size_t wav_decode_exec(WAV_DECODE_HANDLE* wav, int16_t* output_buffer, int16_t* source_buffer, size_t source_buffer_len);

#endif