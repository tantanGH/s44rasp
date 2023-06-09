#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "oled_ssd1306.h"

// 6x8 font bitmap 0x20 - 0x7e
static uint8_t font6x8_data[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x5f,0x00,0x00,0x00,
    0x04,0x03,0x04,0x03,0x00,0x00,
    0x12,0x7f,0x12,0x7f,0x12,0x00,
    0x24,0x2a,0x7f,0x2a,0x12,0x00,
    0x23,0x13,0x08,0x64,0x62,0x00,
    0x30,0x4e,0x59,0x26,0x50,0x00,
    0x04,0x03,0x00,0x00,0x00,0x00,
    0x00,0x00,0x1c,0x22,0x41,0x00,
    0x41,0x22,0x1c,0x00,0x00,0x00,
    0x14,0x08,0x3e,0x08,0x14,0x00,
    0x08,0x08,0x3e,0x08,0x08,0x00,
    0x00,0x50,0x30,0x00,0x00,0x00,
    0x08,0x08,0x08,0x08,0x08,0x00,
    0x60,0x60,0x00,0x00,0x00,0x00,
    0x40,0x20,0x10,0x08,0x04,0x00,
    0x3e,0x51,0x49,0x45,0x3e,0x00,
    0x00,0x42,0x7f,0x40,0x00,0x00,
    0x62,0x51,0x49,0x49,0x46,0x00,
    0x22,0x41,0x49,0x49,0x36,0x00,
    0x18,0x14,0x12,0x7f,0x10,0x00,
    0x2f,0x45,0x45,0x45,0x39,0x00,
    0x3e,0x49,0x49,0x49,0x32,0x00,
    0x01,0x61,0x19,0x05,0x03,0x00,
    0x36,0x49,0x49,0x49,0x36,0x00,
    0x26,0x49,0x49,0x49,0x3e,0x00,
    0x00,0x36,0x36,0x00,0x00,0x00,
    0x00,0x56,0x36,0x00,0x00,0x00,
    0x00,0x08,0x14,0x22,0x41,0x00,
    0x14,0x14,0x14,0x14,0x14,0x00,
    0x41,0x22,0x14,0x08,0x00,0x00,
    0x02,0x01,0x59,0x09,0x06,0x00,
    0x3e,0x41,0x5d,0x55,0x2e,0x00,
    0x60,0x1c,0x13,0x1c,0x60,0x00,
    0x7f,0x49,0x49,0x49,0x36,0x00,
    0x1c,0x22,0x41,0x41,0x22,0x00,
    0x7f,0x41,0x41,0x22,0x1c,0x00,
    0x7f,0x49,0x49,0x49,0x41,0x00,
    0x7f,0x09,0x09,0x01,0x01,0x00,
    0x1c,0x22,0x41,0x49,0x3a,0x00,
    0x7f,0x08,0x08,0x08,0x7f,0x00,
    0x00,0x41,0x7f,0x41,0x00,0x00,
    0x20,0x40,0x40,0x40,0x3f,0x00,
    0x7f,0x08,0x14,0x22,0x41,0x00,
    0x7f,0x40,0x40,0x40,0x40,0x00,
    0x7f,0x04,0x18,0x04,0x7f,0x00,
    0x7f,0x04,0x08,0x10,0x7f,0x00,
    0x3e,0x41,0x41,0x41,0x3e,0x00,
    0x7f,0x09,0x09,0x09,0x06,0x00,
    0x3e,0x41,0x51,0x21,0x5e,0x00,
    0x7f,0x09,0x19,0x29,0x46,0x00,
    0x26,0x49,0x49,0x49,0x32,0x00,
    0x01,0x01,0x7f,0x01,0x01,0x00,
    0x3f,0x40,0x40,0x40,0x3f,0x00,
    0x03,0x1c,0x60,0x1c,0x03,0x00,
    0x0f,0x70,0x0f,0x70,0x0f,0x00,
    0x41,0x36,0x08,0x36,0x41,0x00,
    0x01,0x06,0x78,0x06,0x01,0x00,
    0x61,0x51,0x49,0x45,0x00,0x00,
    0x00,0x00,0x7f,0x41,0x41,0x00,
    0x01,0x06,0x08,0x30,0x40,0x00,
    0x41,0x41,0x7f,0x00,0x00,0x00,
    0x00,0x02,0x01,0x02,0x00,0x00,
    0x40,0x40,0x40,0x40,0x40,0x00,
    0x00,0x00,0x00,0x36,0x49,0x00,
    0x20,0x54,0x54,0x78,0x00,0x00,
    0x7f,0x44,0x44,0x38,0x00,0x00,
    0x38,0x44,0x44,0x28,0x00,0x00,
    0x38,0x44,0x44,0x7f,0x00,0x00,
    0x38,0x54,0x54,0x18,0x00,0x00,
    0x04,0x7e,0x05,0x01,0x00,0x00,
    0x08,0x54,0x54,0x3c,0x00,0x00,
    0x7f,0x04,0x04,0x78,0x00,0x00,
    0x00,0x00,0x7d,0x00,0x00,0x00,
    0x00,0x40,0x40,0x3d,0x00,0x00,
    0x7f,0x10,0x28,0x44,0x00,0x00,
    0x00,0x01,0x7f,0x00,0x00,0x00,
    0x7c,0x04,0x7c,0x04,0x78,0x00,
    0x7c,0x04,0x04,0x78,0x00,0x00,
    0x38,0x44,0x44,0x38,0x00,0x00,
    0x7c,0x14,0x14,0x08,0x00,0x00,
    0x08,0x14,0x14,0x7c,0x00,0x00,
    0x7c,0x08,0x04,0x04,0x00,0x00,
    0x48,0x54,0x54,0x24,0x00,0x00,
    0x04,0x3e,0x44,0x44,0x00,0x00,
    0x3c,0x40,0x40,0x7c,0x00,0x00,
    0x3c,0x40,0x20,0x1c,0x00,0x00,
    0x1c,0x60,0x1c,0x60,0x1c,0x00,
    0x6c,0x10,0x10,0x6c,0x00,0x00,
    0x4c,0x50,0x20,0x1c,0x00,0x00,
    0x44,0x64,0x54,0x4c,0x00,0x00,
    0x00,0x00,0x00,0x36,0x49,0x00,
    0x00,0x00,0x7f,0x00,0x00,0x00,
    0x49,0x36,0x00,0x00,0x00,0x00,
    0x01,0x01,0x01,0x01,0x01,0x00,
};

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
void oled_ssd1306_print(OLED_SSD1306* ssd1306, int16_t pos_x, int16_t pos_y, char* mes) {

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
//  uint8_t data_peak[7] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, };

  int16_t num_cells = ( 128 - pos_x ) / 6;
  int16_t on_cells = current_level < 0 ? num_cells * current_level / -32768 : num_cells * current_level / 32768;
  for (int16_t i = 0; i < num_cells; i++) {
    write(ssd1306->handle, i < on_cells ? data_on : data_off, 7);
  }

}