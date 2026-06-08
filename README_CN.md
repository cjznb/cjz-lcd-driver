# cjz-lcd-driver

一个硬件无关的嵌入式LCD驱动抽象层，同时支持SPI（4线）和FSMC（8080并行）两种通信接口。

## 概述

`cjz-lcd-driver` 为嵌入式MCU平台上的TFT LCD屏幕提供了一个干净的硬件抽象层（HAL）。它将高层的LCD命令逻辑（初始化序列、绘图函数、字体渲染）与底层的硬件实现解耦，使同一套驱动核心代码可以运行在不同的MCU平台上。

### 支持的接口

| 接口 | 说明 |
|------|------|
| **SPI**（4线） | SCK + MOSI + DC（数据/命令选择）+ CS（片选） |
| **FSMC / 8080** | 并行接口，命令和数据映射到独立的地址空间 |

## 特性

- **平台无关**：核心驱动逻辑不依赖任何特定硬件
- **模板化设计**：`lcd_implement_template.c` 提供带中文注释的模板，引导用户完成硬件适配
- **SPI事务模型**：支持定义多步骤SPI序列，每个步骤可配置前置/后置回调
- **FSMC支持**：通过地址映射直接访问命令/数据寄存器
- **统一配置**：所有LCD参数（分辨率、方向、颜色反转等）集中在单个头文件中

## 项目结构

```
cjz-lcd-driver/
├── inc/
│   ├── lcd_interface.h          # 硬件抽象接口定义
│   └── lcd_user_config.h        # 用户配置文件（分辨率、接口类型等）
├── src/
│   └── lcd_implement_template.c # 硬件适配模板（用户实现 TODO 标记的函数）
├── LICENSE                       # MIT 开源协议
└── README.md                     # 英文说明文档
```

## 快速上手

### 第一步：配置LCD参数

编辑 `inc/lcd_user_config.h`：

```c
#define LCD_USE_SPI     1       // 选择 SPI 或 FSMC 接口
#define LCD_RAW_WIDTH   240     // 你的LCD物理宽度
#define LCD_RAW_HEIGHT  320     // 你的LCD物理高度
```

### 第二步：实现硬件适配函数

打开 `src/lcd_implement_template.c`，实现所有标记为 `TODO` 的函数。每个函数都附有详细的中文实现说明。

**SPI 模式下必须实现的函数：**
- `MyLCD_Init()` — 初始化GPIO、SPI外设、DMA
- `MyLCD_setRST()` — 控制复位引脚电平
- `MyLCD_setBL()` — 控制背光引脚电平
- `MyLCD_setDC()` — 控制数据/命令选择引脚电平
- `MyLCD_setCS()` — 控制片选引脚电平
- `MyLCD_delay_ms()` — 毫秒级阻塞延时
- `MyLCD_submit()` — 提交SPI事务（核心函数）

**FSMC 模式下必须实现的函数：**
- `MyLCD_Init()` — 初始化GPIO、FSMC外设
- `MyLCD_setRST()` — 控制复位引脚电平
- `MyLCD_setBL()` — 控制背光引脚电平
- `MyLCD_delay_ms()` — 毫秒级阻塞延时
- `MyLCD_WriteCMD()` — 通过FSMC发送命令
- `MyLCD_WriteData()` — 通过FSMC发送数据（可配合DMA）

### 第三步：集成到上层驱动

上层驱动通过全局指针 `lcd_hw_interface` 访问所有硬件操作。实现完硬件适配函数后，上层驱动代码无需任何修改即可工作。

## 硬件适配指南

模板文件采用清晰的注释格式：

```c
/**
 * @brief   初始化LCD硬件接口
 * @note    【用户需实现】完成以下初始化工作：
 *          1. 配置与LCD相连的GPIO引脚（复位、背光、片选、数据/命令等）
 *          2. 初始化通信外设（SPI或FSMC）
 *          3. 若使用DMA，在此处完成DMA通道配置
 *          4. 若使用RTOS，可在此处创建必要的信号量/队列
 * @return  成功返回0，失败返回非0错误码
 */
static int MyLCD_Init(void)
{
    // TODO: 【用户需实现】在此处添加硬件初始化代码
    return 0;
}
```

每个函数包含：
- **功能说明**：函数的作用是什么
- **实现指引**：详细的实现描述，含伪代码示例
- **TODO 标记**：在何处添加你的代码

## 开源协议

MIT — 详见 [LICENSE](LICENSE)。

## 贡献

欢迎提交 Issue 和 Pull Request。请确保你的贡献保持平台无关的设计理念。
