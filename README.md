<p align="center">
  <h1 align="center">cjz-lcd-driver</h1>
  <p align="center">Platform-agnostic TFT LCD driver library for embedded systems</p>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License MIT">
  <img src="https://img.shields.io/badge/language-C99-green.svg" alt="Language C99">
  <img src="https://img.shields.io/badge/interface-SPI%20%7C%20FSMC%2F8080-orange.svg" alt="SPI | FSMC/8080">
</p>

---

## Overview

`cjz-lcd-driver` is a hardware-agnostic LCD graphics library designed for bare-metal and RTOS embedded systems. It provides a complete 2D drawing pipeline — from low-level bus transactions to high-level shape rendering and off-screen buffering — all while keeping the core logic **completely independent of any specific MCU or toolchain**.

The library separates concerns into three layers:

| Layer | File(s) | Responsibility |
|-------|---------|----------------|
| **Hardware Adapter** | `lcd_implement_template.c` | GPIO, SPI/FSMC, DMA — **the only file you modify** |
| **Core Driver** | `lcd.c` | Init sequences, window setup, pixel ops, rotation — zero hw code |
| **Graphics / Buffer** | `lcd_buffer.c`, `lcd_buffer_draw.c` | Off-screen rendering, shapes, fonts, dirty-rect compositing |

> **Write once, run anywhere.** Port to a new MCU? Implement ~7 functions. Done.

## Features

- **Dual interface support** — SPI (4-wire: SCK/MOSI/DC/CS) and FSMC/8080 parallel bus
- **Full 2D drawing API**
  - Pixels, lines (Bresenham), rectangles, filled rectangles
  - Triangles (wireframe & filled), polygons (wireframe & filled, AET scanline)
  - Circles & ellipses (wireframe & filled, midpoint algorithm)
  - Anti-aliased lines (brush method)
- **Text rendering** — 6×8 ASCII font built in, with Chinese character example
- **Image display** — RGB565 blit with clipping; XBM (1bpp) with integer scaling
- **Rotation** — 0°, 90°, 180°, 270° with coordinate remapping
- **Off-screen buffers** — create movable buffers with automatic dirty-rect cleanup for sprite-like compositing
- **Configurable** — resolution, color inversion, byte swap, RGB/BGR, horizontal/vertical flip all in one header
- **Zero dynamic allocation for core driver** — static local heap; dynamic allocation only for user-created buffers
- **No external dependencies** — `stdint.h`, `stdlib.h`, `string.h` only

## Supported Controllers

Currently ships with a **ST7789** initialization sequence. Adding support for other controllers (ILI9341, ST7735, GC9A01, etc.) requires only adding a new init sequence table in `lcd.c`.

## Quick Start

### 1. Clone

```bash
git clone https://github.com/cjznb/cjz-lcd-driver.git
```

### 2. Configure

Edit `inc/lcd_user_config.h`:

```c
// Choose interface: SPI or FSMC
#define LCD_USE_SPI     1
// #define LCD_USE_FSMC    1

// Your LCD panel dimensions
#define LCD_RAW_WIDTH   240
#define LCD_RAW_HEIGHT  320
```

### 3. Implement the hardware adapter

Open `src/lcd_implement_template.c`. Every function marked with `TODO` requires your implementation. Each function includes detailed Chinese documentation.

**SPI mode — 7 functions**:

| Function | What you implement |
|----------|-------------------|
| `MyLCD_Init()` | GPIO init, SPI peripheral init, DMA config |
| `MyLCD_setRST()` | Drive reset pin high/low |
| `MyLCD_setBL()` | Drive backlight pin high/low |
| `MyLCD_setDC()` | Drive data/command pin high/low |
| `MyLCD_setCS()` | Drive chip select pin high/low |
| `MyLCD_delay_ms()` | Blocking millisecond delay |
| `MyLCD_submit()` | **Core**: iterate SPI transaction steps, send via polling or DMA |

**FSMC mode — 6 functions**: `MyLCD_Init`, `MyLCD_setRST`, `MyLCD_setBL`, `MyLCD_delay_ms`, `MyLCD_WriteCMD`, `MyLCD_WriteData`

### 4. Integrate with your project

Add `inc/` to your include path (`-I inc`) and compile all `.c` files in `src/`. The core driver accesses hardware exclusively through the `lcd_hw_interface` pointer — no hardware headers are needed in the core.

### 5. Draw something

```c
#include "lcd.h"

int main(void) {
    // lcd_hw_interface is defined in lcd_implement_template.c
    LCD_Init((LCD_HW_Interface_t*)lcd_hw_interface);

    LCD_ClearScreen(lcd_hw_interface, COLOR_BLACK);
    LCD_DrawString(lcd_hw_interface, 10, 10, "Hello World!", COLOR_WHITE, COLOR_BLACK);
    LCD_FillRect(lcd_hw_interface, 50, 50, 100, 60, COLOR_RED);
    LCD_DrawCircleBuffer(my_buffer, 64, 64, 30, COLOR_BLUE);

    while (1);
}
```

## Porting Guide

Porting to a new MCU platform involves exactly one file: `src/lcd_implement_template.c`.

### Step-by-step

1. **Read the template** — each function has `@note` comments describing exactly what to do, with pseudo-code examples.

2. **Implement basic GPIO functions first** — `MyLCD_setRST`, `MyLCD_setBL`, `MyLCD_setDC`, `MyLCD_setCS`, `MyLCD_delay_ms`. These are straightforward pin toggles.

3. **Implement initialization** — `MyLCD_Init`: configure your GPIOs, SPI/FSMC peripheral, and optionally DMA.

4. **Implement the data path** — this is the most important step:

   **For SPI**: implement `MyLCD_submit()`. The function receives a `LCD_Transaction_t*` containing an array of `LCD_SPI_Step_t`. For each step:
   - Call `step->feedforward()` if present (handles DC/CS switching)
   - Send `step->tx_buf` of `step->length` bytes via SPI (polling for small data, DMA for large)
   - Call `step->callback()` if present
   - After all steps, call `transaction->on_complete()` if present
   - The function must block until everything is done

   **For FSMC**: implement `MyLCD_WriteCMD()` and `MyLCD_WriteData()`. These are direct register writes to mapped addresses.

5. **Verify** — compile, flash, and check if `LCD_Init()` produces a black screen (correct) and subsequent drawing commands appear correctly.

### Example SPI submit (pseudo-code)

```c
static void MyLCD_submit(const LCD_HW_Interface_t *hw,
                         const LCD_Transaction_t *transaction)
{
    for (int i = 0; i < transaction->step_count; i++) {
        LCD_SPI_Step_t *step = &transaction->steps[i];

        if (step->feedforward)
            step->feedforward(step->ff_arg);

        // Send data — use DMA for large payloads
        if (step->length >= DMA_THRESHOLD)
            spi_dma_send(step->tx_buf, step->length);
        else
            spi_poll_send(step->tx_buf, step->length);

        if (step->callback)
            step->callback(step->cb_arg);
    }
    if (transaction->on_complete)
        transaction->on_complete(transaction->completion_arg);
}
```

## Off-Screen Buffer API

The library supports off-screen rendering for flicker-free updates:

```c
// Create a 64x64 buffer at screen position (100, 50)
LCD_Buffer_t *buf = LCD_BufferMake(lcd_hw_interface, 100, 50, 64, 64);

// Draw into the buffer (not visible yet)
LCD_FillCircleBuffer(buf, 32, 32, 30, COLOR_BLUE);
LCD_DrawStringBuffer(buf, 5, 5, "OK", COLOR_WHITE, COLOR_BLUE);

// Flush to screen
LCD_SendBuffer(lcd_hw_interface, buf);

// Cleanup
LCD_BufferDestroy(buf);
```

**Movable buffers** support sprite-like animation with automatic background restoration:

```c
LCD_MoveableBuffer_t *sprite = LCD_MakeMoveable(lcd_hw_interface, 0, 0, 32, 32);
// Draw sprite content...
LCD_BufferMove(sprite, new_x, new_y, bg_color);  // marks dirty regions
LCD_SendBuffer(lcd_hw_interface, sprite);          // auto-restores background
```

## API Reference

### Core (lcd.h)

| Function | Description |
|----------|-------------|
| `LCD_Init(hw)` | Initialize LCD controller with ST7789 sequence |
| `LCD_DeInit(hw)` | Power down display and backlight |
| `LCD_Reset(hw)` | Hardware reset via RST pin |
| `LCD_SetWindow(hw, x0, y0, x1, y1)` | Define the active pixel region |
| `LCD_WriteBuffer(hw, buf, len)` | Stream pixel data to the active window |
| `LCD_DrawPixel(hw, x, y, color)` | Set a single pixel |
| `LCD_DrawLine(hw, x0, y0, x1, y1, color)` | Bresenham line |
| `LCD_DrawRect(hw, x, y, w, h, color)` | Rectangle outline |
| `LCD_FillRect(hw, x, y, w, h, color)` | Filled rectangle |
| `LCD_ClearScreen(hw, color)` | Fill entire screen |
| `LCD_DrawChar(hw, x, y, ch, fg, bg)` | 6×8 ASCII character |
| `LCD_DrawString(hw, x, y, str, fg, bg)` | String with auto-wrap |
| `LCD_DrawImage(hw, x, y, w, h, data)` | RGB565 image with clipping |
| `LCD_DrawXBM(hw, x, y, w, h, bits, fg, bg)` | XBM bitmap |
| `LCD_DrawXBM_Scaled(hw, ..., scale)` | XBM with integer scaling |
| `LCD_SetRotation(hw, rot)` | Set rotation (0/90/180/270) |
| `LCD_GetRotation(hw)` | Get current rotation |
| `LCD_GetWidth(hw)` / `LCD_GetHeight(hw)` | Get dimensions (respects rotation) |

### Buffer (lcd_buffer_draw.h)

| Function | Description |
|----------|-------------|
| `LCD_BufferMake(hw, x, y, w, h)` | Create off-screen buffer |
| `LCD_BufferDestroy(buf)` | Free buffer memory |
| `LCD_BufferClear(buf, color)` | Fill buffer with solid color |
| `LCD_SendBuffer(hw, buf)` | Flush buffer to screen (with clipping & dirty rect cleanup) |
| `LCD_MakeMoveable(hw, x, y, w, h)` | Create movable buffer for sprite animation |
| `LCD_BufferMove(buf, tx, ty, bg)` | Move buffer, mark dirty regions |
| `LCD_FillRectBuffer(buf, x, y, w, h, c)` | Fill rectangle in buffer |
| `LCD_DrawRectangleBuffer(buf, x, y, w, h, c)` | Rectangle outline in buffer |
| `LCD_DrawLineBuffer(buf, x0, y0, x1, y1, c)` | Line in buffer |
| `LCD_DrawCircleBuffer(buf, cx, cy, r, c)` | Circle outline in buffer |
| `LCD_FillCircleBuffer(buf, cx, cy, r, c)` | Filled circle in buffer |
| `LCD_DrawEllipseBuffer(buf, cx, cy, a, b, c)` | Ellipse outline in buffer |
| `LCD_FillEllipseBuffer(buf, cx, cy, a, b, c)` | Filled ellipse in buffer |
| `LCD_DrawTriangleBuffer(buf, ...)` | Triangle outline in buffer |
| `LCD_FillTriangleBuffer(buf, ...)` | Filled triangle (flat-top/bottom split) |
| `LCD_DrawPolygonBuffer(buf, n, x[], y[], c)` | Polygon outline in buffer |
| `LCD_FillPolygonBuffer(buf, n, x[], y[], c)` | Filled polygon (AET scanline) |
| `LCD_DrawAALineBuffer(buf, x0, y0, x1, y1, w, c)` | Anti-aliased line |
| `LCD_DrawCharBuffer(buf, x, y, ch, fg, bg)` | Character in buffer |
| `LCD_DrawStringBuffer(buf, x, y, str, fg, bg)` | String in buffer (supports `\n`) |
| `LCD_DrawImageBuffer(buf, x, y, w, h, data)` | Image in buffer |
| `LCD_DrawXBMBuffer(buf, x, y, w, h, bits, fg, bg)` | XBM in buffer |
| `LCD_DrawXBM_ScaledBuffer(buf, ..., scale)` | Scaled XBM in buffer |

## Colors

Defined in `inc/st7789_cmd.h` (RGB565):

| Name | Value | Swatch |
|------|-------|--------|
| `COLOR_BLACK` | `0x0000` | ■ |
| `COLOR_WHITE` | `0xFFFF` | □ |
| `COLOR_RED` | `0xF800` | ■ |
| `COLOR_GREEN` | `0x07E0` | ■ |
| `COLOR_BLUE` | `0x001F` | ■ |
| `COLOR_YELLOW` | `0xFFE0` | ■ |
| `COLOR_CYAN` | `0x07FF` | ■ |
| `COLOR_MAGENTA` | `0xF81F` | ■ |
| `COLOR_ORANGE` | `0xFD20` | ■ |
| `COLOR_GRAY` | `0x8410` | ■ |

## Project Structure

```
cjz-lcd-driver/
├── inc/
│   ├── lcd.h                    # Core driver API
│   ├── lcd_interface.h          # Hardware abstraction interface
│   ├── lcd_user_config.h        # User configuration
│   ├── st7789_cmd.h             # ST7789 registers + color definitions
│   └── lcd_buffer_draw.h        # Off-screen buffer & drawing API
├── src/
│   ├── lcd_implement_template.c # ★ HARDWARE ADAPTER — YOU EDIT THIS
│   ├── lcd.c                    # Core driver (init, drawing, rotation)
│   ├── lcd_buffer.c             # Buffer management (create/destroy/send/moveable)
│   ├── lcd_buffer_draw.c        # Buffer drawing (shapes, fonts, images)
│   ├── font.c                   # 6x8 ASCII font + 16x16 Chinese font example
├── LICENSE                       # MIT
├── README.md                     # This file
└── README_CN.md                  # Chinese version
```

## FAQ

**Q: Which MCUs are supported?**
A: All of them. The core driver has zero platform dependencies. You only need to implement the hardware adapter for your specific MCU.

**Q: Does it work with RTOS?**
A: Yes. The library is blocking/synchronous. Run it in a dedicated task or wrap calls with your RTOS primitives.

**Q: How do I add support for a different LCD controller?**
A: Add a new `#elif defined(LCD_DRIVER_CHIP_XXX)` block in `inc/lcd_init_seq.h` with your controller's init sequence, then select the chip in `inc/lcd_user_config.h`. Optionally add a new register definition header (e.g. `inc/st7735_cmd.h`).

**Q: Why is `lcd_implement_template.c` not "clean" C?**
A: It is intentionally a template — the function bodies are empty stubs with detailed Chinese documentation. This makes the porting requirements immediately obvious.

**Q: Can I use this with Arduino?**
A: Yes. Wrap `digitalWrite`, `SPI.transfer`, `delay` etc. inside the adapter functions.

## License

MIT — see [LICENSE](LICENSE) for full text.

## Acknowledgments

Font data adapted from common embedded font libraries. ST7789 init sequence derived from the ST7789VW datasheet.
