#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "oled.h"

/**********************************************************************
TGAM与单片机数据传输使用JDY-18蓝牙模块，使用STM32F4的串口1接收数据

		JDY-18蓝牙模块	<-----> STM32F407ZGT6
				GND												GND
				VCC												3.3V
				TX												PA10(USART1_RX)

USB转TTL串口工具 <----->	STM32F407ZGT6
				GND												GND
				RX												PA9(USART1_TX)

OLED与单片机通信使用SPI接口（7针），屏幕规格为0.96寸SPI协议OLED（SSD1306驱动）。
	SPI协议OLED屏幕 <----->	STM32F407ZGT6
				VCC												3.3V/5V
				GND												GND
				D0/SCK											PB13
				D1/MOSI											PB15
				RES												PB12
				DC												PB11
				CS												PB10

OLED屏幕布局 (128x64像素) - 示波器滚动模式:
┌──────────────────────────────────────┐
│ EEG Monitor                          │  y=0~7    标题 (8号字)
│ S:026  A:069  M:080                 │  y=8~23   参数行 (12号字)
│ ~~~|~~~~~~~|~~~~~~~~|~~~~~~~|~~~    │  y=24~63  波形区 (40px高)
│    |        |         |       |      │           中线y=43,每列1个点
└──────────────────────────────────────┘
  ↑旧数据                              ↑新数据(每秒左移24列)

采样参数：
  TGAM帧率 = 1Hz, 每帧24个EEG原始采样点
  屏幕宽度128列, 每帧左移24列 ≈ 每秒刷新18.75%屏宽
  填满全屏约需 ~5.3秒 (128/24帧)
***********************************************************************/

// 外部变量声明
extern uint8_t brain_data1[36];
extern volatile uint8_t tgam_data_ready;
extern int8_t eeg_wave_buf[WAVE_BUF_WIDTH];     // [0]=最旧, [127]=最新
extern volatile uint16_t wave_write_idx;

// ===== 波形显示区域参数 =====
#define WAVE_Y_TOP          24      // 波形区顶部行号
#define WAVE_Y_BOTTOM       63      // 波形区底部行号  
#define WAVE_CENTER_Y       43      // 中线位置 (24+63)/2 取整
#define WAVE_HALF_H         18      // 半幅高度 (63-24)/2 - 2留边
#define WAVE_COLS           128     // 屏幕总列数

// 已接收帧数计数（用于判断是否已填满波形缓冲区）
static volatile uint16_t frame_count = 0;


/*
 * 绘制EEG波形 - 示波器滚动模式
 * 
 * eeg_wave_buf[128] 每个元素对应屏幕一列(x=0~127)
 * 数据在中断中已按左移方式更新: 新数据在右端,旧数据在左端
 */
void Draw_EEG_Waveform(void)
{
	uint8_t x, page;
	int8_t val;
	uint8_t y_pos, prev_y_pos;
	uint8_t valid_cols;   // 有效数据列数
	
	// ===== 第零步：清除波形区域显存 (y=24~63 → page 3~7) =====
	// OLED_GRAM[列][页], 每页对应8行像素, 波形区占page 3,4,5,6,7
	for(page = 3; page <= 7; page++)
	{
		for(x = 0; x < WAVE_COLS; x++)
		{
			OLED_GRAM[x][page] = 0;  // 清除该列该页的显存
		}
	}
	
	// 计算有效数据量：未满屏时只绘制已有部分
	if(frame_count < (WAVE_COLS / WAVE_SAMPLES_PER_FRAME))
	{
		// 未填满：有效列数 = 已接收帧数 × 24
		valid_cols = frame_count * WAVE_SAMPLES_PER_FRAME;
	}
	else
	{
		valid_cols = WAVE_COLS;  // 已满屏，绘制全部128列
	}
	
	if(valid_cols < 2) return;
	
	// ---- 第一步：画中线基准线 (虚线效果: 每4个点画1个) ----
	for(x = 0; x < valid_cols; x += 4)
	{
		OLED_DrawPoint(x, WAVE_CENTER_Y, 1);
	}
	
	// ---- 第二步：绘制波形曲线 ----
	// 第一个点的y坐标
	val = eeg_wave_buf[0];
	prev_y_pos = WAVE_CENTER_Y - (val * WAVE_HALF_H / 128);
	if(prev_y_pos <= WAVE_Y_TOP) prev_y_pos = WAVE_Y_TOP + 1;
	if(prev_y_pos >= WAVE_Y_BOTTOM) prev_y_pos = WAVE_Y_BOTTOM - 1;
	
	// 从第1列开始，逐列连线到前一列
	for(x = 1; x < valid_cols; x++)
	{
		val = eeg_wave_buf[x];
		
		// int8_t (-127~+127) 映射到相对于中线的y偏移
		y_pos = WAVE_CENTER_Y - (val * WAVE_HALF_H / 128);
		
		// 边界钳位
		if(y_pos <= WAVE_Y_TOP) y_pos = WAVE_Y_TOP + 1;
		if(y_pos >= WAVE_Y_BOTTOM) y_pos = WAVE_Y_BOTTOM - 1;
		
		// 连线：从 (x-1, prev_y_pos) 到 (x, y_pos)
		OLED_DrawLine(x - 1, prev_y_pos, x, y_pos, 1);
		
		prev_y_pos = y_pos;
	}
}


int main(void)
{
    delay_init(168);           // 延时初始化(168MHz主频)
    LED_Init();                // LED初始化
    uart_init(57600);          // 串口1初始化 波特率57600(TGAM通信波特率)
    OLED_Init();               // OLED初始化
    OLED_Clear();              // 清屏
    delay_ms(100);             // 延时确保清屏完成
    
    printf("TGAM EEG Monitor (Scope Mode)\r\n");
    
    while(1)
    {
        if(tgam_data_ready == 1)
        {
            tgam_data_ready = 0;
            frame_count++;  // 帧计数+1
            
            // ===== 第一部分：标题 + 参数显示 =====
            
            OLED_ShowString(0, 0, "EEG Monitor", 8, 1);
            
            OLED_ShowString(0, 10, "S:", 12, 1);
            OLED_ShowNum(14, 10, brain_data1[4], 3, 12, 1);
            
            OLED_ShowString(46, 10, "A:", 12, 1);
            OLED_ShowNum(58, 10, brain_data1[32], 3, 12, 1);
            
            OLED_ShowString(92, 10, "M:", 12, 1);
            OLED_ShowNum(104, 10, brain_data1[34], 3, 12, 1);
            
            // ===== 第二部分：示波器式滚动波形 =====
            Draw_EEG_Waveform();
            
            // ===== 第三步：统一刷新屏幕 =====
            OLED_Refresh();
            
            // 调试输出
            printf("sig:%d att:%d med:%d\r\n", 
                   brain_data1[4], brain_data1[32], brain_data1[34]);
        }
        
        LED1 = !LED1;
        delay_ms(200);
    }
}
