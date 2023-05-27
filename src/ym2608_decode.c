#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "ym2608_decode.h"

//
//  MSM6258V ADPCM constant tables
//
static const int16_t step_adjust[] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

static const int16_t step_size[] = { 
        16,  17,  19,  21,  23,  25,  28,  31,  34,  37,  41,  45,   50,   55,   60,   66,
        73,  80,  88,  97, 107, 118, 130, 143, 157, 173, 190, 209,  230,  253,  279,  307,
       337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552 };

//
//  YM2608 ADPCM decode
//
static inline int16_t ym2608_decode(uint8_t code, int16_t* step_index, int16_t last_data) {

  int16_t si = *step_index;
  int16_t ss = step_size[ si ];

  int16_t delta = ( ss >> 3 );
  if (code & 0x01) {
    delta += (ss >> 2);
  }
  if (code & 0x02) {
    delta += (ss >> 1);
  }
  if (code & 0x04) {
    delta += ss;
  }
  if (code & 0x08) {
    delta = -delta;
  }
    
  int16_t estimate = last_data + delta;
  if (estimate > 2047) {
    estimate = 2047;
  }

  if (estimate < -2048) {
    estimate = -2048;
  }

  si += step_adjust[ code ];
  if (si < 0) {
    si = 0;
  }
  if (si > 48) {
    si = 48;
  }
  *step_index = si;

  return estimate;
}

//
//  YM2608 ADPCM encode
//
static uint8_t ym2608_encode(int16_t current_data, int16_t last_estimate, int16_t* step_index, int16_t* new_estimate) {

  int16_t ss = step_size[ *step_index ];

  int16_t delta = current_data - last_estimate;

  uint8_t code = 0x00;
  if (delta < 0) {
    code = 0x08;          // bit3 = 1
    delta = -delta;
  }
  if (delta >= ss) {
    code += 0x04;         // bit2 = 1
    delta -= ss;
  }
  if (delta >= (ss>>1)) {
    code += 0x02;         // bit1 = 1
    delta -= ss>>1;
  }
  if (delta >= (ss>>2)) {
    code += 0x01;         // bit0 = 1
  } 

  // need to use decoder to estimate
  *new_estimate = ym2608_decode(code, step_index, last_estimate);

  return code;
}

//
//  initialize ym2608 decoder handle
//
int32_t ym2608_decode_open(YM2608_DECODE_HANDLE* ym2608, int32_t sample_rate, int16_t channels) {

  int32_t rc = -1;

  ym2608->sample_rate = sample_rate;
  ym2608->channels = channels;

  ym2608->step_index = 0;
  ym2608->last_estimate = 0;

  ym2608->adpcm_counter = 0;

  ym2608->resample_rate = 48000;
  ym2608->resample_counter = 0;

  rc = 0;

exit:
  return rc;
}

//
//  close ym2608 decoder handle
//
void ym2608_decode_close(YM2608_DECODE_HANDLE* ym2608) {

}

//
//  execute ym2608 decoding
//
size_t ym2608_decode_exec(YM2608_DECODE_HANDLE* ym2608, int16_t* output_buffer, uint8_t* source_buffer, size_t source_buffer_len) {

  size_t source_buffer_ofs = 0;
  size_t output_buffer_ofs = 0;

  while (source_buffer_ofs < source_buffer_len) {

    // up sampling
    ym2608->resample_counter += ym2608->sample_rate;
    if (ym2608->resample_counter < ym2608->resample_rate) {

      output_buffer[ output_buffer_ofs ++ ] = ym2608->last_estimate * 16;
      output_buffer[ output_buffer_ofs ++ ] = ym2608->last_estimate * 16;

    } else {

      uint8_t code;
      if ((ym2608->adpcm_counter % 2) == 0) {
        code = source_buffer[ source_buffer_ofs ] & 0x0f;
      } else {
        code = (source_buffer[ source_buffer_ofs++ ] >> 4) & 0x0f;
      }
      ym2608->adpcm_counter++;

      int16_t step_index = ym2608->step_index;
      int16_t new_estimate = ym2608_decode(code, &step_index, ym2608->last_estimate);
      output_buffer[ output_buffer_ofs ++ ] = new_estimate * 16;   // 12bit signed PCM to 16bit signed PCM
      output_buffer[ output_buffer_ofs ++ ] = new_estimate * 16;   // mono to stereo duplication
      ym2608->step_index = step_index;
      ym2608->last_estimate = new_estimate;

      ym2608->resample_counter -= ym2608->resample_rate;

    }

    //printf("source_buffer_ofs=%d, output_buffer_ofs=%d\n", source_buffer_ofs, output_buffer_ofs);

  }

  return output_buffer_ofs;
}
