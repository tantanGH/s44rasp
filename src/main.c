#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

// alsa
#include <alsa/asoundlib.h>

// codec
#include "adpcm_decode.h"
#include "raw_decode.h"
#include "wav_decode.h"
#include "ym2608_decode.h"
#include "mp3_decode.h"
#include "macs_decode.h"

// oled
#include "oled_ssd1306.h"

// application
#include "s44rasp.h"

// abort control
static int16_t abort_flag = 0;

//
//  sigint handler
//
static void sigint_handler(int signal) {
  if (signal == SIGINT) {
    abort_flag = 1;
  }
}

//
//  show help message
//
static void show_help_message() {
  printf("usage: s44rasp [options] <input-file.(pcm|sXX|mXX|aXX|nXX|wav|mp3|mcs)>\n");
  printf("options:\n");
  printf("     -d hw:x,y ... ALSA PCM device name (i.e. hw:3,0)\n");
  printf("     -o        ... enable OLED(SSD1306) display\n");
  printf("     -u        ... upsample to 48kHz (default for 15.6kHz/24kHz/32kHz source)\n");
  printf("     -f        ... (not for play) check supported ALSA format\n");
  printf("     -l        ... (not for play) check peak/average level\n");
  printf("     -h        ... show help message\n");
}

//
//  main
//
int32_t main(int32_t argc, char* argv[]) {

  // default exit code
  int32_t rc = -1;

  // alsa
  snd_pcm_t* pcm_handle = NULL;
  uint32_t alsa_latency = ALSA_LATENCY;
  int32_t alsa_rc = 0;

  // pcm attribs
  int16_t* pcm_buffer = NULL; 
  void* fread_buffer = NULL;
  void* mp3_read_buffer = NULL;
  char* pcm_file_name = NULL;
  char* pcm_device_name = NULL;
  int16_t up_sampling = 0;          // 0:no upsampling, 1:to 48kHz, 2:to 44.1kHz
  int16_t pcm_format_check = 0;
  int16_t pcm_level_check = 0;
  int16_t quiet_mode = 0;

  // input file handle
  FILE* fp = NULL;

  // decoders
  ADPCM_DECODE_HANDLE adpcm_decoder = { 0 };
  RAW_DECODE_HANDLE raw_decoder = { 0 };
  WAV_DECODE_HANDLE wav_decoder = { 0 };
  YM2608_DECODE_HANDLE ym2608_decoder = { 0 };
  MP3_DECODE_HANDLE mp3_decoder = { 0 };
  MACS_DECODE_HANDLE macs_decoder = { 0 };

  // oled
  int16_t use_oled = 0;
  OLED_SSD1306 ssd1306 = { 0 };

  // credit
  printf("s44rasp - ADPCM/S44/A44/WAV/MP3/MCS player version " PROGRAM_VERSION " by tantan\n");

  // command line
  for (int16_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && strlen(argv[i]) >= 2) {
      if (argv[i][1] == 'd' && i+1 < argc) {
        pcm_device_name = argv[ i + 1 ];
        i++;
//      } else if (argv[i][1] == 'l' && i+1 < argc) {
//        alsa_latency = atoi(argv[ i + 1 ]);
//        i++;
      } else if (argv[i][1] == 'f') {
        pcm_format_check = 1;
      } else if (argv[i][1] == 'l') {
        pcm_level_check = 1;
      } else if (argv[i][1] == 'o') {
        use_oled = 1;
      } else if (argv[i][1] == 'u') {
        up_sampling = 1;
      } else if (argv[i][1] == 'q') {
        quiet_mode = 1;
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

  // check available ALSA formats for this device
  if (pcm_format_check) {

    printf("PCM device name: %s\n", pcm_device_name != NULL ? pcm_device_name : "default");

    snd_pcm_hw_params_t* params;
    snd_pcm_format_mask_t* format_mask;
    snd_pcm_open(&pcm_handle, pcm_device_name != NULL ? pcm_device_name : "default", SND_PCM_STREAM_PLAYBACK, 0);
    
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

  // no pcm file?
  if (pcm_file_name == NULL) {
    show_help_message();
    goto exit;
  }

  // input format check
  char* pcm_file_exp = pcm_file_name + strlen(pcm_file_name) - 4;
  int16_t input_format = FORMAT_ADPCM;
  int32_t pcm_freq = 15625;
  int16_t pcm_channels = 1;
  if (stricmp(".pcm", pcm_file_exp) == 0) {
    input_format = FORMAT_ADPCM;
    pcm_freq = 15625;
    pcm_channels = 1;
  } else if (stricmp(".s22", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 22050;
    pcm_channels = 2;
  } else if (stricmp(".s24", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 24000;
    pcm_channels = 2;
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
  } else if (stricmp(".m22", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 22050;
    pcm_channels = 1;
  } else if (stricmp(".m24", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 24000;
    pcm_channels = 1;
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
  } else if (stricmp(".a22", pcm_file_exp) == 0) {
    input_format = FORMAT_YM2608;
    pcm_freq = 22050;
    pcm_channels = 2;
  } else if (stricmp(".a24", pcm_file_exp) == 0) {
    input_format = FORMAT_YM2608;
    pcm_freq = 24000;
    pcm_channels = 2;
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
  } else if (stricmp(".n22", pcm_file_exp) == 0) {
    input_format = FORMAT_YM2608;
    pcm_freq = 22050;
    pcm_channels = 1;
  } else if (stricmp(".n24", pcm_file_exp) == 0) {
    input_format = FORMAT_YM2608;
    pcm_freq = 24000;
    pcm_channels = 1;
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
  } else if (stricmp(".mp3", pcm_file_exp) == 0) {
    input_format = FORMAT_MP3;
    pcm_freq = -1;        // not yet determined
    pcm_channels = -1;    // not yet determined
  } else if (stricmp(".mcs", pcm_file_exp) == 0) {
    input_format = FORMAT_MACS;
    pcm_freq = -1;        // not yet determined
    pcm_channels = -1;    // not yet determined
  } else {
    printf("error: unknown format file (%s).\n", pcm_file_name);
    goto exit;
  }

  // init adpcm (msm6258v) decoder
  if (input_format == FORMAT_ADPCM) {
    if (adpcm_decode_open(&adpcm_decoder) != 0) {
      printf("error: ADPCM encoder initialization error.\n");
      goto exit;
    }
  }

  // init raw pcm decoder if needed
  if (input_format == FORMAT_RAW) {
    if (raw_decode_open(&raw_decoder, pcm_freq, pcm_channels, up_sampling) != 0) {
      printf("error: PCM decoder initialization error.\n");
      goto exit;
    }
  }

  // init wav decoder if needed
  if (input_format == FORMAT_WAV) {
    if (wav_decode_open(&wav_decoder, up_sampling) != 0) {
      printf("error: WAV decoder initialization error.\n");
      goto exit;
    }
  }

  // init adpcm (ym2608) decoder if needed
  if (input_format == FORMAT_YM2608) {
    if (ym2608_decode_open(&ym2608_decoder, pcm_freq, pcm_channels, up_sampling) != 0) {
      printf("error: YM2608 adpcm decoder initialization error.\n");
      goto exit;
    }
  }

  // init mp3 decoder if needed
  if (input_format == FORMAT_MP3) {
    if (mp3_decode_open(&mp3_decoder, up_sampling) != 0) {
      printf("error: MP3 decoder initialization error.\n");
      goto exit;
    }
  }

  // init macs decoder if needed
  if (input_format == FORMAT_MACS) {
    if (macs_decode_open(&macs_decoder, up_sampling) != 0) {
      printf("error: MACS decoder initialization error.\n");
      goto exit;
    }
  }

  // init OLED SSD1306
  if (use_oled) {
    if (oled_ssd1306_open(&ssd1306) != 0) {
      printf("error: OLED SSD1306 device init error.\n");
      goto exit;
    }
  }

  // open pcm file
  fp = fopen(pcm_file_name, "rb");
  if (fp == NULL) {
    printf("error: input pcm file open error.\n");
    goto exit;
  }

  // check file size
  fseek(fp, 0, SEEK_END);
  size_t pcm_data_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // read header part of WAV/MP3/MCS file
  size_t skip_offset = 0;
  if (input_format == FORMAT_WAV) {
    if (wav_decode_parse_header(&wav_decoder, fp) < 0) {
      printf("error: wav header parse error.\n");
      goto exit;
    }
    pcm_freq = wav_decoder.sample_rate;
    pcm_channels = wav_decoder.channels;
    skip_offset = wav_decoder.skip_offset;
    pcm_data_size -= skip_offset;             // overwrite
  } else if (input_format == FORMAT_MP3) {
    if (mp3_decode_parse_header(&mp3_decoder, fp) < 0) {
      printf("error: mp3 header parse error.\n");
      goto exit;
    }
    skip_offset = mp3_decoder.skip_offset;
    pcm_data_size -= skip_offset;   // overwrite
  } else if (input_format == FORMAT_MACS) {
    if (macs_decode_parse_header(&macs_decoder, fp) < 0) {
      printf("error: macs header parse error.\n");
      goto exit;
    }
    pcm_freq = macs_decoder.sample_rate;
    pcm_channels = macs_decoder.channels;
    skip_offset = macs_decoder.skip_offset;
    pcm_data_size = macs_decoder.total_bytes;   // overwrite
  }
  fseek(fp, skip_offset, SEEK_SET);

  if (input_format == FORMAT_MP3) {

    // in case of MP3, read all data at once
    mp3_read_buffer = malloc(pcm_data_size);
    fread(mp3_read_buffer, sizeof(uint8_t), pcm_data_size, fp);

    mp3_decode_setup(&mp3_decoder, mp3_read_buffer, pcm_data_size, 0);
    pcm_freq = mp3_decoder.sample_rate;
    pcm_channels = mp3_decoder.channels;

  } else {

    // dummy read for disk cache to avoid buffer underrun
    size_t dummy_read_size = pcm_freq * 2 * 2 * 8;
    uint8_t* dummy_buffer = malloc(dummy_read_size);
    fread(dummy_buffer, sizeof(uint8_t), dummy_read_size, fp);
    free(dummy_buffer);
    fseek(fp, skip_offset, SEEK_SET);

  }

  // OLED information display
  if (use_oled) {
    static char mes[128];

    char* c = strrchr(pcm_file_name, '/');
    sprintf(mes, "  FILE: %s", c != NULL ? c+1 : pcm_file_name);
    oled_ssd1306_print(&ssd1306, 0, 0, mes);

    sprintf(mes, "  SIZE: %'d", pcm_data_size);
    oled_ssd1306_print(&ssd1306, 0, 1, mes);

    sprintf(mes, "FORMAT: %s", 
      input_format == FORMAT_RAW ? "RAW" : 
      input_format == FORMAT_WAV ? "WAV" :
      input_format == FORMAT_YM2608 ? "YM2608" :
      input_format == FORMAT_MACS ? "MACS" :
      "ADPCM");
    oled_ssd1306_print(&ssd1306, 0, 2, mes);

    sprintf(mes, "  FREQ: %d [Hz]", pcm_freq);
    oled_ssd1306_print(&ssd1306, 0, 3, mes);

    sprintf(mes, "    CH: %s", pcm_channels == 1 ? "mono" : "stereo");
    oled_ssd1306_print(&ssd1306, 0, 4, mes);

    oled_ssd1306_print(&ssd1306, 0, 6, "L:");
    oled_ssd1306_print(&ssd1306, 0, 7, "R:");
  }

  // describe PCM file information
  printf("File name     : %s\n", pcm_file_name);
  printf("Data size     : %zu [bytes]\n", pcm_data_size);
  printf("Data format   : %s\n", 
    input_format == FORMAT_WAV    ? "WAV" :
    input_format == FORMAT_YM2608 ? "ADPCM(YM2608)" :
    input_format == FORMAT_RAW    ? "Raw 16bit PCM (big)" : 
    input_format == FORMAT_MACS   ? "MACS 16bit PCM (big)" :
    input_format == FORMAT_MP3    ? "MP3" :
    "ADPCM(MSM6258V)");

  // describe playback drivers
  printf("PCM driver    : %s\n", "ALSA");
  printf("PCM device    : %s\n", pcm_device_name != NULL ? pcm_device_name : "default");

  if (input_format == FORMAT_ADPCM) {
    float pcm_1sec_size = pcm_freq * 0.5;
    printf("PCM frequency : %d [Hz]\n", pcm_freq);
    printf("PCM channels  : %s\n", "mono");
    printf("PCM length    : %4.2f [sec]\n", (float)pcm_data_size / pcm_1sec_size);
  }

  // describe raw format
  if (input_format == FORMAT_RAW) {
    float pcm_1sec_size = pcm_freq * 2;
    printf("PCM frequency : %d [Hz]\n", pcm_freq);
    printf("PCM channels  : %s\n", pcm_channels == 1 ? "mono" : "stereo");
    printf("PCM length    : %4.2f [sec]\n", (float)pcm_data_size / pcm_channels / pcm_1sec_size);
  }

  // describe wav format
  if (input_format == FORMAT_WAV) {
    printf("PCM frequency : %d [Hz]\n", pcm_freq);
    printf("PCM channels  : %s\n", pcm_channels == 1 ? "mono" : "stereo");
    printf("PCM length    : %4.2f [sec]\n", (float)wav_decoder.duration / pcm_freq);
  }

  // describe ym2608 format
  if (input_format == FORMAT_YM2608) {
    float pcm_1sec_size = pcm_freq * 0.5;
    printf("PCM frequency : %d [Hz]\n", pcm_freq);
    printf("PCM channels  : %s\n", pcm_channels == 1 ? "mono" : "stereo");
    printf("PCM length    : %4.2f [sec]\n", (float)pcm_data_size / pcm_channels / pcm_1sec_size);
  }

  // describe mp3 format
  if (input_format == FORMAT_MP3) {
    printf("PCM frequency : %d [Hz]\n", pcm_freq);
    printf("PCM channels  : %s\n", pcm_channels == 1 ? "mono" : "stereo");
  }

  // describe macs format
  if (input_format == FORMAT_MACS) {
    float pcm_1sec_size = pcm_freq * 2;
    printf("PCM frequency : %d [Hz]\n", pcm_freq);
    printf("PCM channels  : %s\n", pcm_channels == 1 ? "mono" : "stereo");
    printf("PCM length    : %4.2f [sec]\n", (float)pcm_data_size / pcm_channels / pcm_1sec_size);
  }

  // allocate file read buffer
  size_t fread_buffer_len = 
    input_format == FORMAT_ADPCM  ? pcm_channels * pcm_freq * 1 / 25 :       // 2000 / 25 =  80 msec
    input_format == FORMAT_YM2608 ? pcm_channels * pcm_freq * 1 / 2 / 10 :   // 1000 / 10 = 100 msec 
                                    pcm_channels * pcm_freq * 1 / 10;        // 1000 / 10 = 100 msec

  fread_buffer = malloc(input_format == FORMAT_ADPCM  ? sizeof(uint8_t) * fread_buffer_len :
                        input_format == FORMAT_YM2608 ? sizeof(uint8_t) * fread_buffer_len : 
                                                        sizeof(int16_t) * fread_buffer_len);

  // allocate ALSA pcm buffer
  size_t pcm_buffer_len = 2 * ((pcm_freq < 44100 || up_sampling) ? 48000 * 2 : pcm_freq ) * 1;  // 1 sec (adpcm=2sec)
  pcm_buffer = (int16_t*)malloc(sizeof(int16_t) * pcm_buffer_len);            // 16/24bit & stereo ... fixed

  // level check mode
  if (pcm_level_check) {

    if (input_format == FORMAT_MP3) {
      printf("MP3 level check is not supported.\n");
      goto exit;
    }
    
    int16_t peak_level = 0;
    double total_level = 0.0;
    size_t num_samples = 0;

    if (input_format == FORMAT_ADPCM || input_format == FORMAT_YM2608) {

      size_t fread_len = 0;

      do {
        size_t len = fread(fread_buffer, sizeof(uint8_t), fread_buffer_len, fp);
        if (len == 0) break;
        fread_len += len;
        size_t decode_len = 0;
        if (input_format == FORMAT_ADPCM) {
          decode_len = adpcm_decode_exec(&adpcm_decoder, pcm_buffer, fread_buffer, len);
        } else if (input_format == FORMAT_YM2608) {
          decode_len = ym2608_decode_exec(&ym2608_decoder, pcm_buffer, fread_buffer, len);
        }
        for (size_t i = 0; i < decode_len / 2; i++) {
          int16_t v = abs(pcm_buffer[i]);
          if (v > peak_level) peak_level = v;
          total_level += (double)v;
        }
        num_samples += decode_len / 2;
      } while (fread_len < pcm_data_size && abort_flag == 0);

    } else {

      size_t fread_len = 0;

      do {
        size_t remain = pcm_data_size / 2 - fread_len;
        size_t len = fread(fread_buffer, sizeof(int16_t), remain < fread_buffer_len ? remain : fread_buffer_len, fp);
        if (len == 0) break;
        fread_len += len;
        size_t decode_len = 0;
        if (input_format == FORMAT_RAW) {
          decode_len = raw_decode_exec(&raw_decoder, pcm_buffer, fread_buffer, len);
        } else if (input_format == FORMAT_WAV) {
          decode_len = wav_decode_exec(&wav_decoder, pcm_buffer, fread_buffer, len);
        } else if (input_format == FORMAT_MACS) {
          decode_len = macs_decode_exec(&macs_decoder, pcm_buffer, fread_buffer, len);
        }
        for (size_t i = 0; i < decode_len / 2; i++) {
          int16_t v = abs(pcm_buffer[i]);
          if (v > peak_level) peak_level = v;
          total_level += (double)v;
        }
        num_samples += decode_len / 2;
      } while (fread_len * sizeof(int16_t) < pcm_data_size && abort_flag == 0);
    }

    printf("Average Level ... %4.2f%%\n", 100.0 * total_level / num_samples / 32767.0);
    printf("Peak Level    ... %4.2f%%\n", 100.0 * peak_level / 32767.0);

    goto exit;
  }

  // init ALSA device
  if ((alsa_rc = snd_pcm_open(&pcm_handle, pcm_device_name != NULL ? pcm_device_name : "default", 
                    SND_PCM_STREAM_PLAYBACK, 0)) != 0) {
    printf("error: pcm device (%s) open error. (%s)\n", pcm_device_name, snd_strerror(alsa_rc));
    goto exit;
  }

  // in case of 15.6kHz - 32kHz, upscale to 48kHz
  if ((alsa_rc = snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2,
                          (pcm_freq < 44100 || up_sampling) ? 48000 : pcm_freq, 1, alsa_latency)) != 0) {
    printf("error: pcm device setting error. (%s)\n", snd_strerror(alsa_rc));
    goto exit;
  }

  // sigint/sigterm handler
  abort_flag = 0;
  signal(SIGINT, sigint_handler);
  //signal(SIGTERM, sigint_handler);

  printf("\nnow playing ... push CTRL+C to quit.\n");

  if (input_format == FORMAT_MP3) {

  } else if (input_format == FORMAT_ADPCM || input_format == FORMAT_YM2608) {

    size_t fread_len = 0;

    do {
      size_t len = fread(fread_buffer, sizeof(uint8_t), fread_buffer_len, fp);
      if (len == 0) break;
      fread_len += len;
      size_t decode_len = 0;
      if (input_format == FORMAT_ADPCM) {
        decode_len = adpcm_decode_exec(&adpcm_decoder, pcm_buffer, fread_buffer, len);
      } else if (input_format == FORMAT_YM2608) {
        decode_len = ym2608_decode_exec(&ym2608_decoder, pcm_buffer, fread_buffer, len);
      }
      if ((alsa_rc = snd_pcm_writei(pcm_handle, (const void*)pcm_buffer, decode_len / 2)) < 0) {    
        if (snd_pcm_recover(pcm_handle, alsa_rc, 0) < 0) {
          printf("error: fatal pcm data write error.\n");
          goto exit;
        }
      }
      if (use_oled) {
        int16_t peak_l = 0;
        int16_t peak_r = 0;
        for (size_t i = 0; i < decode_len; i++) {
          int16_t v = pcm_buffer[i] < 0 ? 0 - pcm_buffer[i] : pcm_buffer[i];
          if (i & 0x01) {
            if (v > peak_r) peak_r = v;
          } else {
            if (v > peak_l) peak_l = v;
          }
        }
        oled_ssd1306_show_meter(&ssd1306, 12, 6, peak_l, 0);
        oled_ssd1306_show_meter(&ssd1306, 12, 7, peak_r, 0);
      }
      if (!quiet_mode) {
        printf("\r%d/%d (%4.2f%%)", fread_len, pcm_data_size, fread_len * 100.0 / pcm_data_size);
        fflush(stdout);
      }
    } while (fread_len < pcm_data_size && abort_flag == 0);

  } else {

    size_t fread_len = 0;

    do {
      size_t remain = pcm_data_size / 2 - fread_len;
      size_t len = fread(fread_buffer, sizeof(int16_t), remain < fread_buffer_len ? remain : fread_buffer_len, fp);
      if (len == 0) break;
      fread_len += len;
      size_t decode_len = 0;
      if (input_format == FORMAT_RAW) {
        decode_len = raw_decode_exec(&raw_decoder, pcm_buffer, fread_buffer, len);
      } else if (input_format == FORMAT_WAV) {
        decode_len = wav_decode_exec(&wav_decoder, pcm_buffer, fread_buffer, len);
      } else if (input_format == FORMAT_MACS) {
        decode_len = macs_decode_exec(&macs_decoder, pcm_buffer, fread_buffer, len);
      }
      if ((alsa_rc = snd_pcm_writei(pcm_handle, (const void*)pcm_buffer, decode_len / 2)) < 0) {    
        if (snd_pcm_recover(pcm_handle, alsa_rc, 0) < 0) {
          printf("error: fatal pcm data write error.\n");
          goto exit;
        }
      }
      if (use_oled) {
        int16_t peak_l = 0;
        int16_t peak_r = 0;
        for (size_t i = 0; i < decode_len; i++) {
          int16_t v = pcm_buffer[i] < 0 ? 0 - pcm_buffer[i] : pcm_buffer[i];
          if (i & 0x01) {
            if (v > peak_r) peak_r = v;
          } else {
            if (v > peak_l) peak_l = v;
          }
        }
        oled_ssd1306_show_meter(&ssd1306, 12, 6, peak_l, 0);
        oled_ssd1306_show_meter(&ssd1306, 12, 7, peak_r, 0);
      }
      if (!quiet_mode) {
        printf("\r%d/%d (%4.2f%%)", fread_len * sizeof(int16_t), pcm_data_size, fread_len * sizeof(int16_t) * 100.0 / pcm_data_size);
        fflush(stdout);
      }
    } while (fread_len * sizeof(int16_t) < pcm_data_size && abort_flag == 0);
  }

  printf("\n");

  // close input file
  fclose(fp);
  fp = NULL;

  // aborted?
  if (abort_flag) {
    printf("aborted.\n");
    rc = 1;
  } else {
    // output remaining data
    if ((alsa_rc = snd_pcm_drain(pcm_handle)) != 0) {
      printf("error: pcm drain error. (%s)\n", snd_strerror(alsa_rc));
      goto exit;
    }
    rc = 0;
  }

exit:

  // close input file handle
  if (fp != NULL) {
    fclose(fp);
    fp = NULL;
  }

  // close alsa device
  if (pcm_handle != NULL) {
    snd_pcm_close(pcm_handle);
    pcm_handle = NULL;
  }

  // reclaim pcm buffer
  if (pcm_buffer != NULL) {
    free(pcm_buffer);
    pcm_buffer = NULL;
  }

  // reclaim file read buffer
  if (fread_buffer != NULL) {
    free(fread_buffer);
    fread_buffer = NULL;
  }

  // reclaim mp3 read buffer
  if (mp3_read_buffer != NULL) {
    free(mp3_read_buffer);
    mp3_read_buffer = NULL;
  }

  // close adpcm decoder
  adpcm_decode_close(&adpcm_decoder);

  // close raw decoder
  raw_decode_close(&raw_decoder);

  // close wav decoder
  wav_decode_close(&wav_decoder);

  // close ym2608 decoder
  ym2608_decode_close(&ym2608_decoder);

  // close macs decoder
  macs_decode_close(&macs_decoder);

  // close mp3 decoder
  mp3_decode_close(&mp3_decoder);

  // close OLED SSD1306
  if (use_oled) {
    oled_ssd1306_close(&ssd1306);
  }

  return rc;
}
