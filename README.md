# cjz-lcd-driver

A hardware-agnostic LCD driver abstraction layer for embedded systems, supporting both SPI (4-wire) and FSMC (8080 parallel) interfaces.

## Overview

`cjz-lcd-driver` provides a clean hardware abstraction layer (HAL) for driving TFT LCD displays on microcontrollers. It separates the high-level LCD command logic (init sequences, drawing primitives, font rendering) from the low-level hardware implementation, allowing the same driver core to work across different MCU platforms.

### Supported Interfaces

| Interface | Description |
|-----------|-------------|
| **SPI** (4-wire) | SCK + MOSI + DC (data/command) + CS (chip select) |
| **FSMC / 8080** | Parallel interface with separate command/data address space |

## Features

- **Platform-independent**: Core driver logic has zero hardware dependencies
- **Template-based**: `lcd_implement_template.c` with Chinese documentation guides users through hardware adaptation
- **SPI transaction model**: Define multi-step SPI sequences with pre/post callbacks
- **FSMC support**: Direct address-mapped command/data register access
- **Configurable**: All LCD parameters (resolution, orientation, color inversion) in a single header

## Project Structure

```
cjz-lcd-driver/
├── inc/
│   ├── lcd_interface.h          # Hardware abstraction interface definition
│   └── lcd_user_config.h        # User configuration (resolution, interface type, etc.)
├── src/
│   └── lcd_implement_template.c # Hardware adaptation template (user implements TODO functions)
├── LICENSE                       # MIT License
└── README.md                     # This file
```

## Quick Start

### 1. Configure your LCD

Edit `inc/lcd_user_config.h`:

```c
#define LCD_USE_SPI     1       // Choose SPI or FSMC
#define LCD_RAW_WIDTH   240     // Your LCD width
#define LCD_RAW_HEIGHT  320     // Your LCD height
```

### 2. Implement hardware functions

Open `src/lcd_implement_template.c` and implement all functions marked with `TODO`. Each function includes Chinese documentation describing what needs to be done.

**Must-implement functions (SPI mode):**
- `MyLCD_Init()` — Initialize GPIOs, SPI peripheral, DMA
- `MyLCD_setRST()` — Control reset pin
- `MyLCD_setBL()` — Control backlight pin
- `MyLCD_setDC()` — Control data/command pin
- `MyLCD_setCS()` — Control chip select pin
- `MyLCD_delay_ms()` — Millisecond delay
- `MyLCD_submit()` — Submit SPI transaction (core function)

**Must-implement functions (FSMC mode):**
- `MyLCD_Init()` — Initialize GPIOs, FSMC peripheral
- `MyLCD_setRST()` — Control reset pin
- `MyLCD_setBL()` — Control backlight pin
- `MyLCD_delay_ms()` — Millisecond delay
- `MyLCD_WriteCMD()` — Write command via FSMC
- `MyLCD_WriteData()` — Write data via FSMC (with optional DMA)

### 3. Integrate with driver core

The driver core accesses hardware through the global `lcd_hw_interface` pointer. Once your hardware functions are implemented, the core driver can work without modification.

## Hardware Adaptation Guide

The template file uses a clear pattern:

```c
static int MyLCD_Init(void)
{
    // TODO: 【User Implementation】Add hardware initialization code here
    // 1. Configure GPIO pins (RST, BL, DC, CS)
    // 2. Initialize SPI/FSMC peripheral
    // 3. Configure DMA if needed
    return 0;
}
```

Each function includes:
- **Purpose**: What the function does
- **Implementation notes**: Detailed description with pseudo-code examples
- **TODO marker**: Where to add your code

## License

MIT — see [LICENSE](LICENSE) for details.

## Contributing

Issues and pull requests are welcome. Please ensure your contributions maintain the platform-agnostic design philosophy.
