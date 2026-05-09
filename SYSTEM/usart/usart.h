#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

////////////////////////////////////////////////////////////////////////////////// 
//STM32F4工程-库函数版本
//移植TGAM脑电波数据采集功能 + EEG波形显示
////////////////////////////////////////////////////////////////////////////////// 	

#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define EN_USART1_RX 			1		//使能（1）/禁止（0）串口1接收

extern u8  USART_RX_BUF[USART_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART_RX_STA;         		//接收状态标记	

// TGAM脑电波数据相关声明
extern uint8_t brain_data1[36];           // 脑电波大包数据缓冲区(36字节)
extern volatile uint8_t tgam_data_ready;  // 数据帧接收完成标志

// EEG波形显示相关声明 (原始EEG数据在brain_data1[7~30]，共24字节/帧)
#define WAVE_BUF_WIDTH   128              // 波形缓冲区宽度(对应OLED屏幕宽度)
#define WAVE_SAMPLES_PER_FRAME 24         // 每帧包含的EEG采样点数
extern int8_t eeg_wave_buf[WAVE_BUF_WIDTH];  // 波形数据缓冲区(有符号,中心值为0)
extern volatile uint16_t wave_write_idx;     // 波形写入索引(循环缓冲)

void uart_init(u32 bound);
#endif
