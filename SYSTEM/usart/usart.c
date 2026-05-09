#include "sys.h"
#include "usart.h"	
#include <string.h>		// memmove函数声明（示波器滚动模式使用）
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos 使用	 
#include "string.h"
#endif
////////////////////////////////////////////////////////////////////////////////// 	 
// STM32F4工程-库函数版本
// 移植TGAM脑电波数据采集功能 - 通过JDY-18蓝牙模块接收数据
//
// TGAM与单片机数据传输使用JDY-18蓝牙模块，使用STM32F4的串口1接收数据
//		JDY-18蓝牙模块	<-----> STM32F407ZGT6
//				GND			GND
//				VCC			3.3V
//				TX			PA10(USART1_RX)
//		
//  USB转TTL串口工具 <----->	STM32F407ZGT6
//				RX			PA9(USART1_TX)
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
	USART1->DR = (u8) ch;      
	return ch;
}
#endif 

#if EN_USART1_RX   //如果使能了接收
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
u16 USART_RX_STA=0;       //接收状态标记	

// TGAM脑电波数据相关全局变量
uint8_t brain_data1[36] = {0};    // 脑电波大包数据缓冲区(36字节)
volatile uint8_t tgam_data_ready = 0;  // 数据帧接收完成标志

// EEG波形显示相关全局变量
// 原始EEG数据在brain_data1[7~30]，共24字节/帧，无符号值(0~255)
int8_t eeg_wave_buf[WAVE_BUF_WIDTH] = {0};   // 波形循环缓冲区(有符号，中心值=0)
volatile uint16_t wave_write_idx = 0;          // 当前写入位置

void uart_init(u32 bound){
   //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //使能GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//使能USART1时钟

	//串口1对应引脚复用映射
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1); //GPIOA9复用为USART1_TX
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1); //GPIOA10复用为USART1_RX
	
	//USART1端口配置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA9，PA10

  //USART1 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  USART_Init(USART1, &USART_InitStructure); //初始化串口1
	
  USART_Cmd(USART1, ENABLE);  //使能串口1 
	
	USART_ClearFlag(USART1, USART_FLAG_TC);

	//开启串口接收中断
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);
}


// TGAM脑电波数据包格式 (36字节):
// [0-1]: 0xAA 0xAA   帧头
// [2]  : 0x20        数据长度
// [3]  : 0x02        Payload长度
// [4]  :             信号质量值 (0-200)
// [5]  : 0x83        心率标识
// [6]  : 0x18        心率数据长度 (24字节原始EEG数据)
// [7-30]:            原始脑电波数据 (24字节)
// [31] : 0x04        专注度标识
// [32] :             专注度 Attention (0-100)
// [33] : 0x05        放松度标识
// [34] :             放松度 Meditation (0-100)
// [35] :             校验和

//串口1中断服务程序 - TGAM数据解析状态机
void USART1_IRQHandler(void)                	
{		
	static uint8_t counter = 0;
	uint8_t res;

#ifdef OS_TICKS_PER_SEC	 	
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		res = USART_ReceiveData(USART1); 
		brain_data1[counter] = res; //存入缓冲区
		
		switch(counter)
		{
			case 0:
				if (res == 0xAA) counter++;
				else counter = 0;
				break;
			case 1:
				if (res == 0xAA) counter++;
				else counter = 0;
				break;
			case 2:
				if (res == 0x20) counter++;
				else counter = 0;
				break; 
			case 3:
				if (res == 0x02) counter++;
				else counter = 0;
				break;
			case 4:  //信号质量
				counter++;
				break;
			case 5:  //心率标识 0x83
				if (res == 0x83) counter++;
				else counter = 0;
				break;               
			case 6:  //心率长度 0x18
				if (res == 0x18) counter++;
				else counter = 0;
				break;
			// case 7~30: 原始脑电波数据24字节，直接接收
			case 7: case 8: case 9: case 10:
			case 11: case 12: case 13: case 14:
			case 15: case 16: case 17: case 18:
			case 19: case 20: case 21: case 22:
			case 23: case 24: case 25: case 26:
			case 27: case 28: case 29: case 30:
				counter++;
				break;
			case 31: //专注度标识 0x04
				if (res == 0x04) counter++;
				else counter = 0;
				break;
			case 32: //专注度数值
				counter++;
				break;
			case 33: //放松度标识 0x05
				if (res == 0x05) counter++;
				else counter = 0;
				break;
			case 34: //放松度数值
				counter++;
				break;
			case 35: //校验和 - 一帧数据接收完成
			{
				uint8_t i;
				counter = 0;
				
				// ===== 示波器滚动模式：左移+追加 =====
				// 将波形缓冲区整体左移 WAVE_SAMPLES_PER_FRAME(24) 位
				// 新的24个EEG采样点写入右端 [104..127]
				memmove(&eeg_wave_buf[0], 
				        &eeg_wave_buf[WAVE_SAMPLES_PER_FRAME], 
				        (WAVE_BUF_WIDTH - WAVE_SAMPLES_PER_FRAME) * sizeof(int8_t));
				
				// 原始EEG数据(brain_data1[7~30])为无符号(0~255)，转为有符号(中心值128→0)
				for(i = 0; i < WAVE_SAMPLES_PER_FRAME; i++)
				{
					eeg_wave_buf[WAVE_BUF_WIDTH - WAVE_SAMPLES_PER_FRAME + i] = 
						(int8_t)(brain_data1[7 + i] - 128);
				}
				
				tgam_data_ready = 1;  // 标记一帧数据接收完成
				break;
			}
			default:
				counter = 0;
				break;
		}        
	}

#ifdef OS_TICKS_PER_SEC	 	
	OSIntExit();											 
#endif
} 
#endif
