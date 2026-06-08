/**
 * @file    lcd_implement_template.c
 * @brief   LCD驱动硬件适配层实现模板
 * @note    本文件为模板文件，用户需根据实际硬件平台实现所有标记为 TODO 的函数。
 *          所有平台/芯片相关的代码均已移除，仅保留函数签名和中文实现说明。
 *          支持 SPI 和 FSMC（8080并行）两种接口模式，通过 lcd_user_config.h 中的宏选择。
 */

#include "lcd_interface.h"

/* =========================================================================
 * 一、基础外设控制函数（SPI / FSMC 通用，用户必须实现）
 * ========================================================================= */

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

/**
 * @brief   设置LCD复位引脚状态
 * @param   state   LCD_PIN_RESET(0)：拉低复位引脚
 *                  LCD_PIN_SET(1)：拉高复位引脚
 * @note    【用户需实现】调用平台的GPIO操作函数控制复位引脚电平。
 *          示例（伪代码）：
 *          if (state) GPIO_SetPin(RST_PORT, RST_PIN);
 *          else       GPIO_ClrPin(RST_PORT, RST_PIN);
 */
static void MyLCD_setRST(LCD_PinState_e state)
{
    // TODO: 【用户需实现】根据state设置复位引脚电平
    (void)state;
}

/**
 * @brief   设置LCD背光引脚状态
 * @param   state   LCD_PIN_RESET(0)：关闭背光
 *                  LCD_PIN_SET(1)：开启背光
 * @note    【用户需实现】控制背光引脚电平。若使用PWM调光，此处可仅做开关控制，
 *          调光功能可另设函数或在初始化中配置。
 */
static void MyLCD_setBL(LCD_PinState_e state)
{
    // TODO: 【用户需实现】根据state设置背光引脚电平
    (void)state;
}

/**
 * @brief   毫秒级阻塞延时
 * @param   ms   延时毫秒数
 * @note    【用户需实现】调用平台提供的毫秒级延时函数。阻塞等待直到指定毫秒数过去。
 *          裸机环境通常使用定时器轮询或指令循环；RTOS环境可使用 vTaskDelay() 等。
 */
static void MyLCD_delay_ms(uint32_t ms)
{
    // TODO: 【用户需实现】调用平台的毫秒延时函数
    (void)ms;
}

/* =========================================================================
 * 二、SPI 接口专属函数（仅在 LCD_USE_SPI 定义时编译）
 * ========================================================================= */

#if LCD_USE_SPI

/**
 * @brief   设置LCD数据/命令选择引脚状态
 * @param   state   LCD_PIN_RESET(0)：命令模式（DC引脚拉低）
 *                  LCD_PIN_SET(1)：数据模式（DC引脚拉高）
 * @note    【用户需实现】控制DC引脚电平，用于区分SPI发送的是命令还是数据。
 */
static void MyLCD_setDC(LCD_PinState_e state)
{
    // TODO: 【用户需实现】根据state设置DC引脚电平
    (void)state;
}

/**
 * @brief   设置LCD片选引脚状态
 * @param   state   LCD_PIN_RESET(0)：选中LCD（CS引脚拉低）
 *                  LCD_PIN_SET(1)：取消选中（CS引脚拉高）
 * @note    【用户需实现】控制CS引脚电平。通常在事务提交函数中由上层决定片选时机，
 *          此函数用于独立的片选控制场景。
 */
static void MyLCD_setCS(LCD_PinState_e state)
{
    // TODO: 【用户需实现】根据state设置CS引脚电平
    (void)state;
}

/**
 * @brief   提交SPI事务到LCD
 * @param   hw            LCD硬件接口指针
 * @param   transaction   待提交的事务对象
 * @note    【用户需实现】此函数是SPI模式下最核心的函数，需实现以下逻辑：
 *          1. 遍历 transaction->steps 数组（共 step_count 个步骤）
 *          2. 对每个步骤：
 *             a. 若步骤定义了 feedforward 回调，调用它（通常用于切换DC引脚）
 *             b. 通过SPI发送步骤中的 tx_buf 数据（长度为 length）
 *                - 小数据量可使用轮询发送
 *                - 大数据量建议使用DMA以提高效率
 *             c. 若步骤定义了 callback 回调，在SPI发送完成后调用它
 *          3. 所有步骤完成后，若 transaction 定义了 on_complete 回调，调用它
 *          4. 该函数必须阻塞直到所有操作完成
 *
 *          实现提示：
 *          - 轮询发送示例：循环检查SPI发送完成标志后写入下一个字节
 *          - DMA发送示例：配置DMA源地址为tx_buf、目标为SPI数据寄存器、传输长度为length；
 *            启用DMA后等待传输完成中断或轮询DMA完成标志
 *          - 发送完成后务必清空SPI接收FIFO，避免残留数据影响后续通信
 */
static void MyLCD_submit(const LCD_HW_Interface_t *hw, const LCD_Transaction_t *transaction)
{
    // TODO: 【用户需实现】遍历 transaction->steps 并通过SPI发送数据
    (void)hw;
    (void)transaction;
}

/* SPI模式硬件接口实例 */
static LCD_HW_Interface_t _lcd_hw_interface = {
    .submit     = MyLCD_submit,      /* 用户实现：SPI事务提交函数 */
    .init       = MyLCD_Init,        /* 用户实现：硬件初始化函数 */
    .set_cs     = MyLCD_setCS,       /* 用户实现：片选引脚控制函数 */
    .set_dc     = MyLCD_setDC,       /* 用户实现：数据/命令引脚控制函数 */
    .set_rst    = MyLCD_setRST,      /* 用户实现：复位引脚控制函数 */
    .set_bl     = MyLCD_setBL,       /* 用户实现：背光引脚控制函数 */
    .delay_ms   = MyLCD_delay_ms,    /* 用户实现：毫秒延时函数 */

    .raw_width  = LCD_RAW_WIDTH,     /* 根据实际LCD屏幕设置物理宽度（在lcd_user_config.h中配置） */
    .raw_height = LCD_RAW_HEIGHT     /* 根据实际LCD屏幕设置物理高度（在lcd_user_config.h中配置） */
};

/* =========================================================================
 * 三、FSMC（8080并行）接口专属函数（仅在 LCD_USE_FSMC 定义时编译）
 * ========================================================================= */

#elif LCD_USE_FSMC

/**
 * @brief   通过FSMC/8080接口发送命令到LCD
 * @param   hw    LCD硬件接口指针
 * @param   cmd   命令字节
 * @note    【用户需实现】向FSMC的命令地址写入命令字节。
 *          示例（伪代码）：
 *          *(hw->cmd_reg) = cmd;
 *          cmd_reg 是映射到FSMC命令地址的 volatile 指针。
 */
void MyLCD_WriteCMD(const LCD_HW_Interface_t *hw, uint8_t cmd)
{
    // TODO: 【用户需实现】通过FSMC命令地址发送命令
    (void)hw;
    (void)cmd;
}

/**
 * @brief   通过FSMC/8080接口发送数据到LCD
 * @param   hw    LCD硬件接口指针
 * @param   data  数据缓冲区指针（16位RGB565格式）
 * @param   len   数据长度（像素点数，非字节数）
 * @note    【用户需实现】向FSMC的数据地址写入像素数据。
 *          实现提示：
 *          - 小数据量可逐像素写入：while(len--) { *(hw->data_reg) = *data++; }
 *          - 大数据量建议使用DMA：配置DMA源地址为data、目标为FSMC数据地址；
 *            单次DMA传输有长度上限（通常65535），需分块传输
 *          - 该函数必须阻塞直到所有数据传输完成
 */
void MyLCD_WriteData(const LCD_HW_Interface_t *hw, const uint8_t *data, size_t len)
{
    // TODO: 【用户需实现】通过FSMC数据地址发送像素数据
    (void)hw;
    (void)data;
    (void)len;
}

/* FSMC模式硬件接口实例 */
static LCD_HW_Interface_t _lcd_hw_interface = {
    .sendCmd    = MyLCD_WriteCMD,     /* 用户实现：FSMC命令发送函数 */
    .sendData   = MyLCD_WriteData,    /* 用户实现：FSMC数据发送函数 */
    .init       = MyLCD_Init,         /* 用户实现：硬件初始化函数 */
    .set_rst    = MyLCD_setRST,       /* 用户实现：复位引脚控制函数 */
    .set_bl     = MyLCD_setBL,        /* 用户实现：背光引脚控制函数 */
    .delay_ms   = MyLCD_delay_ms,     /* 用户实现：毫秒延时函数 */

    /* 用户需在初始化时将以下指针指向实际的FSMC地址映射 */
    .cmd_reg    = NULL,   /* TODO: 【用户需配置】指向FSMC命令地址，如 (volatile uint16_t *)0x6C000000 */
    .data_reg   = NULL,   /* TODO: 【用户需配置】指向FSMC数据地址，如 (volatile uint16_t *)0x6C000080 */

    .raw_width  = LCD_RAW_WIDTH,      /* 根据实际LCD屏幕设置物理宽度（在lcd_user_config.h中配置） */
    .raw_height = LCD_RAW_HEIGHT      /* 根据实际LCD屏幕设置物理高度（在lcd_user_config.h中配置） */
};

#else
#error "请在 lcd_user_config.h 中定义 LCD_USE_SPI 或 LCD_USE_FSMC 以选择接口类型"
#endif

/* =========================================================================
 * 导出全局硬件接口指针
 * ========================================================================= */

/**
 * @brief   LCD硬件接口全局指针
 * @note    上层驱动通过此指针访问所有硬件操作函数。
 *          在系统初始化阶段调用 MyLCD_Init() 后即可使用。
 */
const LCD_HW_Interface_t *lcd_hw_interface = &_lcd_hw_interface;
