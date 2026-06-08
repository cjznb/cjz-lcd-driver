/**
 * @file    lcd_init_seq.h
 * @brief   LCD 驱动芯片初始化序列库
 * @note    新增芯片支持时，在此文件中添加对应的 #if 分支。
 *          每个分支需定义 static const 的 lcd_init_seq[] 数组。
 *          在 lcd_user_config.h 中选择芯片型号。
 */

#ifndef LCD_INIT_SEQ_H_
#define LCD_INIT_SEQ_H_

#include "lcd_user_config.h"
#include "st7789_cmd.h"

/* ========================================================================
 * 一、通用类型定义
 * ======================================================================== */

/** LCD 初始化序列的单个步骤 */
typedef struct {
    uint8_t        cmd;       /* 命令字节 */
    uint8_t        data_len;  /* 数据长度（0 表示无数据） */
    const uint8_t *data;      /* 数据指针（data_len=0 时可传 NULL） */
    uint16_t       delay;     /* 发送后延时（毫秒） */
} lcd_init_step_t;

/* ========================================================================
 * 二、MADCTL 计算宏（依赖 lcd_user_config.h 中的显示方向配置）
 * ======================================================================== */

#define LCD_MADCTL_VAL (                               \
    (LCD_HORIZONTAL_FLIP ? MADCTL_MX  : 0) |           \
    (LCD_VERTICAL_FLIP   ? MADCTL_MY  : 0) |           \
    (LCD_SWAP_RGB        ? MADCTL_BGR : MADCTL_RGB)     \
)

/* ========================================================================
 * 三、各芯片初始化序列定义
 *
 * 数据格式说明：
 *   { 命令, 数据长度, 数据指针, 延时(ms) }
 *
 * 添加新芯片的步骤：
 *   1. 在 lcd_user_config.h 中添加 #define LCD_DRIVER_CHIP_XXX 1
 *   2. 在此文件下方新增一个 #elif defined(LCD_DRIVER_CHIP_XXX) 分支
 *   3. 在分支内定义 static 数据数组和 lcd_init_seq[] 序列
 * ======================================================================== */

/* ================================================================
 * ST7789 / ST7789VW
 * 分辨率：240×320，接口：SPI / 8080
 * ================================================================ */
#if defined(LCD_DRIVER_CHIP_ST7789)

static const uint8_t _st7789_data_B2[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
static const uint8_t _st7789_data_C2[] = {0x01, 0xFF};
static const uint8_t _st7789_data_E0[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
static const uint8_t _st7789_data_E1[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};

#if LCD_COLOR_INVERSION
static const lcd_init_step_t _st7789_inv = { ST7789_INVON,  0, NULL, 0 };
#else
static const lcd_init_step_t _st7789_inv = { ST7789_INVOFF, 0, NULL, 0 };
#endif

static const lcd_init_step_t lcd_init_seq[] = {
    { ST7789_SLPOUT,    0, NULL,                          120 },   /* 退出睡眠 */
    { ST7789_MADCTL,    1, (uint8_t[]){LCD_MADCTL_VAL},   0   },   /* 显示方向 */
    { ST7789_COLMOD,    1, (uint8_t[]){0x55},             10  },   /* RGB565 像素格式 */
    _st7789_inv,                                                  /* 颜色反转 */
    { ST7789_PORCTRL,   5, _st7789_data_B2,               0   },   /* Porch 时序 */
    { ST7789_GCTRL,     1, (uint8_t[]){0x35},             0   },   /* Gate 控制 */
    { ST7789_VCOMS,     1, (uint8_t[]){0x19},             0   },   /* VCOM 设置 */
    { ST7789_LCMCTRL,   1, (uint8_t[]){0x2C},             0   },   /* LCM 控制 */
    { ST7789_VDVVRHEN,  2, _st7789_data_C2,               0   },   /* VDV/VRH 使能 */
    { ST7789_VRHS,      1, (uint8_t[]){0x12},             0   },   /* VRH 设置 */
    { ST7789_VDVS,      1, (uint8_t[]){0x20},             0   },   /* VDV 设置 */
    { ST7789_FRCTRL2,   1, (uint8_t[]){0x0F},             0   },   /* 帧率控制 */
    { ST7789_PVGAMCTRL, 14, _st7789_data_E0,              0   },   /* 正 Gamma */
    { ST7789_NVGAMCTRL, 14, _st7789_data_E1,              0   },   /* 负 Gamma */
    { ST7789_DISPON,    0, NULL,                          100 },   /* 开启显示 */
};

/* ================================================================
 * ILI9341（示例占位）
 * 分辨率：240×320，接口：SPI / 8080
 * ================================================================ */
#elif defined(LCD_DRIVER_CHIP_ILI9341)

/* TODO：添加 ILI9341 初始化序列 */
/* 数据格式：{ 命令, 数据长度, 数据指针, 延时(ms) } */

static const lcd_init_step_t lcd_init_seq[] = {
    /* 示例格式 —— 请替换为 ILI9341 实际的初始化序列 */
    { 0x01, 0, NULL, 5 },       /* SWRESET */
    { 0x11, 0, NULL, 120 },     /* SLPOUT */
    { 0x29, 0, NULL, 20 },      /* DISPON */
};

/* ================================================================
 * ILI9488（示例占位）
 * ================================================================ */
#elif defined(LCD_DRIVER_CHIP_ILI9488)

static const lcd_init_step_t lcd_init_seq[] = {
    /* TODO：添加 ILI9488 初始化序列 */
    { 0x01, 0, NULL, 5 },
};

/* ================================================================
 * ST7735 / ST7735S（示例占位）
 * ================================================================ */
#elif defined(LCD_DRIVER_CHIP_ST7735)

static const lcd_init_step_t lcd_init_seq[] = {
    /* TODO：添加 ST7735 初始化序列 */
    { 0x01, 0, NULL, 5 },
};

/* ================================================================
 * 未选择芯片
 * ================================================================ */
#else
#error "请在 lcd_user_config.h 中定义 LCD_DRIVER_CHIP_xxx 以选择 LCD 驱动芯片"
#endif

/* 序列长度宏，供 LCD_Init() 使用 */
#define LCD_INIT_SEQ_COUNT  (sizeof(lcd_init_seq) / sizeof(lcd_init_step_t))

#endif /* LCD_INIT_SEQ_H_ */
