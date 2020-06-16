#include "display.h"

#include <lvgl.h>
#include "fast_spi.h"
#include "images.h"
#include "font57cz.h"
#include "battery.h"
#include "touch.h"
#include "accl.h"
#include "menu.h"
#include "ble.h"
#include "heartrate.h"
#include "backlight.h"
#include "inputoutput.h"
#include "bootloader.h"
#include "time.h"
#include "push.h"

#define buffer_lcd_size LV_HOR_RES_MAX * 30
static lv_disp_buf_t disp_buf;
static lv_color_t buf[buffer_lcd_size];

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  startWrite_display();
  setAddrWindowDisplay(area->x1, area->y1, w, h);
  write_fast_spi(reinterpret_cast<const uint8_t *>(color_p), (w * h * 2));
  endWrite_display();
  lv_disp_flush_ready(disp);
}

bool my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data)
{
  bool touched = false;
  touch_data_struct touch_data;
  if (swipe_enabled()) {
    get_read_touch();
    touch_data = get_touch();
    touched = (touch_data.event == 2) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    check_menu(touch_data);
    get_new_touch_interrupt();
  } else {
    if (get_new_touch_interrupt()) {
      touch_data = get_touch();
      touched = (touch_data.gesture == TOUCH_SINGLE_CLICK) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
      check_menu(touch_data);
    } else {
      last_uni_char = false;
      if (last_char == 0xC3) {    //remap UTF-8 characters to 8-bit encoding
        switch (c) {
          case 0x84://Ä
            c = 0x8E;
            break;
          case 0xA4://ä
            c = 0x84;
            break;
          case 0x96://Ö
            c = 0x99;
            break;
          case 0xB6://ö
            c = 0x94;
            break;
          case 0x9C://Ü
            c = 0x9A;
            break;
          case 0xBC://ü
            c = 0x81;
            break;
          case 0x9F://ß
            c = 0xE1;
            break;

          case 0x81://Á
            c = 0xB5;
            break;
          case 0xA1://á
            c = 0xA0;
            break;
          case 0x89://É
            c = 0x90;
            break;
          case 0xA9://é
            c = 0x82;
            break;
          case 0x8D://Í
            c = 0xD6;
            break;
          case 0xAD://í
            c = 0xA1;
            break;
          case 0x93://Ó
            c = 0xE0;
            break;
          case 0xB3://ó
            c = 0xA2;
            break;
          case 0x9A://Ú
            c = 0xE9;
            break;
          case 0xBA://ú
            c = 0xA3;
            break;
          case 0x9D://Ý
            c = 0xED;
            break;
          case 0xBD://ý
            c = 0xEC;
            break;
          default:
            return false;
            break;
        }
      } else if (last_char == 0xC4) {
        switch (c) {
          case 0x8C://Č
            c = 0x80;
            break;
          case 0x8D://č
            c = 0x87;
            break;
          case 0x8E://Ď
            c = 0x9D;
            break;
          case 0x8F://ď
            c = 0x9B;
            break;
          case 0x9A://Ě
            c = 0xD2;
            break;
          case 0x9B://ě
            c = 0x88;
            break;
          default:
            return false;
            break;
        }
      } else if (last_char == 0xC5) {
        switch (c) {
          case 0xAE://Ů
            c = 0xEA;
            break;
          case 0xAF://ů
            c = 0x96;
            break;
          case 0x87://Ň
            c = 0xA5;
            break;
          case 0x88://ň
            c = 0xA4;
            break;
          case 0x98://Ř
            c = 0x92;
            break;
          case 0x99://ř
            c = 0x91;
            break;
          case 0xA0://Š
            c = 0xB6;
            break;
          case 0xA1://š
            c = 0x83;
            break;
          case 0xA4://Ť
            c = 0xD7;
            break;
          case 0xA5://ť
            c = 0x8C;
            break;
          case 0xBD://Ž
            c = 0xE2;
            break;
          case 0xBE://ž
            c = 0x93;
            break;
          default:
            return false;
            break;
        }
      } else if (last_char == 0xF0 && c == 0x9F)
        c = 0x02;
      else
        return false;
    }
  }
  for (int8_t i = 0; i < 5; i++) {
    uint8_t line = font57[c * 5 + i];
    for (int8_t j = 0; j < 8; j++, line >>= 1) {
      if (line & 1) {
        displayRect(x + i * size, y + j * size, size, size, color);
      } else if (bg != color) {
        displayRect(x + i * size, y + j * size, size, size, bg);
      }
      touched = LV_INDEV_STATE_REL;
    }
  }
  data->state = touched;
  data->point.x = touch_data.xpos;
  data->point.y = touch_data.ypos;
  return false;
}

void inc_tick() {
  lv_tick_inc(40);
}

void init_display() {
  initDisplay();
  lv_init();
  lv_disp_buf_init(&disp_buf, buf, NULL, buffer_lcd_size);

  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 240;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;

  lv_indev_drv_register(&indev_drv);

  lv_theme_t *th = lv_theme_night_init(10, NULL);
  lv_theme_set_current(th);
}

void display_enable(bool state) {
  uint8_t temp[2];
  startWrite_display();
  if (state) {
    spiCommand(ST77XX_DISPON);
    spiCommand(ST77XX_SLPOUT);
  } else {
    spiCommand(ST77XX_SLPIN);
    spiCommand(ST77XX_DISPOFF);
  }
  endWrite_display();
}

void setAddrWindowDisplay(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  uint8_t temp[4];
  spiCommand(0x2A);
  temp[0] = 0x00;
  temp[1] = x;
  temp[2] = 0x00;
  temp[3] = (x + w - 1);
  write_fast_spi(temp, 4);
  spiCommand(0x2B);
  temp[0] = 0x00;
  temp[1] = y;
  temp[2] = 0x00;
  temp[3] = ((y + h - 1) & 0xFF);
  write_fast_spi(temp, 4);
  spiCommand(0x2C);
}

void initDisplay() {
  uint8_t temp[25];
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_RESET, OUTPUT);
  pinMode(LCD_DET, OUTPUT);

  digitalWrite(LCD_CS , HIGH);
  digitalWrite(LCD_RS , HIGH);

  digitalWrite(LCD_RESET, HIGH);
  delay(20);
  digitalWrite(LCD_RESET, LOW);
  delay(100);
  digitalWrite(LCD_RESET, HIGH);
  delay(100);
  startWrite_display();
  spiCommand(54);
  spiWrite(0);
  temp[0] = 0x00;
  write_fast_spi(temp, 1);
  spiCommand(58);
  temp[0] = 5;
  write_fast_spi(temp, 1);
  spiCommand(178);
  spiWrite(12);
  spiWrite(12);
  spiWrite(0);
  spiWrite(51);
  spiWrite(51);
  temp[0] = 12;
  temp[1] = 12;
  temp[2] = 0;
  temp[3] = 51;
  temp[4] = 51;
  write_fast_spi(temp, 5);
  spiCommand(183);
  temp[0] = 53;
  write_fast_spi(temp, 1);
  spiCommand(187);
  temp[0] = 25;
  write_fast_spi(temp, 1);
  spiCommand(192);
  temp[0] = 44;
  write_fast_spi(temp, 1);
  spiCommand(194);
  temp[0] = 1;
  write_fast_spi(temp, 1);
  spiCommand(195);
  temp[0] = 18;
  write_fast_spi(temp, 1);
  spiCommand(196);
  temp[0] = 32;
  write_fast_spi(temp, 1);
  spiCommand(198);
  temp[0] = 15;
  write_fast_spi(temp, 1);
  spiCommand(208);
  temp[0] = 164;
  temp[1] = 161;
  write_fast_spi(temp, 2);
  spiCommand(224);
  temp[0] = 208;
  temp[1] = 4;
  temp[2] = 13;
  temp[3] = 17;
  temp[4] = 19;
  temp[5] = 43;
  temp[6] = 63;
  temp[7] = 84;
  temp[8] = 76;
  temp[9] = 24;
  temp[10] = 13;
  temp[11] = 11;
  temp[12] = 31;
  temp[13] = 35;
  write_fast_spi(temp, 14);
  spiCommand(225);
  temp[0] = 208;
  temp[1] = 4;
  temp[2] = 12;
  temp[3] = 17;
  temp[4] = 19;
  temp[5] = 44;
  temp[6] = 63;
  temp[7] = 68;
  temp[8] = 81;
  temp[9] = 47;
  temp[10] = 31;
  temp[11] = 31;
  temp[12] = 32;
  temp[13] = 35;
  write_fast_spi(temp, 14);
  spiCommand(33);
  spiCommand(17);
  delay(120);
  spiCommand(41);
  spiCommand(0x11);
  spiCommand(0x29);
  endWrite_display();
}

void spiCommand(uint8_t d) {
  digitalWrite(LCD_RS , LOW);
  write_fast_spi(&d, 1);
  digitalWrite(LCD_RS , HIGH);
}

void spiWrite(uint8_t d) {
  write_fast_spi(&d, 1);
}

void startWrite_display(void) {
  enable_spi(true);
  digitalWrite(LCD_CS , LOW);
}

void endWrite_display(void) {
  digitalWrite(LCD_CS , HIGH);
  enable_spi(false);
}
