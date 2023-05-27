#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// alsa
#include <alsa/asoundlib.h>

// codec
#include "adpcm_decode.h"
#include "raw_decode.h"
#include "wav_decode.h"

// application
#include "s44rasp.h"

static void show_help_message() {
  printf("usage: s44rasp [options] <input-file[.pcm|.sXX|.mXX|.aXX|.nXX|.wav]>\n");
  printf("options:\n");
  printf("     -d <device>  ... ALSA PCM device name (i.e. hw:3,0)\n");
  printf("     -l <latency> ... ALSA PCM latency in msec (default:100ms)\n");
//  printf("     -f           ... supported format check\n");
  printf("     -h           ... show help message\n");
}

int32_t main(int32_t argc, uint8_t* argv[]) {

  // default exit code
  int32_t rc = -1;

  // pcm attribs
  snd_pcm_t* pcm_handle = NULL;
  snd_pcm_hw_params_t* pcm_params = NULL;
  snd_pcm_uframes_t num_frames = 0;
  int16_t* pcm_buffer = NULL; 
  uint8_t* pcm_file_name = NULL;
  uint8_t* pcm_device_name = NULL;
  uint32_t pcm_latency = 100;
//  int16_t pcm_format_check = 0;
  int32_t alsa_rc = 0;
  FILE* fp = NULL;

  // decoders
  ADPCM_DECODE_HANDLE adpcm_decoder = { 0 };
  RAW_DECODE_HANDLE raw_decoder = { 0 };
  WAV_DECODE_HANDLE wav_decoder = { 0 };
//  YM2608_DECODE_HANDLE ym2608_decoder = { 0 };

  printf("s44rasp - X68k ADPCM/PCM/WAV player for Raspberry Pi version " PROGRAM_VERSION " by tantan\n");

  for (int16_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && strlen(argv[i]) >= 2) {
      if (argv[i][1] == 'd' && i+1 < argc) {
        pcm_device_name = argv[ i + 1 ];
        i++;
      } else if (argv[i][1] == 'l' && i+1 < argc) {
        pcm_latency = atoi(argv[ i + 1 ]);
        i++;
//      } else if (argv[i][1] == 'f') {
//        pcm_format_check = 1;
      } else if (argv[i][1] == 'h') {
        show_help_message();
        goto exit;
      } else {
        printf("error: unknown option (%s).\n",argv[i]);
        goto exit;
      }
    } else {
      if (pcm_file_name != NULL) {
        printf("error: multiple files are not supported.\n");
        goto exit;
      }
      pcm_file_name = argv[i];
    }
  }

/*
  if (pcm_format_check) {
    snd_pcm_hw_params_t* params;
    snd_pcm_format_mask_t* format_mask;
    snd_pcm_open(&pcm_handle, pcm_device_name, SND_PCM_STREAM_PLAYBACK, 0);
    
    // Allocate and initialize the hardware parameters
    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    
    // Retrieve the format mask
    snd_pcm_format_mask_malloc(&format_mask);
    snd_pcm_hw_params_get_format_mask(params, format_mask);
    
    // Iterate through the possible formats and check support
    for (int32_t format = SND_PCM_FORMAT_S8; format <= SND_PCM_FORMAT_FLOAT64; format++) {
      if (snd_pcm_format_mask_test(format_mask, format)) {
        printf("Format %s is supported\n", snd_pcm_format_name(format));
      }
    }
    
    // Cleanup and close the PCM device
    snd_pcm_format_mask_free(format_mask);
    snd_pcm_hw_params_free(params);
    snd_pcm_close(pcm_handle);
    pcm_handle = NULL;
    goto exit;
  }
*/

  if (pcm_file_name == NULL) {
    show_help_message();
    goto exit;
  }

  // input format check
  uint8_t* pcm_file_exp = pcm_file_name + strlen(pcm_file_name) - 4;
  int16_t input_format = FORMAT_ADPCM;
  int32_t pcm_freq = 15625;
  int16_t pcm_channels = 1;
  if (stricmp(".pcm", pcm_file_exp) == 0) {
    input_format = FORMAT_ADPCM;
    pcm_freq = 15625;                 // fixed
    pcm_channels = 1;
  } else if (stricmp(".s32", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 32000;
    pcm_channels = 2;
  } else if (stricmp(".s44", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 44100;
    pcm_channels = 2;
  } else if (stricmp(".s48", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 48000;
    pcm_channels = 2;
  } else if (stricmp(".m32", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 32000;
    pcm_channels = 1;
  } else if (stricmp(".m44", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 44100;
    pcm_channels = 1;
  } else if (stricmp(".m48", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 48000;
    pcm_channels = 1;
  } else if (stricmp(".a32", pcm_file_exp) == 0) {
    input_format = FORMAT_YM2608;
    pcm_freq = 32000;
    pcm_channels = 2;
  } else if (stricmp(".a44", pcm_file_exp) == 0) {
    input_format = FORMAT_YM2608;
    pcm_freq = 44100;
    pcm_channels = 2;
  } else if (stricmp(".a48", pcm_file_exp) == 0) {
    input_format = FORMAT_YM2608;
    pcm_freq = 48000;
    pcm_channels = 2;
  } else if (stricmp(".n32", pcm_file_exp) == 0) {
    input_format = FORMAT_YM2608;
    pcm_freq = 32000;
    pcm_channels = 1;
  } else if (stricmp(".n44", pcm_file_exp) == 0) {
    input_format = FORMAT_YM2608;
    pcm_freq = 44100;
    pcm_channels = 1;
  } else if (stricmp(".n48", pcm_file_exp) == 0) {
    input_format = FORMAT_YM2608;
    pcm_freq = 48000;
    pcm_channels = 1;
  } else if (stricmp(".wav", pcm_file_exp) == 0) {
    input_format = FORMAT_WAV;
    pcm_freq = -1;        // not yet determined
    pcm_channels = -1;    // not yet determined
  } else {
    printf("error: unknown format file (%s).\n", pcm_file_name);
    goto exit;
  }

  // init adpcm (msm6258v) decoder
  if (input_format == FORMAT_ADPCM) {
    if (adpcm_decode_init(&adpcm_decoder) != 0) {
      printf("error: ADPCM encoder initialization error.\n");
      goto exit;
    }
  }

  // init raw pcm decoder if needed
  if (input_format == FORMAT_RAW) {
    if (raw_decode_init(&raw_decoder, pcm_freq, pcm_channels) != 0) {
      printf("error: PCM decoder initialization error.\n");
      goto exit;
    }
  }

  // init wav decoder if needed
  if (input_format == FORMAT_WAV) {
    if (wav_decode_init(&wav_decoder) != 0) {
      printf("error: WAV decoder initialization error.\n");
      goto exit;
    }
  }

  // init adpcm (ym2608) decoder if needed
//  if (input_format == FORMAT_YM2608) {
//    if (ym2608_decode_init(&ym2608_decoder, pcm_freq * pcm_channels * 4, pcm_freq, pcm_channels) != 0) {
//      printf("error: YM2608 adpcm decoder initialization error.\n");
//      goto exit;
//    }
//  }

  // init ALSA device
  if ((alsa_rc = snd_pcm_open(&pcm_handle, pcm_device_name != NULL ? pcm_device_name : (uint8_t*)"default", 
                    SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) != 0) {
    printf("error: pcm device (%s) open error. (%s)\n", pcm_device_name, snd_strerror(alsa_rc));
    goto exit;
  }

  // set ALSA PCM parameters
  snd_pcm_hw_params_alloca(&pcm_params);
  snd_pcm_hw_params_any(pcm_handle, params);
  snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_channels(pcm_handle, params, pcm_channels);
  snd_pcm_hw_params_set_rate(pcm_handle, params, pcm_freq, 0);
  snd_pcm_hw_params(pcm_handle, params);
//  if ((alsa_rc = snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, 
//                          pcm_channels, pcm_freq, ALSA_SOFT_RESAMPLE, pcm_latency)) != 0) {
//    printf("error: pcm device setting error. (%s)\n", snd_strerror(alsa_rc));
//    goto exit;
//  }

  /* Allocate buffer to hold single period */
  snd_pcm_hw_params_get_period_size(params, &num_frames, NULL);
  fprintf(stderr,"# frames in a period: %d\n", num_frames);

  size_t pcm_buffer_len = num_frames * pcm_channels * sizeof(int16_t); //pcm_freq * pcm_channels * 2;
  pcm_buffer = malloc(sizeof(int16_t) * pcm_buffer_len);
  fp = fopen(pcm_file_name, "rb");
  if (fp == NULL) {
    printf("error: s44 file open error.\n");
    goto exit;
  }

  // read header part of WAV file
  size_t skip_offset = 0;
  if (input_format == FORMAT_WAV) {
    int32_t ofs = wav_decode_parse_header(&wav_decoder, fp);
    if (ofs < 0) {
      //printf("error: wav header parse error.\n");
      goto catch;
    }
    pcm_freq = wav_decoder.sample_rate;
    pcm_channels = wav_decoder.channels;
    skip_offset = ofs;
  }

  // check data content size
  fseek(fp, 0, SEEK_END);
  size_t pcm_data_size = ftell(fp) - skip_offset;
  fseek(fp, skip_offset, SEEK_SET);

  // describe PCM file information
  printf("File name     : %s\n", pcm_file_name);
  printf("Data size     : %d [bytes]\n", pcm_data_size);
  printf("Data format   : %s\n", 
    input_format == FORMAT_WAV ? "WAV" :
    input_format == FORMAT_YM2608 ? "ADPCM(YM2608)" :
    input_format == FORMAT_RAW ? "16bit signed raw PCM (big)" : 
    "ADPCM(MSM6258V)");

  // describe playback drivers
  printf("PCM driver    : %s\n",
    "ALSA");

  if (input_format == FORMAT_ADPCM) {
    float pcm_1sec_size = pcm_freq * 0.5;
    printf("PCM frequency : %d [Hz]\n", pcm_freq);
    printf("PCM channels  : %s\n", "mono");
    printf("PCM length    : %4.2f [sec]\n", (float)pcm_data_size / pcm_1sec_size);
  }

  if (input_format == FORMAT_RAW) {
    float pcm_1sec_size = pcm_freq * 2;
    printf("PCM frequency : %d [Hz]\n", pcm_freq);
    printf("PCM channels  : %s\n", pcm_channels == 1 ? "mono" : "stereo");
    printf("PCM length    : %4.2f [sec]\n", (float)pcm_data_size / pcm_channels / pcm_1sec_size);
  }

  if (input_format == FORMAT_YM2608) {
    float pcm_1sec_size = pcm_freq * 0.5;
    printf("PCM frequency : %d [Hz]\n", pcm_freq);
    printf("PCM channels  : %s\n", pcm_channels == 1 ? "mono" : "stereo");
    printf("PCM length    : %4.2f [sec]\n", (float)pcm_data_size / pcm_channels / pcm_1sec_size);
  }

  if (input_format == FORMAT_WAV) {
    printf("PCM frequency : %d [Hz]\n", pcm_freq);
    printf("PCM channels  : %s\n", pcm_channels == 1 ? "mono" : "stereo");
    printf("PCM length    : %4.2f [sec]\n", (float)wav_decoder.duration / pcm_freq);
  }

  size_t fread_len = 0;
  size_t fread_buffer_len = 1024;
  do {
    size_t len = fread(pcm_buffer, sizeof(int16_t), fread_buffer_len, fp);
    if (len <= 0) break;
    fread_len += len;
    uint8_t* pcm_buffer_uint8 = (uint8_t*)pcm_buffer;
    for (size_t i = 0; i < len; i++) {
      uint8_t c = pcm_buffer_uint8[ i * 2 + 0 ];
      pcm_buffer_uint8[ i * 2 + 0 ] = pcm_buffer_uint8[ i * 2 + 1 ]; 
      pcm_buffer_uint8[ i * 2 + 1 ] = c;
    }
    num_frames = len / pcm_channels;
    if ((alsa_rc = snd_pcm_writei(pcm_handle, (const void*)pcm_buffer, num_frames)) < 0) {    
      if (snd_pcm_recover(pcm_handle, alsa_rc, 0) < 0) {
        printf("error: fatal pcm data write error.\n");
        goto exit;
      }
    }
    printf(".\n");
  } while (fread_len * sizeof(int16_t) < pcm_data_size);

  fclose(fp);
  fp = NULL;

  if ((alsa_rc = snd_pcm_drain(pcm_handle)) != 0) {
    printf("error: pcm drain error. (%s)\n", snd_strerror(alsa_rc));
    goto exit;
  }

  getchar();

  rc = 0;

catch:


exit:

  if (fp != NULL) {
    fclose(fp);
    fp = NULL;
  }

  if (pcm_handle != NULL) {
    snd_pcm_close(pcm_handle);
    pcm_handle = NULL;
  }

  if (pcm_buffer != NULL) {
    free(pcm_buffer);
    pcm_buffer = NULL;
  }

  
  return rc;
}
