#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "raw_decode.h"

//
//  init raw pcm decoder handle
//
int32_t raw_decode_open(RAW_DECODE_HANDLE* pcm, int32_t sample_rate, int16_t channels, int16_t use_24bit) {

  int32_t rc = -1;

  // baseline
  pcm->sample_rate = sample_rate;
  pcm->channels = channels;

  // for resampling
  pcm->resample_rate = 48000;
  pcm->resample_counter = 0;

  // workaround for PCM51xx
  pcm->use_24bit = use_24bit;

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
size_t raw_decode_exec(RAW_DECODE_HANDLE* pcm, void* output_buffer, int16_t* source_buffer, size_t source_buffer_len) {

  uint8_t* source_buffer_uint8 = (uint8_t*)source_buffer;
  uint8_t* output_buffer_uint8 = (uint8_t*)output_buffer;
  size_t source_buffer_ofs = 0;
  size_t output_buffer_ofs = 0;
  size_t output_buffer_len = 0;

  if (pcm->sample_rate == 32000) {

    if (pcm->channels == 1) {

      // mono to stereo duplication with endian conversion
      while (source_buffer_ofs < source_buffer_len) {

        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];

        // up sampling
        pcm->resample_counter += pcm->sample_rate;
        if (pcm->resample_counter < pcm->resample_rate) {
          // do not increment
        } else {
          source_buffer_ofs ++;
          pcm->resample_counter -= pcm->resample_rate;
        }

      }

      output_buffer_len = output_buffer_ofs / 2;

    } else {

      // endian converion
      while (source_buffer_ofs < source_buffer_len) {

        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];

        // up sampling
        pcm->resample_counter += pcm->sample_rate;
        if (pcm->resample_counter < pcm->resample_rate) {
          // do not increment
        } else {
          source_buffer_ofs ++;
          pcm->resample_counter -= pcm->resample_rate;
        }

      }

      output_buffer_len = output_buffer_ofs / 2;

    }

  } else if (pcm->sample_rate == 44100) {

    if (pcm->use_24bit) {

      if (pcm->channels == 1) {

        // mono to stereo duplication with endian conversion
        while (source_buffer_ofs < source_buffer_len) {
          output_buffer_uint8[ output_buffer_ofs ++ ] = 0;
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
          output_buffer_uint8[ output_buffer_ofs ++ ] = 0;
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
          source_buffer_ofs ++;
        }

        output_buffer_len = output_buffer_ofs / 2;

      } else {

        // endian converion
        while (source_buffer_ofs < source_buffer_len) {
          output_buffer_uint8[ output_buffer_ofs ++ ] = 0;
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
          source_buffer_ofs ++;
        }

        output_buffer_len = output_buffer_ofs / 2;

      }

    } else {

      if (pcm->channels == 1) {

        // mono to stereo duplication with endian conversion
        while (source_buffer_ofs < source_buffer_len) {
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
          source_buffer_ofs ++;
        }

        output_buffer_len = output_buffer_ofs / 2;

      } else {

        // endian converion
        while (source_buffer_ofs < source_buffer_len) {
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
          output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
          source_buffer_ofs ++;
        }

        output_buffer_len = output_buffer_ofs / 2;

      }

    }

  } else {

    if (pcm->channels == 1) {

      // mono to stereo duplication with endian conversion
      while (source_buffer_ofs < source_buffer_len) {
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
        source_buffer_ofs ++;
      }

      output_buffer_len = output_buffer_ofs / 2;

    } else {

      // endian converion
      while (source_buffer_ofs < source_buffer_len) {
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs * 2 + 0 ];
        source_buffer_ofs ++;
      }

      output_buffer_len = output_buffer_ofs / 2;

    }

  }

  return output_buffer_len;
}
