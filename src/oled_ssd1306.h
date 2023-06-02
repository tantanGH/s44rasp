#ifndef __H_SSD1306__
#define __H_SSD1306__

#define OLED_I2C_BUS    "/dev/i2c-1"
#define OLED_I2C_ADDR   (0x3c)

typedef struct {

  int32_t handle;

  int16_t width;
  int16_t height;

} OLED_SSD1306;

int32_t oled_ssd1306_open(OLED_SSD1306* ssd1306);
void oled_ssd1306_close(OLED_SSD1306* ssd1306);
void oled_ssd1306_print(OLED_SSD1306* ssd1306, int16_t pos_x, int16_t pos_y, char* mes);
void oled_ssd1306_show_meter(OLED_SSD1306* ssd1306, int16_t pos_x, int16_t pos_y, int16_t current_level, int16_t peak_level);

#endif
