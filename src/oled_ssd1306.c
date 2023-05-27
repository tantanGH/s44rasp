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
int32_t oled_ssd1306_open(OLED_SSD1306* ssd1306, int16_t width, int16_t height) {

  int32_t rc = -1;

  // baseline
  ssd1306->width = width;
  ssd1306->height = height;
  
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
  uint8_t	init_commands[] = {
      0x80, 0xAE,           // display off
			0x00, 0xD5, 0x80,     // display clock
			0x00, 0xA8, 0x3F,     // multiplex ratio (64 - 1)
			0x00, 0xD3, 0x00,     // display offset 0
			0x00, 0x40, 0x00,     // start line
			0x80, 0xA1,           // segment remap
			0x80, 0xC8,           // com scan dir
			0x00, 0xDA, 0x12,     // com pin hw config
			0x00, 0x81, 0x7F,     // contrast
			0x80, 0xA4,           // disable entire display on
			0x80, 0xA6,           // set normal display
			0x00, 0x8D, 0x14,     // charge pump
			0x80, 0xAF,           // display on
  };
	write(ssd1306->handle, init_commands, sizeof(init_commands));

  uint8_t clear_commands[] = {
    0x40,
    0x00, 0x21, 0x00, ssd1306->width - 1,
    0x00, 0x22, 0x00, ssd1306->height/8 - 1,
  }
	write(ssd1306->handle, clear_commands, sizeof(clear_commands));

  uint8_t clear_data[ OLED_MAX_WIDTH ];
  memset(clear_data, 0, OLED_MAX_WIDTH);
  for (int16_t i = 0; i < ssd1306->height/8; i++) {
    write(ssd1306->handle, clear_data, ssd1306->width);
  }

  uint8_t on_commands[] = {
			0x80, 0xAF,           // display on
  };
	write(ssd1306->handle, on_commands, sizeof(on_commands));


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
    close(ssd1306->handle);
    ssd1306->handle = 0;
  }
}

//
//  show text message
//
void oled_ssd1306_put_text(OLED_SSD1306* ssd1306, int16_t pos_x, int16_t pos_y, uint8_t* mes) {

}