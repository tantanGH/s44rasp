#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "oled_ssd1306.h"

//
//  init OLED SSD1306 handle
//
int32_t oled_ssd1306_open(OLED_SSD1306* ssd1306) {

  int32_t rc = -1;

  // baseline
  ssd1306->width = 128;
  ssd1306->height = 64;
  
  // open i2c connection
  ssd1306->handle = open(OLED_I2C_BUS, O_RDWR);
  if (ssd1306->handle < 0) {
    printf("error: failed to open i2c bus.\n");
    goto exit;
  }

  // set i2c slave address
  if (ioctl(ssd1306->handle, I2C_SLAVE, OLED_I2C_ADDR) < 0)  {
    printf("error: failed to select i2c device.\n");
    goto exit;
  }

  // use high speed mode
  if (ioctl(ssd1306->handle, I2C_SET_BUS_SPEED, 400000) < 0) {
    printf("error: failed to use i2c high speed mode.\n");
    goto exit;
  }

  // send init command
  uint8_t init_commands[] = {
    0x00,                 // command stream
    0xAE,                 // display off
    0x8D, 0x14,           // charge pump
    0xA8, 0x3F,           // multiplex ratio (64 - 1)
    0x40,                 // start line
    0xA1,                 // segment remap high
    0xC8,                 // com scan dir
    0x81, 0x7F,           // contrast
    0xA4,                 // disable entire display on
    0xA6,                 // set normal display 
    0xD5, 0x80,           // display clock
    0x2E,                 // deactivate scroll
    0x20, 0x00,           // horizontal addressing mode
    0x21, 0x00, 0x7f,     // column range
    0x22, 0x00, 0x07,     // page range
    0xD3, 0x00,           // display offset 0
    0xDA, 0x12,           // com pin hw config
//    0xAF,                 // display on
  };
	write(ssd1306->handle, init_commands, sizeof(init_commands));

  uint8_t clear_commands[] = {
    0x00,                 // command stream
    0xb0,                 // set page start address
    0x21, 0x00, 0x7f,     // column range
  };
	write(ssd1306->handle, clear_commands, sizeof(clear_commands));

  uint8_t clear_data[] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
  for (int16_t i = 0; i < ssd1306->height; i++) {
    write(ssd1306->handle, clear_data, sizeof(clear_data));
  }

//	write(ssd1306->handle, clear_commands, sizeof(clear_commands));
//  uint8_t line_data[] = { 0x40, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, };
//  for (int16_t i = 0; i < 8; i++) {
//    write(ssd1306->handle, line_data, sizeof(line_data));
//  }
  
  static uint8_t on_commands[] = { 0x80, 0xAF };
  write(ssd1306->handle, on_commands, sizeof(on_commands));    // display on

  rc = 0;

exit:
  if (rc != 0) {
    close(ssd1306->handle);
    ssd1306->handle = 0;
  }

  return rc;
}

//
//  close OLED SSD1306 handle
//
void oled_ssd1306_close(OLED_SSD1306* ssd1306) {
  if (ssd1306->handle != 0) {

    uint8_t clear_commands[] = {
      0x00,                 // command stream
      0xb0,                 // set page start address
      0x21, 0x00, 0x7f,     // column range
    };
    write(ssd1306->handle, clear_commands, sizeof(clear_commands));

    uint8_t clear_data[] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
    for (int16_t i = 0; i < ssd1306->height; i++) {
      write(ssd1306->handle, clear_data, sizeof(clear_data));
    }

    close(ssd1306->handle);
    ssd1306->handle = 0;
  }
}

//
//  print text message
//
void oled_ssd1306_print(OLED_SSD1306* ssd1306, int16_t pos_x, int16_t pos_y, uint8_t* mes) {

  int16_t mes_len = strlen(mes);
  if (mes_len > (128 - pos_x)) mes_len = 128 - pos_x;

  uint8_t commands[] = {
    0x00,                 // command stream
    0x21, pos_x, 0x7f,    // column range
    0x22, pos_y, pos_y,   // page range
  };
	write(ssd1306->handle, commands, sizeof(commands));

  uint8_t data[7];
  data[0] = 0x40;
  for (int16_t i = 0; i < mes_len; i++) {
    memcpy(data+1, font6x8_data + 6 * (mes[i] - 0x20), 6);
    write(ssd1306->handle, data, 7);
  }
  
}

//
//  show level meter
//
void oled_ssd1306_show_meter(OLED_SSD1306* ssd1306, int16_t pos_x, int16_t pos_y, int16_t current_level, int16_t peak_level) {

  uint8_t commands[] = {
    0x00,                 // command stream
    0x21, pos_x, 0x7f,    // column range
    0x22, pos_y, pos_y,   // page range
  };
	write(ssd1306->handle, commands, sizeof(commands));

  uint8_t data_on[7]   = { 0x40, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, };
  uint8_t data_off[7]  = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
  uint8_t data_peak[7] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, };

  int16_t num_cells = ( 128 - pos_x ) / 6;
  int16_t on_cells = current_level < 0 ? num_cells * current_level / -32768 : num_cells * current_level / 32768;
  for (int16_t i = 0; i < num_cells; i++) {
    write(ssd1306->handle, i < on_cells ? data_on : data_off, 7);
  }

}