#ifndef __H_SSD1306__
#define __H_SSD1306__

#define OLED_I2C_BUS  (1)
#define OLED_I2C_ADDR (0x3c)

// SSD1306 commands
#define OLED_CMD_DISPLAY_OFF 0xAE
#define OLED_CMD_DISPLAY_ON 0xAF
#define OLED_CMD_SET_DISPLAY_CLOCK_DIV 0xD5
#define OLED_CMD_SET_MULTIPLEX_RATIO 0xA8
#define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
#define OLED_CMD_SET_START_LINE 0x40
#define OLED_CMD_CHARGE_PUMP 0x8D
#define OLED_CMD_MEMORY_MODE 0x20
#define OLED_CMD_SEG_REMAP 0xA1
#define OLED_CMD_COM_SCAN_DIR 0xC8
#define OLED_CMD_SET_COM_PINS 0xDA
#define OLED_CMD_SET_CONTRAST 0x81
#define OLED_CMD_SET_PRECHARGE 0xD9
#define OLED_CMD_SET_VCOM_DETECT 0xDB
#define OLED_CMD_SET_PAGE_ADDR 0xB0
#define OLED_CMD_SET_COLUMN_ADDR 0x21
#define OLED_CMD_SET_ADDRESS_MODE 0x20

typedef struct {

  int32_t handle;

  int16_t width;
  int16_t height;


} OLED_SSD1306;

int32_t oled_ssd1306_open(OLED_SSD1306* ssd1306, int16_t width, int16_t height);
void oled_ssd1306_close(OLED_SSD1306* ssd1306);
void oled_ssd1306_put_text(OLED_SSD1306* ssd1306, int16_t pos_x, int16_t pos_y, uint8_t* mes);

#endif
