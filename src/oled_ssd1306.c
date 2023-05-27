#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pigpio.h>
#include "oled_ssd1306.h"

//
//  init OLED SSD1306 handle
//
int32_t oled_ssd1306_open(OLED_SSD1306* ssd1306, int16_t width, int16_t height) {

  int32_t rc = -1;

  // baseline
  ssd1306->width = width;
  ssd1306->height = height;
  
  // open i2c connection
  ssd1306->handle = open(OLED_I2C_BUS, O_RDWR);
  if (ssd->handle < 0) {
    printf("error: failed to open i2c bus.\n");
    goto exit;
  }

  // set i2c slave address
  if (ioctl(ssd1306->handle, I2C_SLAVE, OLED_I2C_ADDR) < 0)  {
    printf("error: failed to select i2c device.\n");
    goto exit;
  }

  // send init command
  uint8_t	init_commands[] = {
      0x00, 0xAE,           // display off
			0x80, 0xD5, 0x80,     // display clock
			0x80, 0xA8, 0x3F,     // multiplex ratio (64 - 1)
			0x80, 0xD3, 0x00,     // display offset 0
			0x80, 0x40, 0x00,     // start line
			0x00, 0xA1,           // segment remap
			0x00, 0xC8,           // com scan dir
			0x80, 0xDA, 0x12,     // com pin hw config
			0x80, 0x81, 0x7F,     // contrast
			0x00, 0xA4,           // disable entire display on
			0x00, 0xA6,           // set normal display
			0x80, 0x8D, 0x14,     // charge pump
			0x00, 0xAF,           // display on
	};

	write(ssd1306->handle, init_commands, sizeof(init_commands));

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
    close(ssd1306->handle);
    ssd1306->handle = 0;
  }
}

//
//  show text message
//
void oled_ssd1306_put_text(OLED_SSD1306* ssd1306, int16_t pos_x, int16_t pos_y, uint8_t* mes) {

}