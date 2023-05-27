#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <alsa/asoundlib.h>

#define ALSA_SOFT_RESAMPLE (1)
#define ALSA_LATENCY       (50000)

static void show_help_message() {
  printf("usage: s44rasp [options] <input-file[.pcm|.sXX|.mXX|.aXX|.nXX|.wav]>\n");
  printf("options:\n");
  printf("     -d <device> ... asound PCM device name (i.e. hw:3,0)\n");
  printf("     -h          ... show help message\n");
}

int32_t main(int32_t argc, uint8_t* argv[]) {

  int32_t rc = -1;

  snd_pcm_t* pcm_handle = NULL;
  int16_t* pcm_buffer = NULL; 
  uint8_t* pcm_file_name = NULL;
  uint8_t* pcm_device_name = NULL;
  FILE* fp = NULL;

  for (int16_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && strlen(argv[i]) >= 2) {
      if (argv[i][1] == 'd' && i+1 < argc) {
        pcm_device_name = argv[ i + 1 ];
        i++;
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

  if (snd_pcm_open(&pcm_handle, pcm_device_name != NULL ? pcm_device_name : (uint8_t*)"default", 
                    SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) != 0) {
    printf("error: pcm device open error.\n");
    goto exit;
  }

  int16_t pcm_channels = 2;
  int32_t pcm_freq = 44100;
  
  if (snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 
                          pcm_channels, pcm_freq, ALSA_SOFT_RESAMPLE, ALSA_LATENCY) != 0) {
    printf("error: pcm device setting error.\n");
    goto exit;
  }

  size_t pcm_buffer_len = pcm_freq * pcm_channels * 30;
  pcm_buffer = malloc(sizeof(int16_t) * pcm_buffer_len);
  fp = fopen("01.s44", "rb");
  if (fp == NULL) {
    printf("error: s44 file open error.\n");
    goto exit;
  }

  size_t fread_len = fread(pcm_buffer, sizeof(int16_t), pcm_buffer_len, fp);
  uint8_t* pcm_buffer_uint8 = (uint8_t*)pcm_buffer;
  for (size_t i = 0; i < fread_len; i++) {
    uint8_t c = pcm_buffer_uint8[ i * 2 + 0 ];
    pcm_buffer_uint8[ i * 2 + 0 ] = pcm_buffer_uint8[ i * 2 + 1]; 
    pcm_buffer_uint8[ i * 2 + 1 ] = c;
  }
  snd_pcm_writei(pcm_handle, (const void*)pcm_buffer, fread_len);
  
  fclose(fp);
  fp = NULL;

  snd_pcm_drain(pcm_handle);
  
  rc = 0;
  
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
