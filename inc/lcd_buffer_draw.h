#ifndef LCD_BUFFER_DRAW_H_
#define LCD_BUFFER_DRAW_H_
#include "lcd.h"
#include "lcd_interface.h"

typedef struct {
    int x, y, w, h;
} LCDRect_t;

typedef struct {
    uint16_t bg_color;
    LCDRect_t dirty_rects[4];
    int dirty_count;

    int is_dirty;
    int sync_x;
    int sync_y;
} LCDMoveableAttributes_t;

typedef struct
{
    int x;
    int y;
    int width;
    int height;
    size_t len;
    uint8_t *data_ptr;
    int moveable;
} LCD_Buffer_t;

typedef struct LCDMoveableBuffer
{
    LCD_Buffer_t buf;
    LCDMoveableAttributes_t moveableAttributes;
} LCD_MoveableBuffer_t;

LCD_Buffer_t* LCD_BufferMake(const LCD_HW_Interface_t* hw, int x, int y, int width, int height);
void LCD_BufferDestroy(void* _buf);
void LCD_BufferClear(void *buf, uint16_t color);

void LCD_FillRectBuffer(void* _buf, int x, int y, int w, int h, uint16_t color);
void LCD_DrawRectangleBuffer(void* _buf, int x, int y, int w, int h, uint16_t color);
void LCD_SendBuffer(const LCD_HW_Interface_t* hw, void *buf);
void LCD_DrawImageBuffer(void* _buf, int x, int y, int w, int h, const uint8_t *img_data);
void LCD_DrawXBMBuffer(void* _buf, int x, int y, int w, int h, const uint8_t *im_bits, uint16_t color, uint16_t bg);
void LCD_DrawXBM_ScaledBuffer(void* _buf, int x, int y, int w, int h, const uint8_t *im_bits, uint16_t color, uint16_t bg, int scale);
void LCD_DrawLineBuffer(void* _buf, int x0, int y0, int x1, int y1, uint16_t color);
void LCD_DrawTriangleBuffer(void* _buf, int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
void LCD_DrawPolygonBuffer(void* _buf, int n, const int *x, const int *y, uint16_t color);
void LCD_DrawCircleBuffer(void* _buf, int cx, int cy, int r, uint16_t color);
void LCD_DrawEllipseBuffer(void* _buf, int cx, int cy, int a, int b, uint16_t color);
void LCD_FillTriangleBuffer(void* _buf, int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
void LCD_FillPolygonBuffer(void* _buf, int n, const int *vx, const int *vy, uint16_t color);
void LCD_FillCircleBuffer(void* _buf, int cx, int cy, int r, uint16_t color);
void LCD_FillEllipseBuffer(void* _buf, int cx, int cy, int a, int b, uint16_t color);
void LCD_DrawAALineBuffer(void* _buf, int x0, int y0, int x1, int y1, int width, uint16_t color);
void LCD_DrawCharBuffer(void* _buf, int x, int y, char ch, uint16_t color, uint16_t bg);
void LCD_DrawStringBuffer(void* _buf, int x, int y, const char *str, uint16_t color, uint16_t bg);
int LCD_BufferMove(LCD_MoveableBuffer_t* lcdBuffer, int target_x, int target_y, uint16_t bg);
LCD_MoveableBuffer_t* LCD_MakeMoveable(const LCD_HW_Interface_t* hw, int x, int y, int width, int height);

#endif // LCD_BUFFER_DRAW_H_
