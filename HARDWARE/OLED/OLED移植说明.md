# STM32F103 OLED程序移植到STM32F407说明

## 一、移植概述

将基于 **STM32F103C8T6** 的 OLED(SPI接口) 显示程序移植到 **STM32F407ZGT6** 工程模板。

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

**移植完成日期**：2026年4月18日  
**目标平台**：STM32F407ZGT6  
**OLED规格**：128x64 SPI接口 SSD1306驱动芯片
