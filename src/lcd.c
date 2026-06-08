/*
 * lcd.c
 *
 *  Created on: Nov 5, 2025
 *      Author: cjz
 *
 *  LCD核心驱动：初始化、基本绘图、字符/图片显示
 *  支持SPI和FSMC两种接口，通过lcd_user_config.h中的宏切换
 */

#include "lcd.h"
#include <stdint.h>
#include <string.h>

#define LCD_LOCAL_HEAP_SIZE 128
static uint8_t local_heap[LCD_LOCAL_HEAP_SIZE];
static size_t local_heap_offset = 0;
extern unsigned char Font6x8[][6];

#if LCD_SWAP_COLOR_BYTES
#define SWAP_BYTES(color) __builtin_bswap16(color)
#else
#define SWAP_BYTES(color) (color)
#endif

#ifdef LCD_USE_SPI
#define LCD_STEP_CMD(i, cmd)                         \
    do {                                             \
        steps[i].feedforward = LCD_Set_DC_CS;        \
        steps[i].ff_arg = &cmd_state;                \
        steps[i].tx_buf = (uint8_t[]){cmd};          \
        steps[i].length = 1;                         \
        steps[i].callback = NULL;                    \
        steps[i].cb_arg = NULL;                      \
    } while(0)

#define LCD_STEP_DATA_CONST(i, ...)                   \
    do {                                              \
        const uint8_t data[] = {__VA_ARGS__};         \
        steps[i].feedforward = LCD_Set_DC_CS;         \
        steps[i].ff_arg = &data_state;                \
        steps[i].tx_buf = (uint8_t*)data;             \
        steps[i].length = sizeof(data);               \
        steps[i].callback = NULL;                     \
        steps[i].cb_arg = NULL;                       \
    } while(0)

#define LCD_STEP_DATA_BUF(i, buf)            \
    do {                                     \
        steps[i].feedforward = LCD_Set_DC_CS;\
        steps[i].ff_arg = &data_state;       \
        steps[i].tx_buf = (uint8_t*)(buf);   \
        steps[i].length = sizeof(buf);       \
        steps[i].callback = NULL;            \
        steps[i].cb_arg = NULL;              \
    } while(0)

#define LCD_STEP_DATA_BYTE(i, value_ptr)      \
    do {                                      \
        steps[i].feedforward = LCD_Set_DC_CS; \
        steps[i].ff_arg = &data_state;        \
        steps[i].tx_buf = (uint8_t*)(value_ptr); \
        steps[i].length = 1;                  \
        steps[i].callback = NULL;             \
        steps[i].cb_arg = NULL;               \
    } while(0)

#define LCD_SUBMIT(n)                                \
    do {                                             \
        transaction.step_count = n;                  \
        hw->submit(hw, &transaction);                \
    } while(0)

#define LCD_CMD_DATA(cmd, ...) \
    do {                       \
        LCD_STEP_CMD(0, cmd);  \
        LCD_STEP_DATA_CONST(1, __VA_ARGS__); \
        LCD_SUBMIT(2);         \
    } while(0)

#define LCD_MAKE_BUFFER_TRANSACTION(buf, len, p_data_state, p_cplt_state, p_step, p_trans) \
    do {                                                                    \
        (p_data_state)->pin_state.bits.dc = 1;                            \
        (p_cplt_state)->pin_state.bits.cs = 1;                            \
        (p_step)->feedforward = LCD_Set_DC_CS;                            \
        (p_step)->ff_arg = (p_data_state);                                 \
        (p_step)->tx_buf = (uint8_t*)(buf);                                 \
        (p_step)->length = (len);                                          \
        (p_step)->callback = NULL;                                         \
        (p_step)->cb_arg = NULL;                                           \
        (p_trans)->steps = (p_step);                                       \
        (p_trans)->step_count = 1;                                         \
        (p_trans)->on_complete = LCD_Set_DC_CS;                           \
        (p_trans)->completion_arg = (p_cplt_state);                         \
    } while(0)
#endif

typedef struct {
    const LCD_HW_Interface_t* hw;
    union {
        uint32_t raw;
        struct {
            uint32_t cs  : 1;
            uint32_t dc  : 1;
            uint32_t rst : 1;
            uint32_t bl  : 1;
            uint32_t     : 28;
        } bits;
    } pin_state;
} LCD_PinState_t;

typedef struct {
    LCD_rotation_t rotation;
} LCD_RuntimeData_t;

#define LCD_DELAY_FLAG  0xFF

typedef struct {
    uint8_t cmd;
    uint8_t data_len;
    const uint8_t *data;
    uint16_t delay;
} LCD_Init_Sequence_t;

static const uint8_t data_B2[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
static const uint8_t data_C2[] = {0x01, 0xFF};
static const uint8_t data_E0[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
static const uint8_t data_E1[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};

#if LCD_COLOR_INVERSION
static const LCD_Init_Sequence_t inversion_cmd = { 0x21, 0, NULL, 0 };
#else
static const LCD_Init_Sequence_t inversion_cmd = { 0x20, 0, NULL, 0 };
#endif

#define LCD_MADCTL_VAL ( \
    (LCD_HORIZONTAL_FLIP ? MADCTL_MX : 0) | \
    (LCD_VERTICAL_FLIP   ? MADCTL_MY : 0) | \
    (LCD_SWAP_RGB        ? MADCTL_BGR : 0) \
)

static const LCD_Init_Sequence_t st7789_init_seq[] = {
    {0x11, 0, NULL, 120},
    {0x36, 1, (uint8_t[]){LCD_MADCTL_VAL}, 0},
    {0x3A, 1, (uint8_t[]){0x55}, 10},
    inversion_cmd,
    {0xB2, 5, data_B2, 0},
    {0xB7, 1, (uint8_t[]){0x35}, 0},
    {0xBB, 1, (uint8_t[]){0x19}, 0},
    {0xC0, 1, (uint8_t[]){0x2C}, 0},
    {0xC2, 2, data_C2, 0},
    {0xC3, 1, (uint8_t[]){0x12}, 0},
    {0xC4, 1, (uint8_t[]){0x20}, 0},
    {0xC6, 1, (uint8_t[]){0x0F}, 0},
    {0xE0, 14, data_E0, 0},
    {0xE1, 14, data_E1, 0},
    {0x29, 0, NULL, 100},
};

#if LCD_USE_SPI
static void LCD_Set_DC_CS(void* arg) {
    LCD_PinState_t *state = (LCD_PinState_t*)arg;
    const LCD_HW_Interface_t* hw = state->hw;
    LCD_PinState_e dc_state = state->pin_state.bits.dc ? LCD_PIN_SET : LCD_PIN_RESET;
    LCD_PinState_e cs_state = state->pin_state.bits.cs ? LCD_PIN_SET : LCD_PIN_RESET;
    if(hw) {
        if(hw->set_dc) {
            hw->set_dc(dc_state);
        }
        if(hw->set_cs) {
            hw->set_cs(cs_state);
        }
    }
}
#endif

__attribute__((optimize("O0")))
static void LCD_WriteReg(LCD_HW_Interface_t* hw, uint8_t cmd, const uint8_t* data, uint8_t len) {
#if LCD_USE_SPI
    LCD_PinState_t cmd_state = {.hw = hw, .pin_state.raw = 0};
    LCD_PinState_t data_state = {.hw = hw, .pin_state.raw = 0};
    data_state.pin_state.bits.dc = 1;
    LCD_PinState_t cplt_state = {.hw = hw, .pin_state.raw = 0};
    cplt_state.pin_state.bits.cs = 1;

    LCD_SPI_Step_t steps[20];
    LCD_Transaction_t transaction = {
        .steps = steps,
        .on_complete = LCD_Set_DC_CS,
        .completion_arg = &cplt_state
    };

    LCD_STEP_CMD(0, cmd);

    for (uint8_t i = 0; i < len; i++) {
        uint8_t step_idx = i + 1;
        steps[step_idx].feedforward = LCD_Set_DC_CS;
        steps[step_idx].ff_arg = &data_state;
        steps[step_idx].tx_buf = (uint8_t*)&data[i];
        steps[step_idx].length = 1;
        steps[step_idx].callback = NULL;
        steps[step_idx].cb_arg = NULL;
    }

    transaction.step_count = len + 1;
    hw->submit(hw, &transaction);

#elif LCD_USE_FSMC
    hw->sendCmd(hw, cmd);
    if (len > 0 && data != NULL) {
        hw->sendData(hw, (uint8_t*)data, len);
    }
#endif
}

__attribute__((optimize("O0")))
void LCD_Init(LCD_HW_Interface_t* hw)
{
    if (!hw) return;

#if LCD_USE_SPI
    if (!hw->submit) return;
#elif LCD_USE_FSMC
    if (!hw->sendCmd || !hw->sendData) return;
#endif

    if (hw->init) {
        hw->init();
    }

    if (local_heap_offset + sizeof(LCD_RuntimeData_t) > LCD_LOCAL_HEAP_SIZE) {
        return;
    }
    hw->runtime_data = (void*)(local_heap + local_heap_offset);
    local_heap_offset += sizeof(LCD_RuntimeData_t);

    LCD_Reset(hw);
    uint8_t seq_count = sizeof(st7789_init_seq) / sizeof(LCD_Init_Sequence_t);
    for (uint8_t i = 0; i < seq_count; i++) {
        const LCD_Init_Sequence_t* step = &st7789_init_seq[i];
        LCD_WriteReg(hw, step->cmd, step->data, step->data_len);
        if (step->delay > 0) {
            hw->delay_ms(step->delay);
        }
    }

    LCD_ClearScreen(hw, COLOR_BLACK);
    hw->set_bl(1);
}

void LCD_DeInit(const LCD_HW_Interface_t* hw)
{
    #if LCD_USE_SPI
    if (!hw || !hw->submit) return;

    LCD_PinState_t cmd_state = { .hw = hw, .pin_state.raw = 0 };
    LCD_PinState_t cplt_state = { .hw = hw, .pin_state.raw = 0 };
    cplt_state.pin_state.bits.cs = 1;

    LCD_SPI_Step_t steps[2];
    LCD_STEP_CMD(0, 0x28);

    LCD_Transaction_t transaction = {
        .steps = steps,
        .step_count = 1,
        .on_complete = LCD_Set_DC_CS,
        .completion_arg = &cplt_state
    };

    hw->submit(hw, &transaction);
    hw->delay_ms(50);
    hw->set_bl(0);

    LCD_STEP_CMD(0, 0x10);
    hw->submit(hw, &transaction);
    #elif LCD_USE_FSMC
    if (!hw || !hw->sendCmd || !hw->sendData) return;
    hw->sendCmd(hw, 0x28);
    hw->delay_ms(50);
    hw->set_bl(0);
    hw->sendCmd(hw, 0x10);
    #endif
    hw->delay_ms(120);
    LCD_Reset(hw);
}

void LCD_Reset(const LCD_HW_Interface_t* hw) {
    if(hw && hw->set_rst) {
        hw->set_rst(LCD_PIN_RESET);
        hw->delay_ms(100);
        hw->set_rst(LCD_PIN_SET);
        hw->delay_ms(100);
    }
}

__attribute__((optimize("O0")))
void LCD_SetWindow(const LCD_HW_Interface_t* hw, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    #ifdef LCD_USE_SPI
    if (!hw || !hw->submit) return;

    LCD_SPI_Step_t steps[5];
    uint8_t col_data[4] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    uint8_t row_data[4] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};

    LCD_PinState_t cmd_state = {.hw = hw, .pin_state.raw = 0};
    LCD_PinState_t data_state = {.hw = hw, .pin_state.raw = 0};
    LCD_PinState_t cplt_state = {.hw = hw, .pin_state.raw = 0};

    cplt_state.pin_state.bits.cs = 1;
    data_state.pin_state.bits.dc = 1;

    LCD_STEP_CMD(0, 0x2A);
    LCD_STEP_DATA_BUF(1, col_data);
    LCD_STEP_CMD(2, 0x2B);
    LCD_STEP_DATA_BUF(3, row_data);
    LCD_STEP_CMD(4, 0x2C);

    LCD_Transaction_t transaction = {
        .steps = steps,
        .step_count = 5,
        .on_complete = LCD_Set_DC_CS,
        .completion_arg = &cplt_state
    };
    hw->submit(hw, &transaction);
    #elif LCD_USE_FSMC
    if (!hw || !hw->sendCmd || !hw->sendData) return;
    uint8_t col_data[4] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    uint8_t row_data[4] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
    hw->sendCmd(hw, 0x2A);
    hw->sendData(hw, col_data, 4);
    hw->sendCmd(hw, 0x2B);
    hw->sendData(hw, row_data, 4);
    hw->sendCmd(hw, 0x2C);
    #endif
}

void LCD_WriteBuffer(const LCD_HW_Interface_t* hw, const uint8_t* buffer, uint32_t length) {
    #ifdef LCD_USE_SPI
    if (!hw || !hw->submit) return;

    uint32_t offset = 0;
    while (offset < length) {
        uint32_t block_size = (length - offset) > TRANSMIT_BLOCK_SIZE ?
                             TRANSMIT_BLOCK_SIZE : (length - offset);

        LCD_PinState_t data_state = {.hw = hw, .pin_state.raw = 0};
        LCD_PinState_t cplt_state = {.hw = hw, .pin_state.raw = 0};
        LCD_SPI_Step_t step;
        LCD_Transaction_t transaction;

        LCD_MAKE_BUFFER_TRANSACTION(&buffer[offset], block_size, &data_state, &cplt_state, &step, &transaction);
        hw->submit(hw, &transaction);

        offset += block_size;
    }
    #elif LCD_USE_FSMC
    if (!hw || !hw->sendData) return;
    uint32_t offset = 0;
    while (offset < length) {
        uint32_t block_size = (length - offset) > TRANSMIT_BLOCK_SIZE ?
                             TRANSMIT_BLOCK_SIZE : (length - offset);
        hw->sendData(hw, &buffer[offset], block_size);
        offset += block_size;
    }
    #endif
}

__attribute__((optimize("O0")))
void LCD_SetRotation(const LCD_HW_Interface_t* hw, LCD_rotation_t rotation)
{
    #ifdef LCD_USE_SPI
    if (!hw || !hw->submit) return;
    if(rotation != ROTATION_0 && rotation != ROTATION_90 && rotation != ROTATION_180 && rotation != ROTATION_270) return;
    ((LCD_RuntimeData_t*)hw->runtime_data)->rotation = rotation;

    uint8_t madctl_data = 0;
    switch (rotation) {
        case ROTATION_0:  madctl_data = MADCTL_MX | MADCTL_MY | MADCTL_RGB; break;
        case ROTATION_90: madctl_data = MADCTL_MV | MADCTL_MY | MADCTL_RGB; break;
        case ROTATION_180: madctl_data = MADCTL_RGB; break;
        case ROTATION_270: madctl_data = MADCTL_MV | MADCTL_MX | MADCTL_RGB; break;
    }

    LCD_PinState_t cmd_state = {.hw = hw, .pin_state.raw = 0};
    LCD_PinState_t data_state = {.hw = hw, .pin_state.raw = 0};
    LCD_PinState_t cplt_state = {.hw = hw, .pin_state.raw = 0};
    data_state.pin_state.bits.dc = 1;
    cplt_state.pin_state.bits.cs = 1;

    LCD_SPI_Step_t steps[2];
    LCD_Transaction_t transaction = {
        .steps = steps,
        .step_count = 2,
        .on_complete = LCD_Set_DC_CS,
        .completion_arg = &cplt_state
    };

    LCD_STEP_CMD(0, 0x36);
    LCD_STEP_DATA_BYTE(1, &madctl_data);
    hw->submit(hw, &transaction);
    #elif LCD_USE_FSMC
    if (!hw || !hw->sendCmd || !hw->sendData) return;
    if(rotation != ROTATION_0 && rotation != ROTATION_90 && rotation != ROTATION_180 && rotation != ROTATION_270) return;
    ((LCD_RuntimeData_t*)hw->runtime_data)->rotation = rotation;
    uint8_t madctl_data = 0;
    switch (rotation) {
        case ROTATION_0:  madctl_data = MADCTL_MX | MADCTL_MY | MADCTL_RGB; break;
        case ROTATION_90: madctl_data = MADCTL_MV | MADCTL_MY | MADCTL_RGB; break;
        case ROTATION_180: madctl_data = MADCTL_RGB; break;
        case ROTATION_270: madctl_data = MADCTL_MV | MADCTL_MX | MADCTL_RGB; break;
    }
    hw->sendCmd(hw, 0x36);
    hw->sendData(hw, &madctl_data, 1);
    #endif
}

int LCD_GetWidth(const LCD_HW_Interface_t* hw) {
    LCD_rotation_t _rotation = ((LCD_RuntimeData_t*)hw->runtime_data)->rotation;
    switch (_rotation) {
        case ROTATION_0:   return hw->raw_width;
        case ROTATION_90:  return hw->raw_height;
        case ROTATION_180: return hw->raw_width;
        case ROTATION_270: return hw->raw_height;
    }
    return hw->raw_width;
}

int LCD_GetHeight(const LCD_HW_Interface_t* hw) {
    LCD_rotation_t _rotation = ((LCD_RuntimeData_t*)hw->runtime_data)->rotation;
    switch (_rotation) {
        case ROTATION_0:   return hw->raw_height;
        case ROTATION_90:  return hw->raw_width;
        case ROTATION_180: return hw->raw_height;
        case ROTATION_270: return hw->raw_width;
    }
    return hw->raw_height;
}

LCD_rotation_t LCD_GetRotation(const LCD_HW_Interface_t* hw) {
    return ((LCD_RuntimeData_t*)hw->runtime_data)->rotation;
}

void LCD_DrawPixel(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, uint16_t color)
{
    #ifdef LCD_USE_SPI
    if (!hw || !hw->submit) return;
    #elif LCD_USE_FSMC
    if (!hw || !hw->sendCmd || !hw->sendData) return;
    #endif

    uint16_t width  = LCD_GetWidth(hw);
    uint16_t height = LCD_GetHeight(hw);
    if (x >= width || y >= height) return;

    switch (LCD_GetRotation(hw)) {
        case ROTATION_0: break;
        case ROTATION_90: { uint16_t t = x; x = y; y = width - 1 - t; break; }
        case ROTATION_180: x = width - 1 - x; y = height - 1 - y; break;
        case ROTATION_270: { uint16_t t = x; x = height - 1 - y; y = t; break; }
    }

    LCD_SetWindow(hw, x, y, x, y);

    color = SWAP_BYTES(color);
    uint8_t pixel[2] = { color >> 8, color & 0xFF };

    #if LCD_USE_SPI
    LCD_PinState_t data_state = {.hw = hw, .pin_state.raw = 0};
    LCD_PinState_t cplt_state = {.hw = hw, .pin_state.raw = 0};
    data_state.pin_state.bits.dc = 1;
    cplt_state.pin_state.bits.cs = 1;

    LCD_SPI_Step_t steps[1];
    steps[0].feedforward = LCD_Set_DC_CS;
    steps[0].ff_arg = &data_state;
    steps[0].tx_buf = pixel;
    steps[0].length = 2;
    steps[0].callback = NULL;
    steps[0].cb_arg = NULL;

    LCD_Transaction_t transaction = {
        .steps = steps,
        .step_count = 1,
        .on_complete = LCD_Set_DC_CS,
        .completion_arg = &cplt_state
    };
    hw->submit(hw, &transaction);
    #elif LCD_USE_FSMC
    hw->sendData(hw, pixel, 2);
    #endif
}

void LCD_FillRect(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= LCD_GetWidth(hw) || y >= LCD_GetHeight(hw)) return;
    if (x + w > LCD_GetWidth(hw))  w = LCD_GetWidth(hw) - x;
    if (y + h > LCD_GetHeight(hw)) h = LCD_GetHeight(hw) - y;

    LCD_SetWindow(hw, x, y, x + w - 1, y + h - 1);
    color = SWAP_BYTES(color);
    uint32_t line_size = w * 2;
    uint8_t *line_buf = (uint8_t *)malloc(line_size);
    if (line_buf == NULL) return;

    uint8_t high = color >> 8;
    uint8_t low  = color & 0xFF;
    for (uint32_t i = 0; i < line_size; i += 2) {
        line_buf[i]   = high;
        line_buf[i+1] = low;
    }

    for (uint16_t row = 0; row < h; row++) {
        LCD_WriteBuffer(hw, line_buf, line_size);
    }

    free(line_buf);
}

void LCD_ClearScreen(const LCD_HW_Interface_t* hw, uint16_t color) {
    LCD_FillRect(hw, 0, 0, LCD_GetWidth(hw), LCD_GetHeight(hw), color);
}

void LCD_DrawRect(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= LCD_GetWidth(hw) || y >= LCD_GetHeight(hw)) return;
    if (w == 0 || h == 0) return;
    if (x + w > LCD_GetWidth(hw))  w = LCD_GetWidth(hw) - x;
    if (y + h > LCD_GetHeight(hw)) h = LCD_GetHeight(hw) - y;

    if (w < 3 || h < 3) {
        LCD_FillRect(hw, x, y, w, h, color);
        return;
    }
    color = SWAP_BYTES(color);
    uint16_t x_end = x + w - 1;
    uint16_t y_end = y + h - 1;

    LCD_FillRect(hw, x, y, w, 1, color);
    LCD_FillRect(hw, x, y_end, w, 1, color);
    if (h > 2) {
        LCD_FillRect(hw, x, y + 1, 1, h - 2, color);
        LCD_FillRect(hw, x_end, y + 1, 1, h - 2, color);
    }
}

void LCD_DrawLine(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, uint16_t x1, uint16_t y1, uint16_t color) {
    if (y == y1) {
        uint16_t width = (x1 > x) ? (x1 - x + 1) : (x - x1 + 1);
        uint16_t startX = (x < x1) ? x : x1;
        LCD_FillRect(hw, startX, y, width, 1, color);
    } else if (x == x1) {
        uint16_t height = (y1 > y) ? (y1 - y + 1) : (y - y1 + 1);
        uint16_t startY = (y < y1) ? y : y1;
        LCD_FillRect(hw, x, startY, 1, height, color);
    } else {
        int dx = (x1 > x) ? (x1 - x) : (x - x1);
        int dy = (y1 > y) ? (y1 - y) : (y - y1);
        int sx = (x < x1) ? 1 : -1;
        int sy = (y < y1) ? 1 : -1;
        int err = dx - dy;

        while (1) {
            LCD_DrawPixel(hw, x, y, color);
            if (x == x1 && y == y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x += sx; }
            if (e2 < dx) { err += dx; y += sy; }
        }
    }
}

void LCD_DrawChar(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg)
{
    if (ch < 32 || ch > 126) return;
    if ((x + 6 - 1) >= LCD_GetWidth(hw) || (y + 8 - 1) >= LCD_GetHeight(hw)) return;

    uint16_t buffer[6 * 8];
    const unsigned char *bitmap = Font6x8[ch - 32];

    for (uint8_t col = 0; col < 6; col++) {
        uint8_t line = bitmap[col];
        for (uint8_t row = 0; row < 8; row++) {
            uint16_t index = row * 6 + col;
            if (line & 0x01) {
                buffer[index] = SWAP_BYTES(color);
            } else {
                buffer[index] = SWAP_BYTES(bg);
            }
            line >>= 1;
        }
    }

    LCD_SetWindow(hw, x, y, x + 6 - 1, y + 8 - 1);
    LCD_WriteBuffer(hw, (uint8_t *)buffer, sizeof(buffer));
}

void LCD_DrawString(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg)
{
    while (*str) {
        if (x + 6 >= LCD_GetWidth(hw)) {
            x = 0;
            y += 8;
        }
        LCD_DrawChar(hw, x, y, *str, color, bg);
        x += 6;
        str++;
    }
}

void LCD_DrawImage(const LCD_HW_Interface_t* hw, int x, int y, int w, int h, const uint16_t *img_data)
{
    if (x >= LCD_GetWidth(hw) || y >= LCD_GetHeight(hw) || (x + w) <= 0 || (y + h) <= 0)
        return;

    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > LCD_GetWidth(hw)) ? (LCD_GetWidth(hw) - 1) : (x + w - 1);
    int y1 = (y + h > LCD_GetHeight(hw)) ? (LCD_GetHeight(hw) - 1) : (y + h - 1);

    int visible_w = x1 - x0 + 1;
    int visible_h = y1 - y0 + 1;

    LCD_SetWindow(hw, x0, y0, x1, y1);

    if (visible_w == w && visible_h == h) {
        LCD_WriteBuffer(hw, (uint8_t *)img_data, w * h * 2);
    } else {
        for (int row = 0; row < visible_h; row++) {
            const uint16_t *line_ptr = img_data + (y0 - y + row) * w + (x0 - x);
            LCD_WriteBuffer(hw, (uint8_t *)line_ptr, visible_w * 2);
        }
    }
}

static inline int xbm_get_pixel(const uint8_t *bitmap, int x, int y, int width)
{
    int bytes_per_row = (width + 7) / 8;
    int byte_index = y * bytes_per_row + (x >> 3);
    int bit_index = x & 7;
    return (bitmap[byte_index] >> bit_index) & 0x01;
}

void LCD_DrawXBM(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
    const uint8_t *im_bits, uint16_t color, uint16_t bg)
{
    LCD_SetWindow(hw, x, y, x + w - 1, y + h - 1);
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            int pixel = xbm_get_pixel(im_bits, col, row, w);
            uint16_t c = pixel ? color : bg;
            LCD_DrawPixel(hw, x + col, y + row, c);
        }
    }
}

void LCD_DrawXBM_Scaled(const LCD_HW_Interface_t* hw, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
    const uint8_t *im_bits, uint16_t color, uint16_t bg, int scale)
{
    if (scale < 1) scale = 1;
    uint16_t px, py;
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            int pixel = xbm_get_pixel(im_bits, col, row, w);
            uint16_t c = pixel ? color : bg;
            px = x + col * scale;
            py = y + row * scale;
            LCD_FillRect(hw, px, py, scale, scale, c);
        }
    }
}
