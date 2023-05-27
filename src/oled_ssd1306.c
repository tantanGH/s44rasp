#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "oled_ssd1306.h"

//
//  init OLED SSD1306 handle
//
int32_t oled_ssd1306_open(OLED_SSD1306* ssd1306, int16_t width, int16_t height) {

  int32_t rc = -1;

  // baseline
  ssd1306->width = width;
  ssd1306->height = height;
  
  rc = 0;

exit:
  return rc;
}

//
//  close OLED SSD1306 handle
//
void oled_ssd1306_close(OLED_SSD1306* ssd1306) {

}

//
//  show text message
//
void oled_ssd1306_put_text(OLED_SSD1306* ssd1306, int16_t pos_x, int16_t pos_y, uint8_t* mes) {
  
}