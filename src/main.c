#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>

// alsa
#include <alsa/asoundlib.h>

// codec
#include "adpcm_decode.h"
#include "raw_decode.h"
#include "wav_decode.h"
#include "ym2608_decode.h"

// oled
#include "oled_ssd1306.h"

// application
#include "s44rasp.h"

static int16_t abort_flag = 0;

static void sigint_handler(int signal) {
  if (signal == SIGINT) {
    abort_flag = 1;
  }
}

static void show_help_message() {
  printf("usage: s44rasp [options] <input-file[.pcm|.sXX|.mXX|.aXX|.nXX|.wav]>\n");
  printf("options:\n");
  printf("     -d hw:x,y ... ALSA PCM device name (i.e. hw:3,0)\n");
  printf("     -o        ... enable OLED(SSD1306) display\n");
  printf("     -u        ... upsampling to 48kHz (default for 15.6kHz/32kHz source)\n");
//  printf("     -s <serial-device>  ... serial device name (i.e. /dev/serial0)\n");
//  printf("     -l <latency>        ... ALSA PCM latency in msec (default:100ms)\n");
//  printf("     -f           ... supported format check\n");
  printf("     -h           ... show help message\n");
}

int32_t main(int32_t argc, uint8_t* argv[]) {

  // default exit code
  int32_t rc = -1;

  // pcm attribs
  snd_pcm_t* pcm_handle = NULL;
//  snd_pcm_hw_params_t* pcm_params = NULL;
  int16_t* pcm_buffer = NULL; 
  void* fread_buffer = NULL;
  uint8_t* pcm_file_name = NULL;
  uint8_t* pcm_device_name = NULL;
  uint32_t pcm_latency = 50000;
  int16_t up_sampling = 0;
//  int16_t pcm_format_check = 0;
  int32_t alsa_rc = 0;
  FILE* fp = NULL;

  // decoders
  ADPCM_DECODE_HANDLE adpcm_decoder = { 0 };
  RAW_DECODE_HANDLE raw_decoder = { 0 };
  WAV_DECODE_HANDLE wav_decoder = { 0 };
  YM2608_DECODE_HANDLE ym2608_decoder = { 0 };

  // oled
  int16_t use_oled = 0;
  OLED_SSD1306 ssd1306 = { 0 };

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
      } else if (argv[i][1] == 'o') {
        use_oled = 1;
      } else if (argv[i][1] == 'u') {
        up_sampling = 1;
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
    pcm_freq = 15625;
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
    up_sampling = 1;
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
    up_sampling = 1;
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
    if (ym2608_decode_open(&ym2608_decoder, pcm_freq, pcm_channels) != 0) {
      printf("error: YM2608 adpcm decoder initialization error.\n");
      goto exit;
    }
  }

  // initi OLED SSD1306
  if (use_oled) {
    if (oled_ssd1306_open(&ssd1306) != 0) {
      printf("error: OLED SSD1306 device init error.\n");
      goto exit;
    }
    //oled_ssd1306_print(&ssd1306,0,0,"0123456789ABCDEFabcdef");
  }

  // open pcm file
  fp = fopen(pcm_file_name, "rb");
  if (fp == NULL) {
    printf("error: input pcm file open error.\n");
    goto exit;
  }

  // read header part of WAV file
  size_t skip_offset = 0;
  if (input_format == FORMAT_WAV) {
    int32_t ofs = wav_decode_parse_header(&wav_decoder, fp);
    if (ofs < 0) {
      printf("error: wav header parse error.\n");
      goto exit;
    }
    pcm_freq = wav_decoder.sample_rate;
    pcm_channels = wav_decoder.channels;
    skip_offset = ofs;
  }

  // check file size
  fseek(fp, 0, SEEK_END);
  size_t pcm_data_size = ftell(fp) - skip_offset;
  fseek(fp, skip_offset, SEEK_SET);

  // OLED information display
  if (use_oled) {
    static uint8_t mes[128];
    uint8_t* c = strrchr(pcm_file_name, '/');
    sprintf(mes, "  FILE: %s", c != NULL ? c+1 : pcm_file_name);
    oled_ssd1306_print(&ssd1306, 0, 0, mes);
    sprintf(mes, "  SIZE: %'d", pcm_data_size);
    oled_ssd1306_print(&ssd1306, 0, 1, mes);
    sprintf(mes, "FORMAT: %s", 
      input_format == FORMAT_RAW ? "RAW" : 
      input_format == FORMAT_WAV ? "WAV" :
      input_format == FORMAT_YM2608 ? "YM2608" :
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
  printf("Data size     : %d [bytes]\n", pcm_data_size);
  printf("Data format   : %s\n", 
    input_format == FORMAT_WAV ? "WAV" :
    input_format == FORMAT_YM2608 ? "ADPCM(YM2608)" :
    input_format == FORMAT_RAW ? "16bit signed raw PCM (big)" : 
    "ADPCM(MSM6258V)");

  // describe playback drivers
  printf("PCM driver    : %s\n", "ALSA");
  printf("PCM device    : %s\n", pcm_device_name != NULL ? pcm_device_name : (uint8_t*)"default");

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

  // allocate file read buffer
  size_t fread_buffer_len = 
    input_format == FORMAT_ADPCM  ? pcm_channels * pcm_freq * 1 / 25 :       // 2000 / 5  = 400 msec
    input_format == FORMAT_YM2608 ? pcm_channels * pcm_freq * 1 / 4 / 10 :   // 1000 / 10 = 100 msec 
                                    pcm_channels * pcm_freq * 1 / 10;        // 1000 / 10 = 100 msec

  fread_buffer = malloc(input_format == FORMAT_ADPCM ? sizeof(uint8_t) * fread_buffer_len :
                                                       sizeof(int16_t) * fread_buffer_len);
  //printf("fread_buffer_len = %d\n", fread_buffer_len);

  // allocate ALSA pcm buffer
  size_t pcm_buffer_len = 2 * ((pcm_freq < 44100 || up_sampling) ? 48000 * 2 : pcm_freq ) * 1;  // 1 sec (adpcm=2sec)
  pcm_buffer = (int16_t*)malloc(sizeof(int16_t) * pcm_buffer_len);            // 16/24bit & stereo ... fixed
  //printf("pcm_buffer_len = %d\n", pcm_buffer_len);

  // init ALSA device
  if ((alsa_rc = snd_pcm_open(&pcm_handle, pcm_device_name != NULL ? pcm_device_name : (uint8_t*)"default", 
                    SND_PCM_STREAM_PLAYBACK, 0)) != 0) {
    printf("error: pcm device (%s) open error. (%s)\n", pcm_device_name, snd_strerror(alsa_rc));
    goto exit;
  }

// in case of 15.6kHz or 32kHz, upscale to 48kHz
  if ((alsa_rc = snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2,
                          (pcm_freq < 44100 || up_sampling) ? 48000 : pcm_freq, 1, pcm_latency)) != 0) {
    printf("error: pcm device setting error. (%s)\n", snd_strerror(alsa_rc));
    goto exit;
  }

  // sigint handler
  abort_flag = 0;
  signal(SIGINT, sigint_handler);

  printf("\nnow playing ... push CTRL+C to quit.\n");

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
      printf("decode_len = %d, len = %d\n", decode_len, len);
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
      printf("\r%d/%d (%4.2f%%)", fread_len, pcm_data_size, fread_len * 100.0 / pcm_data_size);
      fflush(stdout);
    } while (fread_len < pcm_data_size && abort_flag == 0);

  } else {

    size_t fread_len = 0;

    do {
      size_t len = fread(fread_buffer, sizeof(int16_t), fread_buffer_len, fp);
      if (len == 0) break;
      fread_len += len;
      size_t decode_len = 0;
      if (input_format == FORMAT_RAW) {
        decode_len = raw_decode_exec(&raw_decoder, pcm_buffer, fread_buffer, len);
      } else if (input_format == FORMAT_WAV) {
        decode_len = wav_decode_exec(&wav_decoder, pcm_buffer, fread_buffer, len);
      }
//      printf("decode_len = %d, len = %d\n", decode_len, len);
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
      printf("\r%d/%d (%4.2f%%)", fread_len * sizeof(int16_t), pcm_data_size, fread_len * sizeof(int16_t) * 100.0 / pcm_data_size);
      fflush(stdout);
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

  // close alsa params
//  if (pcm_params != NULL) {
//    snd_pcm_hw_params_free(pcm_params);
//    pcm_params = NULL;
//  }

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

  // close adpcm encoder
  if (input_format == FORMAT_ADPCM) {
    adpcm_decode_close(&adpcm_decoder);
  }

  // close raw decoder
  if (input_format == FORMAT_RAW) {
    raw_decode_close(&raw_decoder);
  }

  // close wav decoder
  if (input_format == FORMAT_WAV) {
    wav_decode_close(&wav_decoder);
  }

  // close ym2608 decoder
  if (input_format == FORMAT_YM2608) {
    ym2608_decode_close(&ym2608_decoder);
  }

  // close OLED SSD1306
  if (use_oled) {
    oled_ssd1306_close(&ssd1306);
  }

  return rc;
}
