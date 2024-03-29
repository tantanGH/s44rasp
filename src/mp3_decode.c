#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "mp3_decode.h"

//
//  inline helper: 24bit signed int to 16bit signed int
//
static inline int16_t scale_16bit(mad_fixed_t sample) {
  // round
  sample += (1L << (MAD_F_FRACBITS - 16));

  // clip
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  // quantize
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

//
//  inline helper: 24bit signed int to 12bit signed int
//
static inline int16_t scale_12bit(mad_fixed_t sample) {
  // round
  sample += (1L << (MAD_F_FRACBITS - 12));

  // clip
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  // quantize
  return sample >> (MAD_F_FRACBITS + 1 - 12);
}

//
//  init mp3 decoder handle
//
int32_t mp3_decode_open(MP3_DECODE_HANDLE* decode, int16_t up_sampling) {

  // baseline
  decode->up_sampling = up_sampling;
  decode->skip_offset = 0;
  decode->mp3_data = NULL;
  decode->mp3_data_len = 0;
  decode->mp3_quality = 0;

  // ID3v2 tags
  decode->mp3_title = NULL;
  decode->mp3_artist = NULL;
  decode->mp3_album = NULL;

  // sampling parameters
  decode->sample_rate = -1;
  decode->channels = -1;
  decode->resample_counter = 0;

  // mad
  memset(&(decode->mad_stream), 0, sizeof(MAD_STREAM));
  memset(&(decode->mad_frame), 0, sizeof(MAD_FRAME));
  memset(&(decode->mad_synth), 0, sizeof(MAD_SYNTH));
  memset(&(decode->mad_timer), 0, sizeof(MAD_TIMER));

  decode->mp3_frame_options = 0;
  decode->current_mad_pcm = NULL;

  return 0;
}

//
//  close decoder handle
//
void mp3_decode_close(MP3_DECODE_HANDLE* decode) {

  mad_synth_finish(&(decode->mad_synth));
  mad_frame_finish(&(decode->mad_frame));
  mad_stream_finish(&(decode->mad_stream));

  if (decode->mp3_title != NULL) {
    free(decode->mp3_title);
  }
  if (decode->mp3_artist != NULL) {
    free(decode->mp3_artist);
  }
  if (decode->mp3_album != NULL) {
    free(decode->mp3_album);
  }

}

//
//  parse ID3v2 tags
//
int32_t mp3_decode_parse_header(MP3_DECODE_HANDLE* decode, FILE* fp) {

  int rc = -1;

  // read the first 10 bytes of the MP3 file
  uint8_t mp3_header[10];
  size_t ret = fread(mp3_header, 1, 10, fp);
  if (ret != 10) {
    printf("error: cannot read mp3 file.\n");
    goto exit;
  }

  // check if the MP3 file has an ID3v2 tag
  if (!(mp3_header[0] == 'I' && mp3_header[1] == 'D' && mp3_header[2] == '3')) {
    rc = 0;
    goto exit;
  }

  // extract the total tag size (syncsafe integer)
  uint32_t total_tag_size = ((mp3_header[6] & 0x7f) << 21) | ((mp3_header[7] & 0x7f) << 14) |
                            ((mp3_header[8] & 0x7f) << 7)  | (mp3_header[9] & 0x7f);

  // ID3v2 version
  int16_t id3v2_version = mp3_header[3];
  if (id3v2_version < 0x03) {
    decode->skip_offset = total_tag_size + 10;     // does not support ID3v2.2 or before
    rc = 0;
    goto exit;
  }

  // skip extended ID3v2 header
  if (mp3_header[5] & (1<<6)) {
    uint8_t ext_header[6];
    fread(ext_header, 1, 6, fp);
    uint32_t ext_header_size = id3v2_version == 0x03 ? *((uint32_t*)(ext_header + 0)) :
                                ((ext_header[0] & 0x7f) << 21) | ((ext_header[1] & 0x7f) << 14) |
                                ((ext_header[2] & 0x7f) << 7)  | (ext_header[3] & 0x7f);
    fseek(fp, ext_header_size, SEEK_CUR);
    total_tag_size -= 6 + ext_header_size;
  }

  uint8_t frame_header[10];
  int32_t ofs = 0;

  //printf("total tag size = %d\n",total_tag_size);

  while (ofs < total_tag_size) {

    fread(frame_header, 1, 10, fp);

    uint32_t frame_size = (id3v2_version == 0x03) ? *((uint32_t*)(frame_header + 4)) :
                            ((frame_header[4] & 0x7f) << 21) | ((frame_header[5] & 0x7f) << 14) |
                            ((frame_header[6] & 0x7f) << 7)  |  (frame_header[7] & 0x7f);    

    if (memcmp(frame_header, "0000", 4) < 0 || memcmp(frame_header, "ZZZZ", 4) > 0) {

      break;
/*
    } else if (memcmp(frame_header, "TIT2", 4) == 0) {

      // title
      uint8_t* frame_data = malloc(frame_size);
      fread(frame_data, 1, frame_size, fp);
      if (frame_data[0] == 0x00) {              // ISO-8859-1
        decode->mp3_title = frame_data + 1;
      } else if (frame_data[0] == 0x01) {       // UTF-16 with BOM
        decode->mp3_title = malloc(frame_size - 3 + 1);
        decode->mp3_title[0] = '\0';
        convert_utf16_to_cp932(decode->mp3_title, frame_data + 1, frame_size - 1);
      }   
      free(frame_data);

    } else if (memcmp(frame_header, "TPE1", 4) == 0) {

      // artist
      uint8_t* frame_data = malloc(frame_size);
      fread(frame_data, 1, frame_size, fp);

      if (frame_data[0] == 0x00) {              // ISO-8859-1
        decode->mp3_artist = frame_data + 1;
      } else if (frame_data[0] == 0x01) {       // UTF-16 with BOM
        decode->mp3_artist = malloc(frame_size - 3 + 1);
        decode->mp3_artist[0] = '\0';
        convert_utf16_to_cp932(decode->mp3_artist, frame_data + 1, frame_size - 1);
      }
      free(frame_data);   

    } else if (memcmp(frame_header, "TALB", 4) == 0) {

      // album
      uint8_t* frame_data = malloc(frame_size);
      fread(frame_data, 1, frame_size, fp);

      if (frame_data[0] == 0x00) {              // ISO-8859-1
        decode->mp3_album = frame_data + 1;
      } else if (frame_data[0] == 0x01) {       // UTF-16 with BOM
        decode->mp3_album = malloc(frame_size - 3 + 1);
        decode->mp3_album[0] = '\0';
        convert_utf16_to_cp932(decode->mp3_album, frame_data + 1, frame_size - 1);
      }
      free(frame_data);
*/
    } else {
      // other tags
      fseek(fp, frame_size, SEEK_CUR);
    }

    ofs += 10 + frame_size;

  }

  decode->skip_offset = 10 + total_tag_size;
  rc = 0;

exit:
  return rc;
}

//
//  setup decode operation
//
int32_t mp3_decode_setup(MP3_DECODE_HANDLE* decode, void* mp3_data, size_t mp3_data_len, int16_t mp3_quality) {

  // baseline
  decode->mp3_data = mp3_data;
  decode->mp3_data_len = mp3_data_len;
  decode->mp3_quality = mp3_quality;

  // sampling parameters
  decode->sample_rate = -1;
  decode->channels = -1;
  decode->resample_counter = 0;

  // mad frame options
  decode->mp3_frame_options = 
    decode->mp3_quality == 1 ? MAD_OPTION_HALFSAMPLERATE    | MAD_OPTION_IGNORECRC : 0;

  mad_stream_init(&(decode->mad_stream));
  mad_frame_init(&(decode->mad_frame));
  mad_synth_init(&(decode->mad_synth));
  mad_timer_reset(&(decode->mad_timer));

  mad_stream_buffer(&(decode->mad_stream), mp3_data, mp3_data_len);

  //decode->current_mad_pcm = NULL;

  // decode 1st frame
  mad_frame_decode(&(decode->mad_frame), &(decode->mad_stream));

  decode->mad_frame.options = decode->mp3_frame_options;

  mad_synth_frame(&(decode->mad_synth), &(decode->mad_frame));
  mad_timer_add(&(decode->mad_timer), decode->mad_frame.header.duration);

  decode->current_mad_pcm = &(decode->mad_synth.pcm);

  decode->sample_rate = decode->current_mad_pcm->samplerate;
  decode->channels = decode->current_mad_pcm->channels;

  return 0;
}

//
//  decode MP3 stream
//
size_t mp3_decode_exec(MP3_DECODE_HANDLE* decode, int16_t* output_buffer, size_t output_buffer_bytes) {

  // decode counter
  int32_t decode_ofs = 0;

  for (;;) {
    
    if (decode->current_mad_pcm == NULL) {

      int16_t result = mad_frame_decode(&(decode->mad_frame), &(decode->mad_stream));
      if (result == -1) {
        if (decode->mad_stream.error == MAD_ERROR_BUFLEN) {
          // MP3 EOF
          break;
        } else if (MAD_RECOVERABLE(decode->mad_stream.error)) {
          continue;
        } else {
//          printf("error: %s\n", mad_stream_errorstr(&(decode->mad_stream)));
          goto exit;
        }
      }

      decode->mad_frame.options = decode->mp3_frame_options;

      mad_synth_frame(&(decode->mad_synth), &(decode->mad_frame));
      mad_timer_add(&(decode->mad_timer), decode->mad_frame.header.duration);

      decode->current_mad_pcm = &(decode->mad_synth.pcm);

      if (decode->sample_rate < 0) {
        decode->sample_rate = decode->current_mad_pcm->samplerate;
        decode->channels = decode->current_mad_pcm->channels;
      }

    } 

    MAD_PCM* pcm = decode->current_mad_pcm;
    if (decode_ofs * sizeof(int16_t) + ( pcm->length * 2 * pcm->channels ) > output_buffer_bytes) {
      // no more buffer space to write
      break;
    }

    if (pcm->channels == 2) {

      for (int32_t i = 0; i < pcm->length; i++) {
        // stereo data
        output_buffer[ decode_ofs++ ] = scale_16bit(pcm->samples[0][i]);
        output_buffer[ decode_ofs++ ] = scale_16bit(pcm->samples[1][i]);
      }

    } else {

      for (int32_t i = 0; i < pcm->length; i++) {
        // mono data
        output_buffer[ decode_ofs++ ] = scale_16bit(pcm->samples[0][i]);
      }

    }

    decode->current_mad_pcm = NULL;

  }

  // success

exit:

  // push resampled count
  return decode_ofs;
}
