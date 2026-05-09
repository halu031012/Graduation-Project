# STM32F103 OLED程序移植到STM32F407说明

## 一、移植概述

将基于 **STM32F103C8T6** 的 OLED(SPI接口) 显示程序移植到 **STM32F407ZGT6** 工程模板。

> **重要提示**：本驱动同时支持 **SSD1306** 和 **SH1106** 两种驱动芯片的OLED模块。

## 二、硬件连接

### OLED模块引脚定义（7针SPI接口）

| 引脚 | 功能 | STM32F407接线 | 说明 |
|:----:|:----:|:-------------:|:----:|
| GND | 地 | GND | 地线 |
| VCC | 电源 | 3.3V | OLED供电（确认模块支持电压）|
| D0 | SCK | PB13 | SPI时钟 |
| D1 | MOSI | PB15 | SPI数据 |
| RES | 复位 | PB12 | 硬件复位 |
| DC | 数据/命令 | PB11 | 高=数据，低=命令 |
| CS | 片选 | PB10 | 低电平有效 |

### 引脚配置（可在oled.h中修改）

```c
#define OLED_SCK_PORT       GPIOB
#define OLED_SCK_PIN        GPIO_Pin_13

#define OLED_MOSI_PORT      GPIOB
#define OLED_MOSI_PIN       GPIO_Pin_15

#define OLED_RES_PORT       GPIOB
#define OLED_RES_PIN        GPIO_Pin_12

#define OLED_DC_PORT        GPIOB
#define OLED_DC_PIN         GPIO_Pin_11

#define OLED_CS_PORT        GPIOB
#define OLED_CS_PIN         GPIO_Pin_10
```

## 三、主要修改内容

### 1. 时钟使能函数差异

**F103版本：**
```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
```

**F407版本：**
```c
RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
```

> **注意**：F407的GPIO时钟在AHB1总线上，不是APB2。

### 2. GPIO模式设置差异

**F103版本：**
```c
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;  // 开漏输出
```

**F407版本：**
```c
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;     // 输出模式
GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;    // 推挽输出
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; // 降低速度提高稳定性
```

### 3. 标准SSD1306初始化序列

```c
void OLED_Init(void)
{
    // ... GPIO初始化 ...
    
    OLED_WR_Byte(0xAE,OLED_CMD);//关闭显示
    OLED_WR_Byte(0x00,OLED_CMD);//设置低列地址
    OLED_WR_Byte(0x10,OLED_CMD);//设置高列地址
    OLED_WR_Byte(0x40,OLED_CMD);//设置起始行地址
    OLED_WR_Byte(0x81,OLED_CMD);//设置对比度
    OLED_WR_Byte(0xCF,OLED_CMD);//对比度值
    OLED_WR_Byte(0xA1,OLED_CMD);//段重映射 (0xA0左右反置 0xA1正常)
    OLED_WR_Byte(0xC8,OLED_CMD);//COM扫描方向 (0xC0上下反置 0xC8正常)
    OLED_WR_Byte(0xA6,OLED_CMD);//正常显示
    OLED_WR_Byte(0xA8,OLED_CMD);//设置多路复用率
    OLED_WR_Byte(0x3F,OLED_CMD);//1/64 duty
    OLED_WR_Byte(0xD3,OLED_CMD);//设置显示偏移
    OLED_WR_Byte(0x02,OLED_CMD);//偏移2像素（关键！）
    OLED_WR_Byte(0xD5,OLED_CMD);//设置时钟分频
    OLED_WR_Byte(0x80,OLED_CMD);
    OLED_WR_Byte(0xD9,OLED_CMD);//设置预充电周期
    OLED_WR_Byte(0xF1,OLED_CMD);
    OLED_WR_Byte(0xDA,OLED_CMD);//设置COM引脚配置
    OLED_WR_Byte(0x12,OLED_CMD);
    OLED_WR_Byte(0xDB,OLED_CMD);//设置VCOMH
    OLED_WR_Byte(0x40,OLED_CMD);
    OLED_WR_Byte(0x20,OLED_CMD);//设置寻址模式
    OLED_WR_Byte(0x02,OLED_CMD);//页寻址模式
    OLED_WR_Byte(0x8D,OLED_CMD);//设置电荷泵
    OLED_WR_Byte(0x14,OLED_CMD);//使能电荷泵
    OLED_WR_Byte(0xA4,OLED_CMD);//关闭全局显示
    OLED_WR_Byte(0xA6,OLED_CMD);//正常显示
    OLED_WR_Byte(0xAF,OLED_CMD);//打开显示
}
```

## 四、遇到的问题及解决方案

### 问题1：显示镜像/左右反置

**现象**：文字左右镜像显示

**解决**：调整段重映射命令
```c
OLED_WR_Byte(0xA1,OLED_CMD);  // 0xA0=反置 0xA1=正常
```

### 问题2：最左侧2列像素缺失

**现象**：第一列字符显示不全

**原因**：OLED模块硬件问题，最左侧2列像素无法正常驱动

**解决**：设置显示偏移
```c
OLED_WR_Byte(0xD3,OLED_CMD);  // 设置显示偏移命令
OLED_WR_Byte(0x02,OLED_CMD);  // 偏移2像素
```

使用时从第2列开始显示：
```c
OLED_ShowString(2, 0, "Hello World!", 16, 1);  // x从2开始
```

### 问题3：SPI通信不稳定

**现象**：显示花屏或内容错乱

**解决**：
1. 降低GPIO速度到25MHz
2. 增加复位延时
3. 确保SPI时钟极性正确（Mode 0）

## 五、API使用说明

### 初始化
```c
delay_init(168);    // 延时初始化
OLED_Init();        // OLED初始化
OLED_Clear();       // 清屏
```

### 显示字符串
```c
// OLED_ShowString(x, y, 字符串, 字号, 模式)
OLED_ShowString(2, 0, "Hello World!", 16, 1);  // x从2开始
OLED_Refresh();     // 刷新显示
```

### 显示数字
```c
OLED_ShowNum(2, 2, 12345, 5, 16, 1);
OLED_Refresh();
```

### 画点
```c
OLED_DrawPoint(10, 10, 1);  // 在(10,10)画点
OLED_Refresh();
```

### 清屏
```c
OLED_Clear();
```

## 六、文件清单

移植后的OLED驱动包含以下文件：

```
HARDWARE/OLED/
├── oled.h          # 头文件（引脚定义、函数声明）
├── oled.c          # 驱动实现（初始化、显示函数）
└── oledfont.h      # 字库文件（ASCII码、汉字字模）
```

## 七、Keil工程配置

1. 将 `oled.c` 添加到Keil工程的HARDWARE组
2. 添加头文件路径：`HARDWARE/OLED`
3. 确保包含 `sys.h`、`delay.h` 等系统文件

## 八、注意事项

1. **偏移量**：由于硬件问题，所有x坐标建议从2开始
2. **电压**：确认OLED模块工作电压（3.3V或5V）
3. **接线**：SPI线尽量短，避免干扰
4. **复位**：RES引脚复位时间要足够（>200ms）

## 九、示例代码

```c
#include "sys.h"
#include "delay.h"
#include "led.h"
#include "oled.h"

int main(void)
{
    delay_init(168);
    LED_Init();
    OLED_Init();
    OLED_Clear();
    
    // 从第2列开始显示（偏移2像素）
    // 注意：y参数是行号(0-7)，16号字体占2行(16像素)
    OLED_ShowString(2, 0, "Hello World!", 16, 1);    // 第0-1行
    OLED_ShowString(2, 2, "STM32F407ZGT6", 16, 1);   // 第2-3行  
    OLED_ShowString(2, 4, "OLED Test OK!", 16, 1);   // 第4-5行
    OLED_Refresh();  // 最后统一刷新
    
    while(1)
    {
        LED1 = !LED1;
        delay_ms(500);
    }
}
```

**注意**：
1. 如果仍然只显示最后一行，请尝试在 `OLED_Clear()` 之后添加 `delay_ms(100);` 延时
2. 或者尝试在每次 `OLED_ShowString` 后都调用 `OLED_Refresh()`：
```c
OLED_ShowString(2, 0, "Hello World!", 16, 1);
OLED_Refresh();
OLED_ShowString(2, 2, "STM32F407ZGT6", 16, 1);
OLED_Refresh();
OLED_ShowString(2, 4, "OLED Test OK!", 16, 1);
OLED_Refresh();
```
3. 也可以不使用 `OLED_Clear()`，直接覆盖显示：
```c
OLED_Init();
delay_ms(100);
// 直接显示，不清屏
OLED_ShowString(2, 0, "Hello World!", 16, 1);
OLED_ShowString(2, 2, "STM32F407ZGT6", 16, 1);
OLED_ShowString(2, 4, "OLED Test OK!", 16, 1);
OLED_Refresh();
```

---

---

## 十、驱动芯片兼容性说明

### SSD1306 vs SH1106 区别

| 特性 | SSD1306 | SH1106 |
|:----:|:-------:|:------:|
| 显存大小 | 128×64 | 132×64 |
| 显示起始列 | 第0列 | 第2列 |
| 常用屏幕尺寸 | 0.96寸 | 1.3寸 |
| 兼容性 | 行业标准 | 部分屏幕使用 |

### 常见问题：右侧白边

**现象**：屏幕右侧出现一条白色竖线

**原因**：SSD1306和SH1106的显存起始地址不同：
- SSD1306：显存128列，从第0列开始显示
- SH1106：显存132列，从第2列开始显示（第0-1列被忽略）

当代码使用SSD1306的列地址（0x00）初始化SH1106屏幕时，会把显存中的前两列（未清零区域）显示在屏幕右侧。

**解决方案**：

修改 `OLED_Refresh()` 函数中的列地址：

```c
// HARDWARE/OLED/oled.c
void OLED_Refresh(void)
{
    u8 i,n;
    for(i=0;i<8;i++)
    {
        OLED_WR_Byte(0xb0+i,OLED_CMD);
        OLED_WR_Byte(0x02,OLED_CMD);  // 低列地址：SSD1306用0x00，SH1106用0x02
        OLED_WR_Byte(0x10,OLED_CMD);  // 高列地址
        for(n=0;n<128;n++)
        {
            OLED_WR_Byte(OLED_GRAM[n][i],OLED_DATA);
        }
    }
}
```

**默认值**：当前代码默认设置为 **0x02**，适配SH1106芯片。如果使用SSD1306屏幕出现左侧缺失2列像素，请将 `0x02` 改回 `0x00`。

---

---

**移植完成日期**：2026年4月18日  
**目标平台**：STM32F407ZGT6  
**OLED规格**：128x64 SPI接口（支持SSD1306/SH1106驱动芯片）

---

# 十一、TGAM脑电波数据采集功能移植文档

## 11.1 移植概述

将基于 **STM32F103C8T6 + I2C OLED** 的 TGAM 脑电波数据采集工程，移植到本 **STM32F407ZGT6 + SPI OLED** 工程上。

### 源工程与目标工程对比

| 项目 | 源工程（TGAM） | 目标工程（本工程） |
|:-----|:--------------|:------------------|
| MCU | STM32F103C8T6 | STM32F407ZGT6 |
| OLED接口 | I2C (4线, PA5/PA7) | SPI (7线, PB10~PB15) |
| OLED驱动方式 | 直接写屏（无显存缓冲）| 显存缓冲 + 统一刷新 |
| 原功能 | TGAM数据采集 + I2C OLED显示 | 仅SPI OLED显示测试 |

### 移植目标

1. 将TGAM模块的36字节数据帧解析状态机从F103移植到F407
2. 使用本工程的SPI OLED驱动替代原工程的I2C OLED驱动
3. 新增EEG原始波形实时滚动显示功能（示波器效果）

### 功能说明

本系统实现 **单导联EEG脑电波信号采集与实时可视化**：

```
TGAM脑电芯片 → HC-05/JDY-18蓝牙(57600bps) → USART1(PA10) → 中断解析 → OLED显示
                                                                    ↓
                                                            USB转TTL(PA9) → printf调试输出
```

## 11.2 硬件连接

### JDY-18蓝牙模块（TGAM通信）

| 模块引脚 | STM32F407ZGT6引脚 | 功能说明 |
|:--------:|:-----------------:|:---------|
| GND | GND | 地线 |
| VCC | **3.3V** | 电源供电 |
| TX | **PA10** (USART1_RX) | TGAM数据发送 → MCU接收 |

> **注意**：JDY-18作为中间桥梁，另一端通过蓝牙与TGAM模块通信。JDY-18的TX对应STM32的RX。

### USB转TTL串口（调试输出）

| 模块引脚 | STM32F407ZGT6引脚 | 功能说明 |
|:--------:|:-----------------:|:---------|
| GND | GND | 地线 |
| RX | **PA9** (USART1_TX) | MCU printf输出 → PC串口助手 |

> **波特率设置**：57600, 8N1

### SPI OLED显示屏（0.96寸 SSD1306，7针接口）

| 引脚 | 功能 | STM32F407引脚 |
|:----:|:----:|:-------------:|
| VCC | 电源 | 3.3V 或 5V |
| GND | 地 | GND |
| D0/SCK | SPI时钟 | PB13 |
| D1/MOSI | SPI数据 | PB15 |
| RES | 硬复位 | PB12 |
| DC | 数据/命令选择 | PB11 |
| CS | 片选(低有效) | PB10 |

## 11.3 修改文件清单

本次移植共修改/新增 **4 个文件**：

| 文件 | 改动类型 | 主要内容 |
|:-----|:-------:|:---------|
| `SYSTEM/usart/usart.h` | 修改 | 新增TGAM数据结构声明、波形缓冲区宏定义和变量声明 |
| `SYSTEM/usart/usart.c` | 重写 | F407串口初始化适配 + TGAM 36字节帧解析状态机 + 波形数据处理 |
| `USER/main.c` | 重写 | 主程序整合：OLED参数显示 + EEG波形绘制 |
| `HARDWARE/OLED/oled.h` | 修改 | 新增显存缓冲区extern声明（供main.c局部清屏使用）|

## 11.4 详细修改内容

### 11.4.1 usart.h — 头文件声明

**新增内容**（`usart.h:18-26`）：

```c
// TGAM脑电波数据相关声明
extern uint8_t brain_data1[36];           // 脑电波大包数据缓冲区(36字节)
extern volatile uint8_t tgam_data_ready;  // 数据帧接收完成标志(volatile)

// EEG波形显示相关宏定义
#define WAVE_BUF_WIDTH       128   // 波形缓冲区宽度(=OLED屏幕宽度)
#define WAVE_SAMPLES_PER_FRAME 24  // 每帧包含的EEG原始采样点数

// EEG波形显示相关变量声明
extern int8_t eeg_wave_buf[WAVE_BUF_WIDTH];    // 波形数据缓冲区(有符号,中心值为0)
extern volatile uint16_t wave_write_idx;       // 波形写入索引
```

**设计要点**：
- `tgam_data_ready` 使用 `volatile` 修饰符，因为在中断中写入、在主循环中读取
- `WAVE_BUF_WIDTH = 128` 对应OLED屏幕水平像素数，每个元素映射为屏幕上一列
- `WAVE_SAMPLES_PER_FRAME = 24` 是TGAM每帧输出的原始EEG采样点数

### 11.4.2 usart.c — 核心移植（改动最大）

#### （A）新增全局变量（`:51-58`）

```c
// TGAM脑电波数据相关全局变量
uint8_t brain_data1[36] = {0};              // 36字节完整帧缓冲
volatile uint8_t tgam_data_ready = 0;       // 数据就绪标志

// EEG波形显示相关全局变量
int8_t eeg_wave_buf[WAVE_BUF_WIDTH] = {0};  // 波形循环缓冲区(int8_t)
volatile uint16_t wave_write_idx = 0;        // 写入索引
```

#### （B）F407串口初始化适配（`:60-103`）

与F103的主要差异：

```c
// === GPIO时钟总线差异 ===
// F103: RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   // APB2总线
// F407: RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);   // AHB1总线

// === 引脚复用映射（F407必须配置）===
GPIO_PinAFConfig(GPIOA, GPIO_PinSource9,  GPIO_AF_USART1);  // PA9 → USART1_TX
GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);  // PA10 → USART1_RX

// === GPIO模式差异 ===
// F103: GPIO_Mode_AF_PP (推挽复用)
// F407: GPIO_Mode_AF + GPIO_OType_PP + GPIO_PuPd_UP (分开设置)
```

**关键点**：F407需要显式调用 `GPIO_PinAFConfig()` 配置引脚复用功能，否则GPIO不会连接到USART1外设。固定波特率 **57600 bps**（TGAM标准通信速率）。

#### （C）TGAM 36字节帧格式定义（`:105-118`）

```
帧格式总览 (共36字节):
┌──────┬──────┬──────┬──────┬────────┬──────┬─────────────────────┬──────┬────────┬──────┬────────┬──────┐
│ [0]  │ [1]  │ [2]  │ [3]  │  [4]   │ [5]  │     [7] ~ [30]      │ [31] │  [32]  │ [33] │  [34]  │ [35] │
│ 0xAA │ 0xAA │ 0x20 │ 0x02 │ Signal │ 0x83 │   Raw EEG (24B)     │ 0x04 │ Att    │ 0x05 │ Med    │ Sum  │
│ 帧头  │ 帧头  │ 长度  │PLen  │ 信号质 │心率ID│   原始脑电数据      │ AttID │ 专注度  │ MedID │ 放松度  │校验和 │
└──────┴──────┴──────┴──────┴────────┴──────┴─────────────────────┴──────┴────────┴──────┴────────┴──────┘
```

| 字段 | 字节位置 | 长度 | 取值范围 | 含义 |
|:-----|:--------:|:----:|:--------:|:-----|
| 帧头 | [0]-[1] | 2 B | 固定 0xAA 0xAA | 同步标志 |
| 数据长度 | [2] | 1 B | 0x20 (32) | 后续有效数据长度 |
| Payload长度 | [3] | 1 B | 0x02 | Payload域长度 |
| Signal信号质量 | [4] | 1 B | 0~200 | 0=最佳, 200=最差 |
| 心率标识 | [5] | 1 B | 0x83 | 固定标识码 |
| 心率数据长度 | [6] | 1 B | 0x18 (24) | 原始EEG数据长度 |
| **Raw EEG原始数据** | **[7]-[30]** | **24 B** | **0~255** | **单导联原始脑电波形** |
| 专注度标识 | [31] | 1 B | 0x04 | Attention字段标记 |
| **Attention专注度** | **[32]** | **1 B** | **0~100** | **数值越大越专注** |
| 放松度标识 | [33] | 1 B | 0x05 | Meditation字段标记 |
| **Meditation放松度** | **[34]** | **1 B** | **0~100** | **数值越大越放松** |
| 校验和 | [35] | 1 B | — | 校验所有字节之和 |

#### （D）帧解析状态机（`:120-217`，USART1_IRQHandler中断服务函数）

采用 **switch-case 状态机** 逐字节校验帧格式：

```
状态流转图:
  counter=0 ──[收到0xAA]──→ counter=1 ──[收到0xAA]──→ counter=2
       ↑                         ↑                        ↓
       │                      [非0xAA]                 [收到0x20]
       │                    counter=0                    ↓
       │                                              counter=3
       │                                                 ↓
       │                                              [收到0x02]
       │                                                 ↓
       │                                              counter=4..30
       │                                              (逐字节接收)
       │                                                 ↓
       │                                    ┌────── counter=31 ──[0x04]──→ 32
       │                                    │          ↓                     │
       │                               [非0x04]   counter=32            counter=33
       │                                  ↓             │                     ↓
       │                              counter=0      counter=33         [0x05]──→ 34
       │                                                  ↓                     ↓
       │                                             [非0x05]           counter=35(完成)
       │                                               ↓                       │
       │                                           counter=0                  ↓
       │                                          (任何字节错误都复位到0)   memmove+填充+flag=1
       └──────────────────────────────────────────────────────────────────────────┘
```

**核心代码逻辑**（以case 35为例，`:186-206`）：

```c
case 35: // 校验和 - 一帧数据接收完成
{
    uint8_t i;
    counter = 0;

    // ===== 示波器滚动模式：左移+追加 =====
    // 步骤1: 波形缓冲区整体左移24位(为新数据腾出右端空间)
    memmove(&eeg_wave_buf[0],
            &eeg_wave_buf[WAVE_SAMPLES_PER_FRAME],   // 从第24个元素开始
            (WAVE_BUF_WIDTH - WAVE_SAMPLES_PER_FRAME) * sizeof(int8_t));  // 左移104个

    // 步骤2: 将brain_data1[7~30]的24字节原始EEG数据转为有符号值,填入缓冲区右端
    for(i = 0; i < WAVE_SAMPLES_PER_FRAME; i++)
    {
        // 原始数据范围 0~255(无符号), 减128后变为 -128~+127(有符号), 以128为零电平中心
        eeg_wave_buf[WAVE_BUF_WIDTH - WAVE_SAMPLES_PER_FRAME + i] =
            (int8_t)(brain_data1[7 + i] - 128);
    }

    tgam_data_ready = 1;  // 通知主循环处理显示
    break;
}
```

**数据转换说明**：
- TGAM原始EEG数据为 **unsigned 8-bit (0~255)**，零电平对应值 **128**
- 转换公式：`signed_value = raw_value - 128`
- 结果范围：**-128 ~ +127 (int8_t)**，正负分别表示高于/低于零电平

### 11.4.3 main.c — 主程序整合

#### 整体架构

```c
int main(void)
{
    // 初始化阶段:
    delay_init(168);
    LED_Init();
    uart_init(57600);        // 串口初始化(固定57600bps给TGAM)
    OLED_Init();
    OLED_Clear();
    delay_ms(100);

    // 主循环:
    while(1)
    {
        if(tgam_data_ready == 1)        // 检查中断标志
        {
            tgam_data_ready = 0;
            frame_count++;

            // 第一部分: 参数区域显示(y=0~23)
            OLED_ShowString(...) "EEG Monitor"
            OLED_ShowNum(...)  brain_data1[4]   // S: 信号质量
            OLED_ShowNum(...)  brain_data1[32]  // A: 专注度
            OLED_ShowNum(...)  brain_data1[34]  // M: 放松度

            // 第二部分: 波形区域绘制(y=24~63)
            Draw_EEG_Waveform();       // 清除旧波形 + 画中线 + 画新曲线

            // 第三步: 统一刷新OLED
            OLED_Refresh();

            // 调试输出到串口
            printf("sig:%d att:%d med:%d\r\n", ...);
        }

        LED1 = !LED1;
        delay_ms(200);
    }
}
```

#### OLED屏幕布局设计（128×64像素）

```
┌──────────────────────────────────────┐
│ EEG Monitor                          │  y=0~7    标题行 (8号字体)
│ S:026  A:069  M:080                 │  y=8~23   参数行 (12号字体)
├──────────────────────────────────────┤ ← y=24 波形区分界线
│ ~~~·~~~~~~~·~~~~~~~~·~~~~~~~·~~~    │  y=24~63  波形显示区 (40像素高)
│    ·        ·         ·       ·    │           中线 y=43 (虚线基准线)
│ ····~~~~~~~~~~~~~~~···············  │           振幅 ±18像素
└──────────────────────────────────────┘ ← y=63 底部
  0                                 127
  ↑旧数据(左侧)               ↑新数据(右侧,持续向左滚动)
```

#### Draw_EEG_Waveform() 函数详解（`:68-128`）

**第一步：清除波形区域显存（`:75-83`）**

```c
// OLED_GRAM是128×8的显存数组, 每"页"(page)对应8行像素
// 波形区y=24~63, 跨越 page 3(page 24~31), 4(32~39), 5(40~47), 6(48~55), 7(56~63)
for(page = 3; page <= 7; page++)
{
    for(x = 0; x < WAVE_COLS; x++)
    {
        OLED_GRAM[x][page] = 0;  // 直接操作显存清零该区域
    }
}
```

> **为什么要手动清显存？**
>
> `OLED_DrawLine()` 内部使用按位或(`|=`)操作写像素：
> ```c
> if(t){OLED_GRAM[x][i]|=n;}  // 只能置1, 不能清0
> ```
> 如果不先清空，新旧波形线条会叠加，最终变成实心矩形块。

**第二步：计算有效数据列数（`:85-94`）**

```c
if(frame_count < (WAVE_COLS / WAVE_SAMPLES_PER_FRAME))  // 128/24 ≈ 5.3
{
    valid_cols = frame_count * WAVE_SAMPLES_PER_FRAME;   // 未满屏: 逐步增长
}
else
{
    valid_cols = WAVE_COLS;  // 已满屏: 显示全部128列
}
```

启动后前5帧(~5秒)，波形从右侧逐渐增长；5秒后全屏显示并持续滚动。

**第三步：画中线基准线（`:98-102`）**

```c
for(x = 0; x < valid_cols; x += 4)
{
    OLED_DrawPoint(x, WAVE_CENTER_Y, 1);  // 每4列画一个点, 形成虚线效果
}
```

中线位置 `WAVE_CENTER_Y = 43`（=(24+63)/2），表示EEG零电平基准。

**第四步：绘制波形曲线（`:104-127`）**

```c
// Y坐标映射公式:
//   y_pixel = WAVE_CENTER_Y - (value * WAVE_HALF_H / 128)
//
// 其中:
//   value    = eeg_wave_buf[x]  (-128 ~ +127, 有符号EEG值)
//   WAVE_HALF_H = 18  (半振幅像素数)
//   映射结果: y_pixel ∈ [25, 61] (钳位到波形区域内)

val = eeg_wave_buf[0];
prev_y_pos = WAVE_CENTER_Y - (val * WAVE_HALF_H / 128);
// 边界钳位防止超出波形区域
if(prev_y_pos <= WAVE_Y_TOP) prev_y_pos = WAVE_Y_TOP + 1;
if(prev_y_pos >= WAVE_Y_BOTTOM) prev_y_pos = WAVE_Y_BOTTOM - 1;

for(x = 1; x < valid_cols; x++)
{
    val = eeg_wave_buf[x];
    y_pos = WAVE_CENTER_Y - (val * WAVE_HALF_H / 128);

    // 边界钳位
    if(y_pos <= WAVE_Y_TOP) y_pos = WAVE_Y_TOP + 1;
    if(y_pos >= WAVE_Y_BOTTOM) y_pos = WAVE_Y_BOTTOM - 1;

    // 用直线连接相邻两点, 形成连续曲线
    OLED_DrawLine(x - 1, prev_y_pos, x, y_pos, 1);

    prev_y_pos = y_pos;
}
```

### 11.4.4 oled.h — 显存声明

**新增一行**（`oled.h:48`）：

```c
extern u8 OLED_GRAM[128][8];	// 显存缓冲区（供外部访问，用于局部清屏）
```

使 `main.c` 的 `Draw_EEG_Waveform()` 能够直接访问显存进行波形区域清除。

## 11.5 架构设计要点

### 中断与主循环分离

```
┌─────────────────────── 中断上下文 (ISR) ───────────────────────┐
│                                                                │
│   USART1_IRQHandler()                                         │
│     ├─ 接收1字节                                               │
│     ├─ switch-case状态机解析帧格式                             │
│     │   └─ 错误自动复位counter=0                              │
│     ├─ case 35(帧完成时):                                     │
│     │   ├─ memmove() 左移波形缓冲区                            │
│     │   ├─ 提取24字节EEG原始数据, 转换为有符号值                │
│     │   │   并填入缓冲区右端                                   │
│     │   └─ tgam_data_ready = 1  ← 设置标志位(仅此操作)        │
│     └─ 返回                                                    │
│                                                                │
│   ⚠️ 不做任何OLED操作, 最小化中断执行时间                      │
└────────────────────────────────────────────────────────────────┘
                           │
                   tgam_data_ready == 1 ?
                           │
┌─────────────────────── 主循环上下文 (main) ─────────────────────┐
│                                                                │
│   while(1):                                                    │
│     if(tgam_data_ready):                                       │
│       ├─ tgam_data_ready = 0                                   │
│       ├─ 写入标题/参数文字到显存(OLED_GRAM)                    │
│       ├─ Draw_EEG_Waveform():                                  │
│       │   ├─ 清除波形区显存(page 3~7)                          │
│       │   ├─ 画中线虚线                                        │
│       │   └─ 画波形曲线(DrawLine连线)                          │
│       ├─ OLED_Refresh() → SPI推送显存到屏幕                    │
│       └─ printf() 调试输出                                     │
│                                                                │
│     delay_ms(200)                                              │
└────────────────────────────────────────────────────────────────┘
```

**相比原TGAM工程的改进**：

| 对比项 | 原TGAM工程(F103) | 移植后(F407) |
|:------|:-----------------|:------------|
| OLED刷新位置 | 在中断ISR中直接调用 `OLED_Clear()` + `ShowString()` | 中断仅设标志位，主循环统一处理 |
| 刷新执行者 | `USART1_IRQHandler` | `main()` 主循环 |
| 中断阻塞时间 | 长（含OLED操作）| 极短（仅memmove+赋值）|
| 系统实时性 | 较差 | 优秀 |

### 示波器滚动模式原理

```
时间轴:  T=0s     T=1s     T=2s     T=3s     T=4s     T=5s     T=6s
        │        │        │        │        │        │        │
帧数:    第1帧    第2帧    第3帧    第4帧    第5帧    第6帧    ...
        │        │        │        │        │        │        │
缓冲区: [24新]   [24旧|24新] [48旧|24新] [72旧|24新] [96旧|24新] [104旧|24新] [满屏滚动...]
         ↑        ↑ ↑        ↑ ↑        ↑ ↑        ↑ ↑         ↑
       x=0~23  x=0~23 x=24~47 x=0~23 x=48~71 ...     x=104~127  (新数据始终在最右侧)

屏幕显示:  [=====>          ]  →  [========>      ]  →  [=============> ]  →  [===============>] (满屏)
           逐渐增长(前5秒)         → 之后持续向左滚动, 新数据从右侧进入
```

**关键参数**：
- TGAM输出帧率：**1 Hz**（约1帧/秒）
- 每帧EEG采样点：**24 个**
- 屏幕宽度：**128 列**
- 每帧左移：**24 列**
- 滚动速度：**24列/秒 ≈ 18.75% 屏宽/秒**
- 填满全屏：**≈ 5.3 秒**（128÷24）
- 等效采样率：**~24 Hz**

## 11.6 数据参数说明

### Signal（信号质量）— `brain_data1[4]`

| 范围 | 评价 |
|:----:|:-----|
| **0 ~ 50** | 信号质量良好 ✓ |
| 51 ~ 100 | 一般 |
| 101 ~ 150 | 较差 |
| 151 ~ 200 | 很差/几乎无信号 |

Signal反映硬件连接质量（电极接触阻抗、环境干扰等），通常比较稳定，不随脑活动剧烈变化。

### Attention（专注度）— `brain_data1[32]`

- 范围：**0 ~ 100**
- 数值越高表示当前注意力越集中
- 典型场景：做题、思考问题时升高至60~85；发呆时降低至20~40
- 与Meditation通常互斥

### Meditation（放松度）— `brain_data1[34]`

- 范围：**0 ~ 100**
- 数值越高表示当前心理越放松平静
- 典型场景：闭眼冥想时升高至60~90；紧张工作时降低至10~30
- 与Attention通常互斥

### 典型状态参考表

| 状态场景 | Att 范围 | Med 范围 |
|:---------|:--------:|:--------:|
| 闭眼静坐/深度冥想 | 10 ~ 30 | 60 ~ 90 |
| 听音乐/放松休息 | 20 ~ 40 | 50 ~ 70 |
| 普通日常状态 | 40 ~ 60 | 30 ~ 50 |
| 专注学习/工作 | 60 ~ 85 | 10 ~ 30 |
| 高度集中/精神紧绷 | 80 ~ 100 | 5 ~ 15 |

### 关于EEG导联类型

当前系统使用的是 **单导联（Single Channel）EEG** 采集：
- 电极位置：通常在前额 FP1 或 FP2 单点
- 采样率：TGAM内部512Hz ADC降采样后等效 ~24Hz 输出
- 用途：适合监测全局脑状态趋势（专注度/放松度）
- 局限性：无法进行脑区定位或多频段空间分析

## 11.7 移植过程遇到的问题及解决方案

### 问题1：memmove隐式声明警告

**现象**：
```
warning: #223-D: function "memmove" declared implicitly
```

**原因**：`<string.h>` 被包含在 `#if SYSTEM_SUPPORT_UCOS` 条件编译块内，当 `SYSTEM_SUPPORT_UCOS=0` 时不生效。

**解决方案**（`usart.c:3`）：
```c
#include <string.h>   // 移到条件编译块之外
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"
#include "string.h"   // 原来的保留
#endif
```

### 问题2：OLED_GRAM未定义错误

**现象**：
```
error: #20: identifier "OLED_GRAM" is undefined
```

**原因**：`OLED_GRAM[128][8]` 定义在 `oled.c` 内部，默认为文件作用域静态变量，`main.c` 无法访问。

**解决方案**（`oled.h:48`）：
```c
extern u8 OLED_GRAM[128][8];  // 在头文件中添加extern声明
```

### 问题3：波形叠加变矩形块

**现象**：OLED下半部分的EEG波形区域逐渐变成实心矩形，无法看到清晰的波形曲线。

**原因分析**：
- `OLED_DrawPoint()` 使用 **按位或(`|=`)** 操作写入像素：`if(t){OLED_GRAM[x][i]\|=n;}`
- 这种操作只能将像素**置为亮（1）**，永远不会**熄灭（0）**
- 每帧新波形叠加在旧波形上，经过多帧后整个区域全部被置为1

**解决方案**（`Draw_EEG_Waveform()` 开头）：
```c
// 在绘制新波形之前，先清除波形区域的显存
for(page = 3; page <= 7; page++)   // y=24~63 对应 page 3~7
{
    for(x = 0; x < WAVE_COLS; x++)
    {
        OLED_GRAM[x][page] = 0;    // 直接清零显存
    }
}
```

## 11.8 功能验证清单

### 阶段一：基础硬件验证

| # | 验证项 | 预期现象 | 方法 |
|:-:|:-------|:---------|:-----|
| 1 | 上电启动 | 电源灯亮，OLED闪一下后稳定 | 目测 |
| 2 | LED闪烁 | 板载LED约200ms翻转 | 目测 |
| 3 | OLED标题显示 | 屏幕顶部显示 "EEG Monitor" | 目测 |
| 4 | printf输出 | 串口助手打印 "TGAM EEG Monitor (Scope Mode)" | PC串口助手(57600bps) |

### 阶段二：通信验证

| # | 验证项 | 预期现象 | 方法 |
|:-:|:-------|:---------|:-----|
| 5 | 蓝牙配对 | JDY-18 LED由快闪变慢闪 | 手机/电脑搜索配对 |
| 6 | 数据流 | 串口助手持续输出 `sig:xx att:x med:x`（约1次/秒）| 观察串口助手 |

### 阶段三：OLED数据显示验证

| # | 验证项 | 预期现象 | 方法 |
|:-:|:-------|:---------|:-----|
| 7 | Signal显示 | `S:` 后显示0~200数值 | OLED第2行 |
| 8 | Att/Med显示 | `A:` 和 `M:` 后显示数值 | OLED第2行 |
| 9 | 数值合理性 | Att/Med ∈ [0,100], Signal ∈ [0,200] | 观察是否越界 |

### 阶段四：波形显示验证

| # | 验证项 | 预期现象 | 方法 |
|:-:|:-------|:---------|:-----|
| 10 | 波形初始增长 | 前5秒内波形从右向左逐渐增长延伸 | 观察OLED下半部 |
| 11 | 滚动效果 | 5秒后波形持续平滑向左滚动，新数据从右侧进入 | 观察 |
| 12 | 中线显示 | y=43处可见虚线基准线 | 观察 |
| 13 | 波形清晰度 | 波形曲线清晰可见，无叠加/花屏/矩形块 | 观察 |

### 阶段五：长期稳定性验证

| # | 验证项 | 预期现象 | 方法 |
|:-:|:-------|:---------|:-----|
| 14 | 串口/OLED同步 | OLED显示值与printf输出一致 | 对比 |
| 15 | 动态更新 | 数值随脑电状态变化而变化 | 佩戴电极观察 |
| 16 | 长时间运行 | 连续运行10分钟无死机/卡住/花屏 | 长时间观察 |

## 11.9 常见问题排查

| 现象 | 可能原因 | 解决方案 |
|:-----|:---------|:---------|
| OLED不亮 | 接线错误/VCC电压不对 | 检查PB10~PB15接线, VCC确认3.3V或5V |
| OLED只显示最后一行 | 初始化延时不足 | 在 `OLED_Clear()` 后增加 `delay_ms(100)` |
| 串口无数据输出 | 波特率不匹配/蓝牙未配对 | 确认串口助手设为 **57600, 8N1** |
| 串口有数据但S/A/M全为0 | 未佩戴电极/TGAM未预热 | 正确佩戴脑电波电极, 等待1~3分钟 |
| OLED波形变成矩形块 | 未清除波形区显存 | 已修复（见问题3解决方案）|
| 波形竖线密集不连续 | 采样点间距过大 | 已修复（改为1对1列映射+左移模式）|
| 编译报错 memmove | 缺少string.h | 见问题1解决方案 |
| 编译报错 OLED_GRAM | 缺少extern声明 | 见问题2解决方案 |

---

**TGAM移植完成日期**：2026年5月9日  
**目标平台**：STM32F407ZGT6  
**功能**：TGAM脑电波数据采集(Signal/Att/Med) + 单导联EEG波形实时示波器显示  
**通信方式**：JDY-18蓝牙模块 @ 57600bps → USART1(PA10)  
**显示设备**：0.96寸 SPI OLED (SSD1306/SH1106) @ PB10~PB15
