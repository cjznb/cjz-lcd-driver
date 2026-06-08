#include "lcd.h"
#include "lcd_buffer_draw.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static int _LCD_BufferMake(void* _buf_ptr, int x, int y, int width, int height, size_t len) {
    LCD_Buffer_t* buf_ptr = _buf_ptr;
    buf_ptr->x = x;
    buf_ptr->y = y;
    buf_ptr->width = width;
    buf_ptr->height = height;
    buf_ptr->len = len;
    buf_ptr->data_ptr = (uint8_t*)malloc(len);
    if (buf_ptr->data_ptr == NULL) {
        return -1;
    }
    memset(buf_ptr->data_ptr, 0, len);
    buf_ptr->moveable = 0;
    return 0;
}

static bool clip_rectangle_to_screen(const LCD_HW_Interface_t* hw, int *x, int *y, int *width, int *height) {
    int x_local = *x;
    int y_local = *y;
    int width_local = *width;
    int height_local = *height;

    if (width_local <= 0 || height_local <= 0) return false;
    if (x_local >= LCD_GetWidth(hw) || y_local >= LCD_GetHeight(hw)) return false;
    if (x_local + width_local <= 0 || y_local + height_local <= 0) return false;

    if (x_local < 0) { width_local += x_local; x_local = 0; }
    if (y_local < 0) { height_local += y_local; y_local = 0; }
    if (x_local + width_local > LCD_GetWidth(hw)) width_local = LCD_GetWidth(hw) - x_local;
    if (y_local + height_local > LCD_GetHeight(hw)) height_local = LCD_GetHeight(hw) - y_local;

    if (width_local <= 0 || height_local <= 0) return false;

    *x = x_local;
    *y = y_local;
    *width = width_local;
    *height = height_local;
    return true;
}

LCD_Buffer_t* LCD_BufferMake(const LCD_HW_Interface_t* hw, int x, int y, int width, int height)
{
    if(!clip_rectangle_to_screen(hw, &x, &y, &width, &height))
        return NULL;

    size_t len = (size_t)width * (size_t)height * BYTES_PER_PIXEL;
    if (len == 0 || len > MAX_BUFFER_SIZE)
        return NULL;

    LCD_Buffer_t* lcdBuffer = (LCD_Buffer_t*)malloc(sizeof(LCD_Buffer_t));
    if (lcdBuffer == NULL)
        return NULL;

    if(_LCD_BufferMake(lcdBuffer, x, y, width, height, len) != 0) {
        free(lcdBuffer);
        return NULL;
    }

    return lcdBuffer;
}

LCD_MoveableBuffer_t* LCD_MakeMoveable(const LCD_HW_Interface_t* hw, int x, int y, int width, int height) {
    if(!clip_rectangle_to_screen(hw, &x, &y, &width, &height))
        return NULL;

    LCD_MoveableBuffer_t* mbuf = malloc(sizeof(LCD_MoveableBuffer_t));
    if (!mbuf) return NULL;
    if(_LCD_BufferMake(mbuf, x, y, width, height, width * height * BYTES_PER_PIXEL) != 0) {
        free(mbuf);
        return NULL;
    }

    mbuf->buf.moveable = 1;
    mbuf->moveableAttributes.bg_color = 0;
    mbuf->moveableAttributes.dirty_count = 0;
    mbuf->moveableAttributes.is_dirty = 0;
    mbuf->moveableAttributes.sync_x = x;
    mbuf->moveableAttributes.sync_y = y;

    return mbuf;
}

void LCD_BufferDestroy(void* _buf) {
    LCD_Buffer_t* buf = _buf;
    if (!buf) return;
    if (buf->data_ptr) {
        free(buf->data_ptr);
    }
    free(buf);
}

void LCD_SendBuffer(const LCD_HW_Interface_t* hw, void *buf)
{
    LCD_Buffer_t *_buf = buf;
    if (!_buf || !_buf->data_ptr) return;

    if (_buf->moveable) {
        LCD_MoveableBuffer_t* mbuf = (LCD_MoveableBuffer_t*)_buf;
        LCDMoveableAttributes_t *lma = &mbuf->moveableAttributes;

        if (lma->is_dirty) {
            for (int i = 0; i < lma->dirty_count; i++) {
                LCD_FillRect(hw, lma->dirty_rects[i].x,
                                lma->dirty_rects[i].y,
                                lma->dirty_rects[i].w,
                                lma->dirty_rects[i].h,
                                lma->bg_color);
            }
            lma->is_dirty = 0;
            lma->dirty_count = 0;
            lma->sync_x = _buf->x;
            lma->sync_y = _buf->y;
        }
    }

    int x0 = (_buf->x < 0) ? 0 : _buf->x;
    int y0 = (_buf->y < 0) ? 0 : _buf->y;
    int x1 = (_buf->x + _buf->width > LCD_GetWidth(hw)) ? (LCD_GetWidth(hw) - 1) : (_buf->x + _buf->width - 1);
    int y1 = (_buf->y + _buf->height > LCD_GetHeight(hw)) ? (LCD_GetHeight(hw) - 1) : (_buf->y + _buf->height - 1);

    int visible_w = x1 - x0 + 1;
    int visible_h = y1 - y0 + 1;
    if (visible_w <= 0 || visible_h <= 0) return;

    LCD_SetWindow(hw, x0, y0, x1, y1);

    int start_col = x0 - _buf->x;
    int start_row = y0 - _buf->y;

    for (int r = 0; r < visible_h; r++) {
        uint8_t *line_ptr = _buf->data_ptr + ((start_row + r) * _buf->width + start_col) * 2;
        LCD_WriteBuffer(hw, line_ptr, visible_w * 2);
    }
}

void LCD_BufferClear(void *buf, uint16_t color)
{
    LCD_Buffer_t* _buf = buf;
    if (_buf == NULL || _buf->data_ptr == NULL) return;

    uint8_t high = color >> 8;
    uint8_t low  = color & 0xFF;

    uint8_t *p = _buf->data_ptr;
    size_t pixels = _buf->width * _buf->height;

    for (size_t i = 0; i < pixels; i++) {
        *p++ = high;
        *p++ = low;
    }
}

int LCD_BufferMove(LCD_MoveableBuffer_t* lcdBuffer, int target_x, int target_y, uint16_t bg) {
    if(!lcdBuffer || !lcdBuffer->buf.moveable) return -1;

    LCDMoveableAttributes_t *lma = &lcdBuffer->moveableAttributes;

    if (!lma->is_dirty) {
        lma->sync_x = lcdBuffer->buf.x;
        lma->sync_y = lcdBuffer->buf.y;
    }

    int old_x = lma->sync_x;
    int old_y = lma->sync_y;
    int w = lcdBuffer->buf.width;
    int h = lcdBuffer->buf.height;

    if (lcdBuffer->buf.x == target_x && lcdBuffer->buf.y == target_y) return 0;

    lcdBuffer->buf.x = target_x;
    lcdBuffer->buf.y = target_y;
    lma->bg_color = bg;
    lma->is_dirty = 1;

    lma->dirty_count = 0;
    if (abs(target_x - old_x) >= w || abs(target_y - old_y) >= h) {
        lma->dirty_rects[lma->dirty_count++] = (LCDRect_t){old_x, old_y, w, h};
    } else {
        if (target_y < old_y)
            lma->dirty_rects[lma->dirty_count++] = (LCDRect_t){old_x, target_y + h, w, old_y - target_y};
        else if (target_y > old_y)
            lma->dirty_rects[lma->dirty_count++] = (LCDRect_t){old_x, old_y, w, target_y - old_y};

        if (target_x < old_x)
            lma->dirty_rects[lma->dirty_count++] = (LCDRect_t){target_x + w, old_y, old_x - target_x, h};
        else if (target_x > old_x)
            lma->dirty_rects[lma->dirty_count++] = (LCDRect_t){old_x, old_y, target_x - old_x, h};
    }

    return 0;
}
