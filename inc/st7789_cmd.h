#ifndef BSP_INC_ST7789_H_
#define BSP_INC_ST7789_H_

#include <stdint.h>
#include <stddef.h>

/* =========================================================================
 * 颜色定义 (RGB565)
 * ========================================================================= */

/* 基础三原色与黑白 */
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F

/* 二级混合色 (CMY) */
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F

/* 辅助标准色 */
#define COLOR_MAROON      0x8000
#define COLOR_OLIVE       0x8400
#define COLOR_DARKGREEN   0x0400
#define COLOR_PURPLE      0x8010
#define COLOR_TEAL        0x0410
#define COLOR_NAVY        0x0010

/* 灰色系列 */
#define COLOR_GRAY        0x8410
#define COLOR_SILVER      0xC618
#define COLOR_DARKGRAY    0x4208

/* 常用暖色补充 */
#define COLOR_ORANGE      0xFD20

/* =========================================================================
 * ST7789 命令表
 * ========================================================================= */
#define ST7789_NOP              0x00
#define ST7789_SWRESET          0x01
#define ST7789_RDDID            0x04
#define ST7789_RDDST            0x09

#define ST7789_SLPIN            0x10
#define ST7789_SLPOUT           0x11
#define ST7789_PTLON            0x12
#define ST7789_NORON            0x13

#define ST7789_INVOFF           0x20
#define ST7789_INVON            0x21
#define ST7789_DISPOFF          0x28
#define ST7789_DISPON           0x29

#define ST7789_CASET            0x2A
#define ST7789_RASET            0x2B
#define ST7789_RAMWR            0x2C
#define ST7789_RAMRD            0x2E

#define ST7789_PTLAR            0x30
#define ST7789_VSCRDEF          0x33
#define ST7789_TEOFF            0x34
#define ST7789_TEON             0x35
#define ST7789_MADCTL           0x36
#define ST7789_VSCRSADD         0x37

#define ST7789_COLMOD           0x3A
#define ST7789_RAMCTRL          0xB0
#define ST7789_RGBCTRL          0xB1
#define ST7789_PORCTRL          0xB2
#define ST7789_FRCTRL2          0xC6
#define ST7789_GCTRL            0xB7
#define ST7789_VCOMS            0xBB
#define ST7789_LCMCTRL          0xC0
#define ST7789_VDVVRHEN         0xC2
#define ST7789_VRHS             0xC3
#define ST7789_VDVS             0xC4
#define ST7789_FRCTRL1          0xC6
#define ST7789_PWCTRL1          0xD0
#define ST7789_RDID1            0xDA
#define ST7789_RDID2            0xDB
#define ST7789_RDID3            0xDC
#define ST7789_RDID4            0xDD
#define ST7789_PVGAMCTRL        0xE0
#define ST7789_NVGAMCTRL        0xE1

/* MADCTL 参数位 */
#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08

#endif /* BSP_INC_ST7789_H_ */
