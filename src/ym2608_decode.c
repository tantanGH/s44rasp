#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "ym2608_decode.h"

/*
//
//  YM2608 ADPCM decode
//
static inline int16_t ym2608_decode(uint8_t code, uint32_t* step_size, int16_t last_data) {

  static const int16_t step_table[] = { 57, 57, 57, 57, 77, 102, 128, 153 };

  int16_t delta = code & 0x07;
  uint32_t ss = *step_size;
  int32_t diff = ss * ( 1 + ( delta * 2 )) / 8;

  int32_t estimate = (code & 0x08) ? last_data - diff : last_data + diff;
  estimate = (estimate > 32767) ? 32767 : (estimate < -32768) ? -32768 : estimate;
 
  uint32_t next_step = ss * step_table[ delta ] / 64;
  *step_size = (next_step < 127) ? 127 : (next_step > 24576) ? 24576 : next_step;

  return estimate;
}
*/

//
//  initialize ym2608 decoder handle
//
int32_t ym2608_decode_open(YM2608_DECODE_HANDLE* ym2608, int32_t sample_rate, int16_t channels, int16_t up_sampling) {

  int32_t rc = -1;

  ym2608->sample_rate = sample_rate;
  ym2608->channels = channels;

//  ym2608->step_size = 127;
//  ym2608->step_index = 0;
//  ym2608->last_estimate = 0;

//  ym2608->step_size2 = 127;
//  ym2608->step_index2 = 0;
//  ym2608->last_estimate2 = 0;

//  ym2608->adpcm_counter = 0;

  ym2608->resample_rate = 48000;
  ym2608->resample_counter = 0;

  ym2608->up_sampling = up_sampling;

  ym2608->x1 = ym2608_conv_table;
  ym2608->x2 = ym2608_conv_table;

  ym2608->last_estimate1 = 0;
  ym2608->last_estimate2 = 0;

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

  if (ym2608->sample_rate < 44100 || ym2608->up_sampling) {
/*
    if (ym2608->channels == 1) {
    
      while (source_buffer_ofs < source_buffer_len) {

        ym2608->resample_counter += ym2608->sample_rate;
        if (ym2608->resample_counter < ym2608->resample_rate) {

          output_buffer[ output_buffer_ofs ++ ] = ym2608->last_estimate;
          output_buffer[ output_buffer_ofs ++ ] = ym2608->last_estimate;

        } else {

          uint8_t code;
          if ((ym2608->adpcm_counter % 2) == 0) {
            code = source_buffer[ source_buffer_ofs ] & 0x0f;
          } else {
            code = (source_buffer[ source_buffer_ofs++ ] >> 4) & 0x0f;
          }
          ym2608->adpcm_counter++;

          uint32_t step_size = ym2608->step_size;
          int32_t new_estimate = ym2608_decode(code, &step_size, ym2608->last_estimate);
          output_buffer[ output_buffer_ofs ++ ] = new_estimate;
          output_buffer[ output_buffer_ofs ++ ] = new_estimate;   // mono to stereo duplication
          ym2608->step_size = step_size;
          ym2608->last_estimate = new_estimate;

          ym2608->resample_counter -= ym2608->resample_rate;

        }

      }

    } else {

    }
*/
  } else {

    if (ym2608->channels == 1) {

      uint8_t* a0 = ym2608->x1;
      int16_t back = ym2608->last_estimate1;

      while (source_buffer_ofs < source_buffer_len) {

        //printf("conv_table=%x, a0=%x, back=%x\n", ym2608_conv_table, a0, back);

        int16_t d3 = 0;
        d3 += source_buffer[ source_buffer_ofs ++ ];
        d3 *= 8;
        a0 += d3;

        int16_t delta1;
        ((uint8_t*)(&delta1))[0] = a0[1];
        ((uint8_t*)(&delta1))[1] = a0[0];
        back += delta1;
        output_buffer[ output_buffer_ofs ++ ] = back;
        output_buffer[ output_buffer_ofs ++ ] = back;

        int16_t delta2;
        ((uint8_t*)(&delta2))[0] = a0[3];
        ((uint8_t*)(&delta2))[1] = a0[2];
        back += delta2;
        output_buffer[ output_buffer_ofs ++ ] = back;
        output_buffer[ output_buffer_ofs ++ ] = back;

        int32_t ofs;
        ((uint8_t*)(&ofs))[0] = a0[7];
        ((uint8_t*)(&ofs))[1] = a0[6]; 
        ((uint8_t*)(&ofs))[2] = a0[5];
        ((uint8_t*)(&ofs))[3] = a0[4];     
        a0 += 4 + ofs;

      }

      ym2608->last_estimate1 = back;
      ym2608->x1 = a0;

    } else {


    }

  }

  return output_buffer_ofs;
}
