#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "adpcm_decode.h"

//
//  MSM6258V ADPCM constant tables
//
static const int16_t step_adjust[] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

static const int16_t step_size[] = { 
        16,  17,  19,  21,  23,  25,  28,  31,  34,  37,  41,  45,   50,   55,   60,   66,
        73,  80,  88,  97, 107, 118, 130, 143, 157, 173, 190, 209,  230,  253,  279,  307,
       337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552 };

//
//  MSM6258V ADPCM decode
//
static inline int16_t msm6258v_decode(uint8_t code, int16_t* step_index, int16_t last_data) {

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
//  MSM6258V ADPCM encode
//
static uint8_t msm6258v_encode(int16_t current_data, int16_t last_estimate, int16_t* step_index, int16_t* new_estimate) {

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
  *new_estimate = msm6258v_decode(code, step_index, last_estimate);

  return code;
}

//
//  initialize adpcm decoder handle
//
int32_t adpcm_decode_init(ADPCM_DECODE_HANDLE* adpcm, int32_t sample_rate, int16_t auto_clip) {

  int32_t rc = -1;

  adpcm->sample_rate = sample_rate;
  adpcm->auto_clip = auto_clip;
  //adpcm->half_bits = 0;

  adpcm->step_index = 0;
  adpcm->last_estimate = 0;

  adpcm->resample_counter = 0;

  rc = 0;

exit:
  return rc;
}

//
//  close adpcm decoder handle
//
void adpcm_decode_close(ADPCM_DECODE_HANDLE* adpcm) {

}

//
//  execute adpcm decoding
//
int32_t adpcm_decode_exec(ADPCM_DECODE_HANDLE* adpcm, void* output_buffer, uint8_t* source_buffer, size_t source_buffer_len) {

  size_t source_buffer_ofs = 0;
  size_t output_buffer_ofs = 0;

  int16_t* output_buffer_int16 = (int16_t*)output_buffer;

  if (adpcm->auto_clip) {

    while (source_buffer_ofs < source_buffer_len) {

      uint8_t code;
      if ((adpcm->resample_counter % 2) == 0) {
        code = source_buffer[ source_buffer_ofs ] & 0x0f;
      } else {
        code = (source_buffer[ source_buffer_ofs++ ] >> 4) & 0x0f;
      }
      adpcm->resample_counter++;

      int16_t step_index = adpcm->step_index;
      int16_t new_estimate = msm6258v_decode(code, &step_index, adpcm->last_estimate);
      output_buffer_int16[ output_buffer_ofs++ ] = new_estimate * 8;   // 12bit signed PCM to 15bit signed PCM
      adpcm->step_index = step_index;
      adpcm->last_estimate = new_estimate;

    }

  } else {

    while (source_buffer_ofs < source_buffer_len) {

      uint8_t code;
      if ((adpcm->resample_counter % 2) == 0) {
        code = source_buffer[ source_buffer_ofs ] & 0x0f;
      } else {
        code = (source_buffer[ source_buffer_ofs++ ] >> 4) & 0x0f;
      }
      adpcm->resample_counter++;

      int16_t step_index = adpcm->step_index;
      int16_t new_estimate = msm6258v_decode(code, &step_index, adpcm->last_estimate);
      output_buffer_int16[ output_buffer_ofs++ ] = new_estimate * 16;   // 12bit signed PCM to 16bit signed PCM
      adpcm->step_index = step_index;
      adpcm->last_estimate = new_estimate;

    }

  }

  return output_buffer_ofs * sizeof(int16_t);
}
