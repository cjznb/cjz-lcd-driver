#ifndef LCD_H_
#define LCD_H_

#define USE_LCD_HW_INTERFACE

#include "lcd_interface.h"
#include "st7789_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ROTATION_0 = 0,
    ROTATION_90 = 90,
    ROTATION_180 = 180,
    ROTATION_270 = 270
} LCD_rotation_t;

void LCD_Init(LCD_HW_Interface_t* hw);
void LCD_DeInit(const LCD_HW_Interface_t* hw);
void LCD_Reset(const LCD_HW_Interface_t* hw);
void LCD_SetWindow(const LCD_HW_Interface_t* hw, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void LCD_FillRect(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_ClearScreen(const LCD_HW_Interface_t* hw, uint16_t color);
void LCD_DrawPixel(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, uint16_t color);
void LCD_DrawLine(const LCD_HW_Interface_t* hw, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void LCD_DrawRect(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_DrawChar(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg);
void LCD_DrawString(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg);
void LCD_DrawImage(const LCD_HW_Interface_t* hw, int x, int y, int w, int h, const uint16_t *img_data);
void LCD_DrawXBM(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* im_bits, uint16_t color, uint16_t bg);
void LCD_DrawXBM_Scaled(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *im_bits, uint16_t color, uint16_t bg, int scale);
int LCD_GetWidth(const LCD_HW_Interface_t* hw);
int LCD_GetHeight(const LCD_HW_Interface_t* hw);
void LCD_SetRotation(const LCD_HW_Interface_t* hw, LCD_rotation_t rotation);
LCD_rotation_t LCD_GetRotation(const LCD_HW_Interface_t* hw);
void LCD_WriteBuffer(const LCD_HW_Interface_t* hw, const uint8_t* buffer, uint32_t length);

#define BYTES_PER_PIXEL 2

#ifdef __cplusplus
}
#endif

#endif /* LCD_H_ */
