#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "macs_decode.h"

//
//  init macs decoder handle
//
int32_t macs_decode_open(MACS_DECODE_HANDLE* macs, int16_t up_sampling) {

  int32_t rc = -1;

  macs->up_sampling = up_sampling;
  macs->resample_rate = 48000;
  macs->resample_counter = 0;

  macs->sample_rate = -1;
  macs->channels = -1;
  macs->duration = -1;

  rc = 0;

//exit:
  return rc;
}

//
//  close decoder handle
//
void macs_decode_close(MACS_DECODE_HANDLE* macs) {

}

//
//  parse macs header
//
int32_t macs_decode_parse_header(MACS_DECODE_HANDLE* macs, FILE* fp) {

  int32_t rc = -1;

  // eye catch
  uint8_t buf[512];
  size_t bytes_read = fread(buf, sizeof(uint8_t), 512, fp);
  if (memcmp(buf + 0, "MACSDATA", 8) != 0) {
    printf("error: not MACS data. (%s)\n", buf);
    goto exit;
  }
  if (buf[ 0x0e ] != 0x00 || buf[ 0x0f ] != 0x01) {
    printf("error: unsupported MACS format.\n");
    goto exit;
  }

  size_t read_ofs = 0x10;
  while (read_ofs < bytes_read - 20) {
    if (buf[ read_ofs +  0 ] == 0x00 && buf[ read_ofs +  1 ] == 0x18 &&
        buf[ read_ofs + 10 ] == 0x00 && buf[ read_ofs + 11 ] == 0x00 &&
        buf[ read_ofs + 12 ] == 0x00 && buf[ read_ofs + 13 ] == 0x08 &&
        buf[ read_ofs + 15 ] == 0x03 &&
        buf[ read_ofs + 16 ] == 0x00 && buf[ read_ofs + 17 ] == 0x00 &&
        buf[ read_ofs + 18 ] == 0x00 && buf[ read_ofs + 19 ] == 0x00) {
      if (buf[ read_ofs + 14 ] == 0x1a) {
        macs->sample_rate = 22050;
        macs->channels = 2;
      } else if (buf[ read_ofs + 14 ] == 0x1d) {
        macs->sample_rate = 44100;
        macs->channels = 2;
      } else if (buf[ read_ofs + 14 ] == 0x1e) {
        macs->sample_rate = 48000;
        macs->channels = 2;
      } else {
        printf("error: unsupported MACS audio format.\n");
        goto exit;
      }
      macs->skip_offset = buf[ read_ofs + 2 ] * 256 * 256 * 256 +
                          buf[ read_ofs + 3 ] * 256 * 256 +
                          buf[ read_ofs + 4 ] * 256 +
                          buf[ read_ofs + 5 ] + 0x0e;
      macs->total_bytes = buf[ read_ofs + 6 ] * 256 * 256 * 256 +
                          buf[ read_ofs + 7 ] * 256 * 256 +
                          buf[ read_ofs + 8 ] * 256 +
                          buf[ read_ofs + 9 ];
      macs->duration = macs->sample_rate * macs->channels * sizeof(int16_t);
      rc = 0;
      break;
    }
    read_ofs ++;
  }


exit:
  return rc;
}

//
//  decode
//
size_t macs_decode_exec(MACS_DECODE_HANDLE* macs, int16_t* output_buffer, int16_t* source_buffer, size_t source_buffer_len) {

  uint8_t* source_buffer_uint8 = (uint8_t*)source_buffer;
  uint8_t* output_buffer_uint8 = (uint8_t*)output_buffer;
  size_t source_buffer_ofs = 0;
  size_t output_buffer_ofs = 0;
  size_t output_buffer_len = 0;

  if (macs->sample_rate < 44100 || macs->up_sampling) {

    if (macs->channels == 1) {

      // mono to stereo duplication with endian conversion
      while (source_buffer_ofs < source_buffer_len * 2) {

        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 0 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 0 ];

        // up sampling
        macs->resample_counter += macs->sample_rate;
        if (macs->resample_counter < macs->resample_rate) {
          // do not increment
        } else {
          source_buffer_ofs += 2;
          macs->resample_counter -= macs->resample_rate;
        }

      }

      output_buffer_len = output_buffer_ofs / 2;

    } else {

      // endian converion
      while (source_buffer_ofs < source_buffer_len * 2) {

        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 0 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 3 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 2 ];

        // up sampling
        macs->resample_counter += macs->sample_rate;
        if (macs->resample_counter < macs->resample_rate) {
          // do not increment
        } else {
          source_buffer_ofs += 4;
          macs->resample_counter -= macs->resample_rate;
        }

      }

      output_buffer_len = output_buffer_ofs / 2;

    }

  } else {

    if (macs->channels == 1) {

      // mono to stereo duplication with endian conversion
      while (source_buffer_ofs < source_buffer_len * 2) {
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 0 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 0 ];
        source_buffer_ofs += 2;
      }

      output_buffer_len = output_buffer_ofs / 2;

    } else {

      // endian converion
      while (source_buffer_ofs < source_buffer_len * 2) {
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 1 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 0 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 3 ];
        output_buffer_uint8[ output_buffer_ofs ++ ] = source_buffer_uint8[ source_buffer_ofs + 2 ];
        source_buffer_ofs += 4;
      }

      output_buffer_len = output_buffer_ofs / 2;

    }

  }

  return output_buffer_len;
}

