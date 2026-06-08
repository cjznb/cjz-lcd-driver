<p align="center">
  <h1 align="center">cjz-lcd-driver</h1>
  <p align="center">嵌入式平台无关 TFT LCD 驱动库</p>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License MIT">
  <img src="https://img.shields.io/badge/language-C99-green.svg" alt="Language C99">
  <img src="https://img.shields.io/badge/interface-SPI%20%7C%20FSMC%2F8080-orange.svg" alt="SPI | FSMC/8080">
</p>

---

## 概述

`cjz-lcd-driver` 是一款硬件无关的嵌入式 TFT LCD 图形驱动库，适用于裸机和 RTOS 环境。它提供从底层总线通信到高层图形渲染、离屏缓冲的完整 2D 绘制管线，**核心逻辑不依赖任何特定 MCU 或工具链**。

库的三层架构：

| 层次 | 文件 | 职责 |
|------|------|------|
| **硬件适配层** | `lcd_implement_template.c` | GPIO、SPI/FSMC、DMA — **移植时唯一需要修改的文件** |
| **核心驱动层** | `lcd.c` | 初始化序列、窗口设置、像素操作、旋转 — 零硬件代码 |
| **图形/缓冲层** | `lcd_buffer.c`, `lcd_buffer_draw.c` | 离屏渲染、几何图形、字体、脏矩形合成 |

> **一次编写，处处运行。** 换一款 MCU？只需实现约 7 个函数即可。

## 功能特性

- **双接口支持** — SPI（4线：SCK/MOSI/DC/CS）和 FSMC/8080 并行总线
- **完整 2D 绘图 API**
  - 点、线（Bresenham算法）、矩形边框、矩形填充
  - 三角形（边框 & 填充）、多边形（边框 & 填充，AET扫描线算法）
  - 圆 & 椭圆（边框 & 填充，中点算法）
  - 抗锯齿线段（笔刷法）
- **文字渲染** — 内置 6×8 ASCII 字体，附带 16×16 汉字示例
- **图像显示** — RGB565 位图传输（带裁剪）；XBM 单色位图（支持整数倍缩放）
- **屏幕旋转** — 支持 0°/90°/180°/270°，自动重映射坐标
- **离屏缓冲区** — 创建可移动缓冲区，自动脏矩形清理，实现类精灵合成
- **集中配置** — 分辨率、颜色反转、字节交换、RGB/BGR、水平/垂直翻转全部在一个头文件中
- **核心驱动零动态内存分配** — 使用静态本地堆；仅用户创建的缓冲区使用 `malloc`
- **无外部依赖** — 仅需 `stdint.h`、`stdlib.h`、`string.h`

## 支持的LCD控制器

当前内置 **ST7789** 初始化序列。如需支持其他控制器（ILI9341、ST7735、GC9A01 等），仅需在 `lcd.c` 中添加对应的初始化序列表。

## 快速开始

### 1. 克隆仓库

```bash
git clone https://github.com/cjznb/cjz-lcd-driver.git
```

### 2. 配置参数

编辑 `inc/lcd_user_config.h`：

```c
// 选择接口类型：SPI 或 FSMC（二选一）
#define LCD_USE_SPI     1
// #define LCD_USE_FSMC    1

// LCD 屏幕物理分辨率
#define LCD_RAW_WIDTH   240
#define LCD_RAW_HEIGHT  320
```

### 3. 实现硬件适配函数

打开 `src/lcd_implement_template.c`，实现所有标记为 `TODO` 的函数。每个函数都附有详细的中文实现说明和伪代码示例。

**SPI 模式需实现 7 个函数：**

| 函数 | 需要实现的内容 |
|------|---------------|
| `MyLCD_Init()` | GPIO 初始化、SPI 外设初始化、DMA 配置 |
| `MyLCD_setRST()` | 控制复位引脚电平（高/低） |
| `MyLCD_setBL()` | 控制背光引脚电平（开/关） |
| `MyLCD_setDC()` | 控制数据/命令选择引脚（数据/命令） |
| `MyLCD_setCS()` | 控制片选引脚电平（选中/取消） |
| `MyLCD_delay_ms()` | 毫秒级阻塞延时 |
| `MyLCD_submit()` | **核心函数**：遍历 SPI 事务步骤，通过轮询或 DMA 发送数据 |

**FSMC 模式需实现 6 个函数：** `MyLCD_Init`、`MyLCD_setRST`、`MyLCD_setBL`、`MyLCD_delay_ms`、`MyLCD_WriteCMD`、`MyLCD_WriteData`

### 4. 集成到工程

将 `inc/` 添加到编译器的头文件搜索路径（`-I inc`），编译 `src/` 下所有 `.c` 文件。核心驱动仅通过 `lcd_hw_interface` 指针访问硬件，无需任何硬件相关头文件。

### 5. 开始绘制

```c
#include "lcd.h"

int main(void) {
    // lcd_hw_interface 在 lcd_implement_template.c 中定义
    LCD_Init((LCD_HW_Interface_t*)lcd_hw_interface);

    LCD_ClearScreen(lcd_hw_interface, COLOR_BLACK);
    LCD_DrawString(lcd_hw_interface, 10, 10, "你好，世界！", COLOR_WHITE, COLOR_BLACK);
    LCD_FillRect(lcd_hw_interface, 50, 50, 100, 60, COLOR_RED);

    while (1);
}
```

## 移植指南

将本库移植到新 MCU 平台只需修改一个文件：`src/lcd_implement_template.c`。

### 移植步骤

1. **阅读模板文件** — 每个函数都有 `@note` 注释，详细描述了实现要点，并附有伪代码示例。

2. **先实现简单 GPIO 函数** — `MyLCD_setRST`、`MyLCD_setBL`、`MyLCD_setDC`、`MyLCD_setCS`、`MyLCD_delay_ms`。这几个函数就是简单的引脚电平控制。

3. **实现初始化函数** — `MyLCD_Init`：配置 GPIO、SPI/FSMC 外设，可选配置 DMA。

4. **实现数据通路（最关键的一步）**：

   **SPI 模式**：实现 `MyLCD_submit()`。该函数接收一个 `LCD_Transaction_t*` 指针，其中包含一组 `LCD_SPI_Step_t`。对每个步骤：
   - 若 `step->feedforward` 存在，调用它（用于切换 DC/CS 引脚状态）
   - 通过 SPI 发送 `step->tx_buf`，长度为 `step->length` 字节（小数据量用轮询，大数据量用 DMA）
   - 若 `step->callback` 存在，发送完成后调用它
   - 所有步骤完成后，若 `transaction->on_complete` 存在，调用它
   - 该函数必须阻塞直到所有操作完成

   **FSMC 模式**：实现 `MyLCD_WriteCMD()` 和 `MyLCD_WriteData()`。直接向映射地址写入数据即可。

5. **验证** — 编译、烧录，检查 `LCD_Init()` 是否显示黑屏（正确），后续绘制命令是否正常显示。

### SPI 事务提交伪代码

```c
static void MyLCD_submit(const LCD_HW_Interface_t *hw,
                         const LCD_Transaction_t *transaction)
{
    for (int i = 0; i < transaction->step_count; i++) {
        LCD_SPI_Step_t *step = &transaction->steps[i];

        // 步骤前馈：切换 DC/CS
        if (step->feedforward)
            step->feedforward(step->ff_arg);

        // 发送数据：大数据量用 DMA，小数据量用轮询
        if (step->length >= DMA_THRESHOLD)
            spi_dma_send(step->tx_buf, step->length);
        else
            spi_poll_send(step->tx_buf, step->length);

        // 步骤回调
        if (step->callback)
            step->callback(step->cb_arg);
    }

    // 事务完成回调：拉高 CS 等
    if (transaction->on_complete)
        transaction->on_complete(transaction->completion_arg);
}
```

## 离屏缓冲区 API

本库支持离屏渲染，实现无闪烁画面更新：

```c
// 在屏幕坐标 (100, 50) 处创建 64×64 的缓冲区
LCD_Buffer_t *buf = LCD_BufferMake(lcd_hw_interface, 100, 50, 64, 64);

// 在缓冲区中绘制（此时屏幕不变）
LCD_FillCircleBuffer(buf, 32, 32, 30, COLOR_BLUE);
LCD_DrawStringBuffer(buf, 5, 5, "OK", COLOR_WHITE, COLOR_BLUE);

// 一次性刷新到屏幕
LCD_SendBuffer(lcd_hw_interface, buf);

// 释放
LCD_BufferDestroy(buf);
```

**可移动缓冲区** 支持精灵（Sprite）式动画，自动恢复背景：

```c
LCD_MoveableBuffer_t *sprite = LCD_MakeMoveable(lcd_hw_interface, 0, 0, 32, 32);
// 绘制精灵内容...
LCD_BufferMove(sprite, new_x, new_y, bg_color);  // 标记残影区域
LCD_SendBuffer(lcd_hw_interface, sprite);          // 自动清理背景残留
```

## API 参考

### 核心函数 (lcd.h)

| 函数 | 说明 |
|------|------|
| `LCD_Init(hw)` | 初始化 LCD 控制器（ST7789 初始化序列） |
| `LCD_DeInit(hw)` | 关闭显示和背光 |
| `LCD_Reset(hw)` | 通过 RST 引脚硬件复位 |
| `LCD_SetWindow(hw, x0, y0, x1, y1)` | 设置当前操作的像素区域 |
| `LCD_WriteBuffer(hw, buf, len)` | 向当前窗口写入像素数据 |
| `LCD_DrawPixel(hw, x, y, color)` | 绘制单个像素 |
| `LCD_DrawLine(hw, x0, y0, x1, y1, color)` | Bresenham 直线 |
| `LCD_DrawRect(hw, x, y, w, h, color)` | 矩形边框 |
| `LCD_FillRect(hw, x, y, w, h, color)` | 填充矩形 |
| `LCD_ClearScreen(hw, color)` | 全屏填充 |
| `LCD_DrawChar(hw, x, y, ch, fg, bg)` | 6×8 ASCII 字符 |
| `LCD_DrawString(hw, x, y, str, fg, bg)` | 字符串（自动换行） |
| `LCD_DrawImage(hw, x, y, w, h, data)` | RGB565 图片（带裁剪） |
| `LCD_DrawXBM(hw, x, y, w, h, bits, fg, bg)` | XBM 单色位图 |
| `LCD_DrawXBM_Scaled(hw, ..., scale)` | XBM 位图（整数倍缩放） |
| `LCD_SetRotation(hw, rot)` | 设置旋转角度（0/90/180/270） |
| `LCD_GetRotation(hw)` | 获取当前旋转角度 |
| `LCD_GetWidth(hw)` / `LCD_GetHeight(hw)` | 获取当前宽高（随旋转变化） |

### 缓冲区函数 (lcd_buffer_draw.h)

| 函数 | 说明 |
|------|------|
| `LCD_BufferMake(hw, x, y, w, h)` | 创建离屏缓冲区 |
| `LCD_BufferDestroy(buf)` | 释放缓冲区 |
| `LCD_BufferClear(buf, color)` | 单色填充缓冲区 |
| `LCD_SendBuffer(hw, buf)` | 将缓冲区刷新到屏幕（含裁剪和脏矩形清理） |
| `LCD_MakeMoveable(hw, x, y, w, h)` | 创建可移动缓冲区 |
| `LCD_BufferMove(buf, tx, ty, bg)` | 移动缓冲区并标记残影 |
| `LCD_FillRectBuffer(buf, x, y, w, h, c)` | 缓冲区内填充矩形 |
| `LCD_DrawRectangleBuffer(buf, x, y, w, h, c)` | 缓冲区内矩形边框 |
| `LCD_DrawLineBuffer(buf, x0, y0, x1, y1, c)` | 缓冲区内直线 |
| `LCD_DrawCircleBuffer(buf, cx, cy, r, c)` | 缓冲区内圆形边框 |
| `LCD_FillCircleBuffer(buf, cx, cy, r, c)` | 缓冲区内填充圆 |
| `LCD_DrawEllipseBuffer(buf, cx, cy, a, b, c)` | 缓冲区内椭圆边框 |
| `LCD_FillEllipseBuffer(buf, cx, cy, a, b, c)` | 缓冲区内填充椭圆 |
| `LCD_DrawTriangleBuffer(buf, ...)` | 缓冲区内三角形边框 |
| `LCD_FillTriangleBuffer(buf, ...)` | 缓冲区内填充三角形（平顶/平底拆分） |
| `LCD_DrawPolygonBuffer(buf, n, x[], y[], c)` | 缓冲区内多边形边框 |
| `LCD_FillPolygonBuffer(buf, n, x[], y[], c)` | 缓冲区内填充多边形（AET 扫描线） |
| `LCD_DrawAALineBuffer(buf, x0, y0, x1, y1, w, c)` | 缓冲区内抗锯齿线段 |
| `LCD_DrawCharBuffer(buf, x, y, ch, fg, bg)` | 缓冲区内字符 |
| `LCD_DrawStringBuffer(buf, x, y, str, fg, bg)` | 缓冲区内字符串（支持 `\n`） |
| `LCD_DrawImageBuffer(buf, x, y, w, h, data)` | 缓冲区内图片 |
| `LCD_DrawXBMBuffer(buf, x, y, w, h, bits, fg, bg)` | 缓冲区内 XBM 位图 |
| `LCD_DrawXBM_ScaledBuffer(buf, ..., scale)` | 缓冲区内 XBM 位图缩放 |

## 颜色定义

在 `inc/st7789_cmd.h` 中预定义（RGB565 格式）：

| 名称 | 数值 | 色块 |
|------|------|------|
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

## 项目结构

```
cjz-lcd-driver/
├── inc/
│   ├── lcd.h                    # 核心驱动 API 声明
│   ├── lcd_interface.h          # 硬件抽象接口定义
│   ├── lcd_user_config.h        # 用户配置文件
│   ├── st7789_cmd.h             # ST7789 寄存器定义 + RGB565 颜色宏
│   └── lcd_buffer_draw.h        # 离屏缓冲区 & 图形绘制 API 声明
├── src/
│   ├── lcd_implement_template.c # ★ 硬件适配模板 — 移植时修改此文件
│   ├── lcd.c                    # 核心驱动（初始化、绘图、旋转）
│   ├── lcd_buffer.c             # 缓冲区管理（创建/销毁/提交/可移动）
│   ├── lcd_buffer_draw.c        # 缓冲区绘图（几何图形、字体、图像）
│   ├── font.c                   # 6×8 ASCII 字体 + 16×16 汉字示例
│   └── cat_pic.c                # 128×96 RGB565 猫咪示例图片
├── LICENSE                       # MIT 开源协议
├── README.md                     # 英文说明文档
└── README_CN.md                  # 中文说明文档（本文件）
```

## 常见问题

**Q: 支持哪些 MCU？**
A: 全部支持。核心驱动没有任何平台依赖，你只需为你的 MCU 实现硬件适配函数即可。

**Q: 支持 RTOS 吗？**
A: 支持。本库采用同步阻塞方式设计，可以在独立任务中运行，也可以用 RTOS 原语封装相关调用。

**Q: 如何添加对其他 LCD 控制器的支持？**
A: 替换 `lcd.c` 中的 `st7789_init_seq[]` 初始化表为你所用控制器的初始化序列即可。可选择性地添加新的 `xxx_cmd.h` 头文件存放寄存器定义。

**Q: 为什么 `lcd_implement_template.c` 里全是空函数？**
A: 它是有意设计成模板的——函数体为空，但带有详细的中文实现说明。这让移植需求一目了然。

**Q: 能在 Arduino 上使用吗？**
A: 可以。将 `digitalWrite`、`SPI.transfer`、`delay` 等 Arduino API 封装进适配函数即可。

## 开源协议

MIT — 详见 [LICENSE](LICENSE)。

## 致谢

字体数据参考自常见嵌入式字体库。ST7789 初始化序列来源于 ST7789VW 数据手册。
