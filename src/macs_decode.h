#ifndef __H_MACS_DECODE__
#define __H_MACS_DECODE__

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

} MACS_DECODE_HANDLE;

int32_t macs_decode_open(MACS_DECODE_HANDLE* wav, int16_t up_sampling);
void macs_decode_close(MACS_DECODE_HANDLE* wav);
int32_t macs_decode_parse_header(MACS_DECODE_HANDLE* wav, FILE* fp);
size_t macs_decode_exec(MACS_DECODE_HANDLE* wav, int16_t* output_buffer, int16_t* source_buffer, size_t source_buffer_len);

#endif