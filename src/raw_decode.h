#ifndef __H_RAW_DECODE__
#define __H_RAW_DECODE__

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {

  int32_t sample_rate;
  int16_t channels;

  int32_t resample_rate;
  size_t resample_counter;

  in16_t use_24bit;

} RAW_DECODE_HANDLE;

int32_t raw_decode_open(RAW_DECODE_HANDLE* pcm, int32_t sample_rate, int16_t channels, int16_t use_24bit);
void raw_decode_close(RAW_DECODE_HANDLE* pcm);
size_t raw_decode_exec(RAW_DECODE_HANDLE* pcm, void* output_buffer, int16_t* source_buffer, size_t source_buffer_len);

#endif