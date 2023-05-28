#ifndef __H_YM2608_DECODE__
#define __H_YM2608_DECODE__

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {

  int32_t sample_rate;
  int16_t channels;

  uint32_t step_size;
  int16_t step_index;
  int32_t last_estimate;

  uint32_t step_size2;
  int16_t step_index2;
  int32_t last_estimate2;

  size_t adpcm_counter;

  int32_t resample_rate;
  size_t resample_counter;

  int16_t up_sampling;

  uint8_t* x1;
  uint8_t* lx1;
  uint8_t* rx1;

  int32_t back;
  int32_t lback;
  int32_t rback;

} YM2608_DECODE_HANDLE;

int32_t ym2608_decode_open(YM2608_DECODE_HANDLE* ym2608, int32_t sample_rate, int16_t channels, int16_t up_sampling);
void ym2608_decode_close(YM2608_DECODE_HANDLE* ym2608);
size_t ym2608_decode_exec(YM2608_DECODE_HANDLE* ym2608, int16_t* output_buffer, uint8_t* source_buffer, size_t source_buffer_len);

extern uint8_t ym2608_conv_table[];

#endif