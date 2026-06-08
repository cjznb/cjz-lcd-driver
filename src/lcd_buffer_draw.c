#include "lcd.h"
#include "lcd_buffer_draw.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

extern unsigned char Font6x8[][6];

void LCD_DrawRectangleBuffer(void* _buf,
                               int x, int y,
                               int w, int h,
                               uint16_t color) {
    LCD_Buffer_t *buf = _buf;
    if (!buf || !buf->data_ptr || w <= 0 || h <= 0) {
        return;
    }
    LCD_DrawLineBuffer(buf, x, y, x + w - 1, y, color);
    LCD_DrawLineBuffer(buf, x, y + h - 1, x + w - 1, y + h - 1, color);
    LCD_DrawLineBuffer(buf, x, y, x, y + h - 1, color);
    LCD_DrawLineBuffer(buf, x + w - 1, y, x + w - 1, y + h - 1, color);
}

void LCD_FillRectBuffer(void* _buf, int x, int y, int w, int h, uint16_t color)
{
    LCD_Buffer_t *buf = _buf;
    if (!buf || !buf->data_ptr || w <= 0 || h <= 0) return;

    int ix0 = (x > 0) ? x : 0;
    int iy0 = (y > 0) ? y : 0;
    int ix1 = (x + w - 1 < buf->width - 1) ? (x + w - 1) : (buf->width - 1);
    int iy1 = (y + h - 1 < buf->height - 1) ? (y + h - 1) : (buf->height - 1);

    if (ix0 > ix1 || iy0 > iy1) return;

    int draw_w = ix1 - ix0 + 1;
    int draw_h = iy1 - iy0 + 1;

    uint16_t color_swapped = (color >> 8) | (color << 8);

    uint16_t *p_base = (uint16_t *)buf->data_ptr;
    uint32_t buf_w = buf->width;
    uint16_t *p_start = p_base + (iy0 * buf_w) + ix0;

    if ((color & 0xFF) == (color >> 8)) {
        for (int row = 0; row < draw_h; row++) {
            memset(p_start + (row * buf_w), color & 0xFF, draw_w * 2);
        }
    } else {
        for (int row = 0; row < draw_h; row++) {
            uint16_t *p_line = p_start + (row * buf_w);
            int col = 0;
            for (; col <= draw_w - 4; col += 4) {
                p_line[col]     = color_swapped;
                p_line[col + 1] = color_swapped;
                p_line[col + 2] = color_swapped;
                p_line[col + 3] = color_swapped;
            }
            for (; col < draw_w; col++) {
                p_line[col] = color_swapped;
            }
        }
    }
}

void LCD_DrawImageBuffer(void* _buf,
                            int x, int y,
                            int w, int h,
                            const uint8_t *img_data)
{
    LCD_Buffer_t *buf = _buf;
    if (!buf || !buf->data_ptr || !img_data || w <= 0 || h <= 0)
        return;

    int cx0 = (x > 0) ? x : 0;
    int cy0 = (y > 0) ? y : 0;
    int cx1 = (x + w - 1 < (int)buf->width - 1) ? (x + w - 1) : ((int)buf->width - 1);
    int cy1 = (y + h - 1 < (int)buf->height - 1) ? (y + h - 1) : ((int)buf->height - 1);

    if (cx0 > cx1 || cy0 > cy1) return;

    int draw_w = cx1 - cx0 + 1;
    int draw_h = cy1 - cy0 + 1;

    uint16_t *dst_base = (uint16_t *)buf->data_ptr;
    uint32_t buf_w     = buf->width;

    int src_x0 = cx0 - x;
    int src_y0 = cy0 - y;

    uint16_t *dst_line = dst_base + (cy0 * buf_w) + cx0;
    const uint8_t *src_line = img_data + (src_y0 * w + src_x0) * 2;
    size_t line_bytes = draw_w * 2;

    for (int row = 0; row < draw_h; row++) {
        memcpy(dst_line, src_line, line_bytes);
        dst_line += buf_w;
        src_line += w * 2;
    }
}

static inline int xbm_get_pixel(const uint8_t *bitmap, int x, int y, int width)
{
    int bytes_per_row = (width + 7) / 8;
    int byte_index = y * bytes_per_row + (x >> 3);
    int bit_index = x & 7;
    return (bitmap[byte_index] >> bit_index) & 0x01;
}

void LCD_DrawXBMBuffer(void* _buf, int x, int y, int w, int h,
    const uint8_t *im_bits, uint16_t color, uint16_t bg)
{
    LCD_Buffer_t *buf = _buf;
    if (buf == NULL || buf->data_ptr == NULL || im_bits == NULL) return;
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            int pixel = xbm_get_pixel(im_bits, col, row, w);
            uint16_t c = pixel ? color : bg;
            LCD_FillRectBuffer(buf, x + col, y + row, 1, 1, c);
        }
    }
}

void LCD_DrawXBM_ScaledBuffer(void* _buf, int x, int y, int w, int h,
    const uint8_t *im_bits, uint16_t color, uint16_t bg, int scale)
{
    LCD_Buffer_t *buf = _buf;
    if (buf == NULL || buf->data_ptr == NULL || im_bits == NULL) return;
    if (scale < 1) scale = 1;
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            int pixel = xbm_get_pixel(im_bits, col, row, w);
            uint16_t c = pixel ? color : bg;
            int px = x + col * scale;
            int py = y + row * scale;
            LCD_FillRectBuffer(buf, px, py, scale, scale, c);
        }
    }
}

void LCD_DrawLineBuffer(void* _buf, int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    LCD_Buffer_t *buf = _buf;

    while (1) {
        LCD_FillRectBuffer(buf, x0, y0, 1, 1, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err << 1;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void LCD_DrawTriangleBuffer(void* _buf, int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
    LCD_Buffer_t *buf = _buf;
    LCD_DrawLineBuffer(buf, x0, y0, x1, y1, color);
    LCD_DrawLineBuffer(buf, x1, y1, x2, y2, color);
    LCD_DrawLineBuffer(buf, x2, y2, x0, y0, color);
}

void LCD_DrawPolygonBuffer(void* _buf, int n, const int *x, const int *y, uint16_t color)
{
    if (n < 3 || x == NULL || y == NULL) return;
    LCD_Buffer_t *buf = _buf;
    for (int i = 0; i < n - 1; i++) {
        LCD_DrawLineBuffer(buf, x[i], y[i], x[i + 1], y[i + 1], color);
    }
    LCD_DrawLineBuffer(buf, x[n - 1], y[n - 1], x[0], y[0], color);
}

void LCD_DrawCircleBuffer(void* _buf, int cx, int cy, int r, uint16_t color)
{
    if (r < 1) return;
    int x = r, y = 0, err = 1 - x;
    LCD_Buffer_t *buf = _buf;
    while (x >= y) {
        LCD_FillRectBuffer(buf, cx + x, cy + y, 1, 1, color);
        LCD_FillRectBuffer(buf, cx + y, cy + x, 1, 1, color);
        LCD_FillRectBuffer(buf, cx - y, cy + x, 1, 1, color);
        LCD_FillRectBuffer(buf, cx - x, cy + y, 1, 1, color);
        LCD_FillRectBuffer(buf, cx - x, cy - y, 1, 1, color);
        LCD_FillRectBuffer(buf, cx - y, cy - x, 1, 1, color);
        LCD_FillRectBuffer(buf, cx + y, cy - x, 1, 1, color);
        LCD_FillRectBuffer(buf, cx + x, cy - y, 1, 1, color);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x + 1); }
    }
}

void LCD_DrawEllipseBuffer(void* _buf, int cx, int cy, int a, int b, uint16_t color)
{
    if (a < 1 || b < 1) return;
    int x = 0, y = b;
    int a2 = a * a, b2 = b * b;
    int dx = 0, dy = 2 * a2 * y;
    int err = b2 - a2 * b + (a2 >> 2);
    LCD_Buffer_t *buf = _buf;

    while (dx < dy) {
        LCD_FillRectBuffer(buf, cx + x, cy + y, 1, 1, color);
        LCD_FillRectBuffer(buf, cx - x, cy + y, 1, 1, color);
        LCD_FillRectBuffer(buf, cx + x, cy - y, 1, 1, color);
        LCD_FillRectBuffer(buf, cx - x, cy - y, 1, 1, color);
        x++; dx += 2 * b2;
        if (err < 0) err += b2 + dx;
        else { y--; dy -= 2 * a2; err += b2 + dx - dy; }
    }

    err = b2 * (x + 0.5) * (x + 0.5) + a2 * (y - 1) * (y - 1) - a2 * b2;
    while (y >= 0) {
        LCD_FillRectBuffer(buf, cx + x, cy + y, 1, 1, color);
        LCD_FillRectBuffer(buf, cx - x, cy + y, 1, 1, color);
        LCD_FillRectBuffer(buf, cx + x, cy - y, 1, 1, color);
        LCD_FillRectBuffer(buf, cx - x, cy - y, 1, 1, color);
        y--; dy -= 2 * a2;
        if (err > 0) err += a2 - dy;
        else { x++; dx += 2 * b2; err += a2 - dy + dx; }
    }
}

static void FillFlatBottomTriangle(void* _buf, int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
    int dx0 = (x2 - x0), dx1 = (x2 - x1), dy = (y2 - y0);
    int sx0 = (dx0 << 16) / dy, sx1 = (dx1 << 16) / dy;
    int curx0 = x0 << 16, curx1 = x1 << 16;
    LCD_Buffer_t *buf = _buf;
    for (int y = y0; y <= y2; y++) {
        int lx = curx0 >> 16, rx = curx1 >> 16;
        if (lx > rx) { int t = lx; lx = rx; rx = t; }
        LCD_FillRectBuffer(buf, lx, y, rx - lx + 1, 1, color);
        curx0 += sx0; curx1 += sx1;
    }
}

static void FillFlatTopTriangle(void* _buf, int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
    int dx0 = (x1 - x0), dx1 = (x2 - x0), dy = (y1 - y0);
    int sx0 = (dx0 << 16) / dy, sx1 = (dx1 << 16) / dy;
    int curx0 = x0 << 16, curx1 = x0 << 16;
    LCD_Buffer_t *buf = _buf;
    for (int y = y0; y <= y1; y++) {
        int lx = curx0 >> 16, rx = curx1 >> 16;
        if (lx > rx) { int t = lx; lx = rx; rx = t; }
        LCD_FillRectBuffer(buf, lx, y, rx - lx + 1, 1, color);
        curx0 += sx0; curx1 += sx1;
    }
}

void LCD_FillTriangleBuffer(void* _buf, int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
    LCD_Buffer_t *buf = _buf;
    if (y0 > y1) { int t; t=x0; x0=x1; x1=t; t=y0; y0=y1; y1=t; }
    if (y1 > y2) { int t; t=x1; x1=x2; x2=t; t=y1; y1=y2; y2=t; }
    if (y0 > y1) { int t; t=x0; x0=x1; x1=t; t=y0; y0=y1; y1=t; }
    if (y1 == y2) {
        FillFlatBottomTriangle(buf, x1, y1, x2, y2, x0, y0, color);
    } else if (y0 == y1) {
        FillFlatTopTriangle(buf, x2, y2, x0, y0, x1, y1, color);
    } else {
        int x_split = x0 + (y1 - y0) * (x2 - x0) / (y2 - y0);
        FillFlatBottomTriangle(buf, x1, y1, x_split, y1, x0, y0, color);
        FillFlatTopTriangle(buf, x2, y2, x1, y1, x_split, y1, color);
    }
}

void LCD_FillPolygonBuffer(void* _buf, int n, const int *vx, const int *vy, uint16_t color)
{
    if (n < 3) return;
    LCD_Buffer_t *buf = _buf;
    int ymin = vy[0], ymax = vy[0];
    for (int i = 1; i < n; i++) {
        if (vy[i] < ymin) ymin = vy[i];
        if (vy[i] > ymax) ymax = vy[i];
    }
    for (int y = ymin; y <= ymax; y++) {
        int inter_x[16], cnt = 0;
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            if ((vy[i] <= y && vy[j] > y) || (vy[j] <= y && vy[i] > y)) {
                inter_x[cnt++] = vx[i] + (y - vy[i]) * (vx[j] - vx[i]) / (vy[j] - vy[i]);
            }
        }
        for (int i = 0; i < cnt - 1; i++)
            for (int j = i + 1; j < cnt; j++)
                if (inter_x[i] > inter_x[j]) {
                    int t = inter_x[i]; inter_x[i] = inter_x[j]; inter_x[j] = t;
                }
        for (int i = 0; i < cnt; i += 2) {
            LCD_FillRectBuffer(buf, inter_x[i], y, inter_x[i + 1] - inter_x[i] + 1, 1, color);
        }
    }
}

void LCD_FillCircleBuffer(void *_buf, int cx, int cy, int r, uint16_t color)
{
    int x = r, y = 0, err = 1 - x;
    LCD_Buffer_t *buf = _buf;
    while (x >= y) {
        LCD_FillRectBuffer(buf, cx - x, cy + y, 2*x + 1, 1, color);
        LCD_FillRectBuffer(buf, cx - x, cy - y, 2*x + 1, 1, color);
        LCD_FillRectBuffer(buf, cx - y, cy + x, 2*y + 1, 1, color);
        LCD_FillRectBuffer(buf, cx - y, cy - x, 2*y + 1, 1, color);
        y++;
        if (err < 0) err += 2*y + 1;
        else { x--; err += 2*(y - x + 1); }
    }
}

void LCD_FillEllipseBuffer(void* _buf, int cx, int cy, int a, int b, uint16_t color)
{
    int x = 0, y = b;
    int a2 = a * a, b2 = b * b;
    int dx = 0, dy = 2 * a2 * y;
    int err = b2 - a2 * b + (a2 >> 2);
    LCD_Buffer_t *buf = _buf;
    while (dx < dy) {
        LCD_FillRectBuffer(buf, cx - x, cy - y, 2*x + 1, 1, color);
        LCD_FillRectBuffer(buf, cx - x, cy + y, 2*x + 1, 1, color);
        x++; dx += 2*b2;
        if (err < 0) err += b2 + dx;
        else { y--; dy -= 2*a2; err += b2 + dx - dy; }
    }
    err = b2*(x + 0.5)*(x + 0.5) + a2*(y - 1)*(y - 1) - a2*b2;
    while (y >= 0) {
        LCD_FillRectBuffer(buf, cx - x, cy - y, 2*x + 1, 1, color);
        LCD_FillRectBuffer(buf, cx - x, cy + y, 2*x + 1, 1, color);
        y--; dy -= 2*a2;
        if (err > 0) err += a2 - dy;
        else { x++; dx += 2*b2; err += a2 - dy + dx; }
    }
}

void LCD_DrawAALineBuffer(void* _buf, int x0, int y0, int x1, int y1, int width, uint16_t color)
{
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int steps = dx > dy ? dx : dy;
    LCD_Buffer_t *buf = _buf;
    for (int i = 0; i <= steps; i++) {
        int x = x0 + (x1 - x0) * i / steps;
        int y = y0 + (y1 - y0) * i / steps;
        LCD_FillCircleBuffer(buf, x, y, width / 2, color);
    }
}

void LCD_DrawCharBuffer(void* _buf, int x, int y, char ch, uint16_t color, uint16_t bg)
{
    LCD_Buffer_t *buf = _buf;
    if (buf == NULL || buf->data_ptr == NULL) return;
    if (ch < 32 || ch > 126) return;

    const int FONT_W = 6, FONT_H = 8;
    const unsigned char *bitmap = Font6x8[ch - 32];

    uint8_t fg_h = color >> 8, fg_l = color & 0xFF;
    uint8_t bg_h = bg >> 8, bg_l = bg & 0xFF;
    size_t stride = buf->width * 2;

    for (int col = 0; col < FONT_W; col++) {
        uint8_t line = bitmap[col];
        for (int row = 0; row < FONT_H; row++) {
            int px = x + col, py = y + row;
            if (px < 0 || py < 0 || px >= buf->width || py >= buf->height) {
                line >>= 1;
                continue;
            }
            uint8_t *p = buf->data_ptr + py * stride + px * 2;
            if (line & 0x01) { p[0] = fg_h; p[1] = fg_l; }
            else             { p[0] = bg_h; p[1] = bg_l; }
            line >>= 1;
        }
    }
}

void LCD_DrawStringBuffer(void* _buf, int x, int y, const char *str, uint16_t color, uint16_t bg)
{
    LCD_Buffer_t *buf = _buf;
    if (buf == NULL || buf->data_ptr == NULL || str == NULL) return;

    const int FONT_W = 6, FONT_H = 8;
    int cx = x, cy = y;

    while (*str) {
        if (*str == '\n') { cx = x; cy += FONT_H; }
        else { LCD_DrawCharBuffer(buf, cx, cy, *str, color, bg); cx += FONT_W; }
        if (cx + FONT_W > buf->width) { cx = x; cy += FONT_H; }
        if (cy + FONT_H > buf->height) break;
        str++;
    }
}
