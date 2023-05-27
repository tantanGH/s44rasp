#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "raw_decode.h"

//
//  init raw pcm decoder handle
//
int32_t raw_decode_open(RAW_DECODE_HANDLE* pcm, int32_t sample_rate, int16_t channels) {

  int32_t rc = -1;

  // baseline
  pcm->sample_rate = sample_rate;
  pcm->channels = channels;
 
  rc = 0;

exit:
  return rc;
}

//
//  close decoder handle
//
void raw_decode_close(RAW_DECODE_HANDLE* pcm) {

}

//
//  decode
//
size_t raw_decode_exec(RAW_DECODE_HANDLE* pcm, int16_t* output_buffer, int16_t* source_buffer, size_t source_buffer_len) {

  uint8_t* source_buffer_uint8 = (uint8_t*)source_buffer;
  uint8_t* output_buffer_uint8 = (uint8_t*)output_buffer;
  size_t source_buffer_ofs = 0;
  size_t output_buffer_ofs = 0;
  size_t output_buffer_len = 0;

  if (pcm->channels == 1) {

    // mono to stereo duplication with endian conversion
    while (source_buffer_ofs < source_buffer_len) {
      output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
      output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
      output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
      output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
      source_buffer_ofs ++;
    }

    output_buffer_len = source_buffer_len * 2;

  } else {

    // endian converion
    while (source_buffer_ofs < source_buffer_len) {
      output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
      output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
      source_buffer_ofs ++;
    }

    output_buffer_len = source_buffer_len;

  }

  return output_buffer_len;
}
