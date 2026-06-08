/**
 * @file    lcd_interface.h
 * @brief   LCD驱动层硬件接口抽象
 * @note    本文件定义了LCD驱动与底层硬件之间的接口抽象，
 *          用户需在 lcd_implement_template.c 中实现所有函数指针。
 *          支持 SPI 和 FSMC（8080并行）两种通信接口。
 */

#ifndef LCD_INTERFACE_H_
#define LCD_INTERFACE_H_

#include <stdint.h>
#include <stdlib.h>
#include "lcd_user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * 一、SPI 事务模型（仅在 LCD_USE_SPI 模式下使用）
 * ======================================================================== */

typedef void (*LCD_Callback_t)(void *arg);

/** SPI 事务中的单个步骤 */
typedef struct {
    const uint8_t   *tx_buf;        /* 待发送数据缓冲区 */
    uint32_t         length;        /* 发送数据长度（字节） */

    /** 步骤前馈回调：在本次数据/命令发送前执行（通常用于切换DC引脚）*/
    LCD_Callback_t   feedforward;
    void            *ff_arg;        /* 前馈回调参数 */

    /** 步骤后置回调：本次数据/命令发送完成后立即执行 */
    LCD_Callback_t   callback;
    void            *cb_arg;        /* 后置回调参数 */
} LCD_SPI_Step_t;

/** SPI 事务：包含一组有序步骤 */
typedef struct {
    LCD_SPI_Step_t  *steps;         /* 步骤数组 */
    uint16_t         step_count;    /* 步骤数量 */

    /** 事务级完成回调：整个事务执行完后触发 */
    LCD_Callback_t   on_complete;
    void            *completion_arg; /* 完成回调参数 */
} LCD_Transaction_t;

/* ========================================================================
 * 二、LCD 硬件接口抽象
 * ======================================================================== */

/** LCD引脚状态枚举 */
typedef enum {
    LCD_PIN_RESET = 0,  /* 低电平 */
    LCD_PIN_SET   = 1   /* 高电平 */
} LCD_PinState_e;

/** LCD硬件接口抽象对象（前向声明） */
typedef struct LCD_HW_Interface_s LCD_HW_Interface_t;

struct LCD_HW_Interface_s {
    /* ----- 通用接口 ----- */

    /**
     * @brief 初始化LCD硬件接口
     * @return 成功返回0，失败返回非0错误码
     */
    int (*init)(void);

    /* ----- SPI 模式专属接口 ----- */
#if LCD_USE_SPI
    /**
     * @brief 提交SPI事务
     * @note  用户实现逻辑：遍历 transaction->steps 数组，依次通过SPI发送数据，
     *        并在适当时机调用 feedforward/callback/on_complete 回调。
     *        该函数必须阻塞直到所有操作完成。
     */
    void (*submit)(const LCD_HW_Interface_t *hw, const LCD_Transaction_t *transaction);

    /**
     * @brief 设置LCD片选引脚状态
     * @param state LCD_PIN_RESET(选中) / LCD_PIN_SET(取消选中)
     */
    void (*set_cs)(LCD_PinState_e state);

    /**
     * @brief 设置LCD数据/命令选择引脚状态
     * @param state LCD_PIN_RESET(命令) / LCD_PIN_SET(数据)
     */
    void (*set_dc)(LCD_PinState_e state);

    /* ----- FSMC 模式专属接口 ----- */
#elif LCD_USE_FSMC
    /**
     * @brief 通过FSMC接口发送命令
     * @note  阻塞执行直到命令发送完成
     */
    void (*sendCmd)(const LCD_HW_Interface_t *hw, uint8_t cmd);

    /**
     * @brief 通过FSMC接口发送数据
     * @param data 数据缓冲区（16位RGB565格式）
     * @param len  数据长度（像素点数）
     * @note  阻塞执行直到数据发送完成
     */
    void (*sendData)(const LCD_HW_Interface_t *hw, const uint8_t *data, size_t len);

    /** FSMC命令地址寄存器指针（用户需在初始化时赋值） */
    volatile uint16_t *cmd_reg;

    /** FSMC数据地址寄存器指针（用户需在初始化时赋值） */
    volatile uint16_t *data_reg;

#else
#error "请在 lcd_user_config.h 中定义 LCD_USE_SPI 或 LCD_USE_FSMC 以选择接口类型"
#endif

    /* ----- 通用引脚控制 ----- */

    /**
     * @brief 设置LCD复位引脚状态
     * @param state LCD_PIN_RESET(复位) / LCD_PIN_SET(正常运行)
     */
    void (*set_rst)(LCD_PinState_e state);

    /**
     * @brief 设置LCD背光引脚状态
     * @param state LCD_PIN_RESET(关闭) / LCD_PIN_SET(开启)
     */
    void (*set_bl)(LCD_PinState_e state);

    /**
     * @brief 毫秒级阻塞延时
     * @param ms 延时毫秒数
     */
    void (*delay_ms)(uint32_t ms);

    /* ----- 运行时可配置参数 ----- */

    void           *runtime_data;    /* LCD运行时数据（可由上层驱动存储任意上下文） */
    const uint16_t  raw_width;      /* LCD物理宽度（像素）*/
    const uint16_t  raw_height;     /* LCD物理高度（像素）*/
};

/**
 * @brief LCD硬件接口全局指针
 * @note  在 lcd_implement_template.c 中定义，指向静态分配的接口实例。
 *        上层驱动通过此指针调用所有硬件操作函数。
 */
extern const LCD_HW_Interface_t *lcd_hw_interface;

#ifdef __cplusplus
}
#endif

#endif /* LCD_INTERFACE_H_ */
