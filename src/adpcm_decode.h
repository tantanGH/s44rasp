#ifndef __H_ADPCM_DECODE__
#define __H_ADPCM_DECODE__

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {

  int32_t sample_rate;

  int16_t step_index;
  int16_t last_estimate;

  size_t adpcm_counter;

  int32_t resample_rate;
  size_t resample_counter;

} ADPCM_DECODE_HANDLE;

int32_t adpcm_decode_open(ADPCM_DECODE_HANDLE* adpcm);
void adpcm_decode_close(ADPCM_DECODE_HANDLE* adpcm);
size_t adpcm_decode_exec(ADPCM_DECODE_HANDLE* adpcm, int16_t* output_buffer, uint8_t* source_buffer, size_t source_buffer_len);

#endif