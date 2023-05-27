#ifndef __H_ADPCM_DECODE__
#define __H_ADPCM_DECODE__

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {

  int32_t sample_rate;
  int16_t auto_clip;

  int16_t step_index;
  int16_t last_estimate;

  size_t resample_counter;

} ADPCM_DECODE_HANDLE;

int32_t adpcm_decode_init(ADPCM_DECODE_HANDLE* adpcm, int32_t sample_rate, int16_t auto_clip);
void adpcm_decode_close(ADPCM_DECODE_HANDLE* adpcm);
int32_t adpcm_decode_exec(ADPCM_DECODE_HANDLE* adpcm, void* output_buffer, uint8_t* source_buffer, size_t source_buffer_len);

#endif