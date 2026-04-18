#ifndef __OLED_H
#define __OLED_H 
#include "sys.h"
#include "stdlib.h"	

/***************SPI引脚配置（7针OLED）****************/
// OLED引脚定义 - 根据实际接线修改
#define OLED_SCK_PORT  		GPIOB
#define OLED_SCK_PIN		GPIO_Pin_13
#define OLED_SCK_GPIO_CLK   RCC_AHB1Periph_GPIOB

#define OLED_MOSI_PORT  	GPIOB
#define OLED_MOSI_PIN		GPIO_Pin_15
#define OLED_MOSI_GPIO_CLK  RCC_AHB1Periph_GPIOB

#define OLED_RES_PORT  		GPIOB
#define OLED_RES_PIN		GPIO_Pin_12
#define OLED_RES_GPIO_CLK   RCC_AHB1Periph_GPIOB

#define OLED_DC_PORT  		GPIOB
#define OLED_DC_PIN			GPIO_Pin_11
#define OLED_DC_GPIO_CLK    RCC_AHB1Periph_GPIOB

#define OLED_CS_PORT  		GPIOB
#define OLED_CS_PIN			GPIO_Pin_10
#define OLED_CS_GPIO_CLK    RCC_AHB1Periph_GPIOB

// OLED控制引脚操作
#define OLED_SCK_Clr() 	GPIO_ResetBits(OLED_SCK_PORT,OLED_SCK_PIN)
#define OLED_SCK_Set() 	GPIO_SetBits(OLED_SCK_PORT,OLED_SCK_PIN)

#define OLED_MOSI_Clr() GPIO_ResetBits(OLED_MOSI_PORT,OLED_MOSI_PIN)
#define OLED_MOSI_Set() GPIO_SetBits(OLED_MOSI_PORT,OLED_MOSI_PIN)

#define OLED_RES_Clr() 	GPIO_ResetBits(OLED_RES_PORT,OLED_RES_PIN)
#define OLED_RES_Set() 	GPIO_SetBits(OLED_RES_PORT,OLED_RES_PIN)

#define OLED_DC_Clr() 	GPIO_ResetBits(OLED_DC_PORT,OLED_DC_PIN)
#define OLED_DC_Set() 	GPIO_SetBits(OLED_DC_PORT,OLED_DC_PIN)

#define OLED_CS_Clr() 	GPIO_ResetBits(OLED_CS_PORT,OLED_CS_PIN)
#define OLED_CS_Set() 	GPIO_SetBits(OLED_CS_PORT,OLED_CS_PIN)

#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

void OLED_ClearPoint(u8 x,u8 y);
void OLED_ColorTurn(u8 i);
void OLED_DisplayTurn(u8 i);
void OLED_WR_Byte(u8 dat,u8 mode);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_DrawPoint(u8 x,u8 y,u8 t);
void OLED_DrawLine(u8 x1,u8 y1,u8 x2,u8 y2,u8 mode);
void OLED_DrawCircle(u8 x,u8 y,u8 r);
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size1,u8 mode);
void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 size1,u8 mode);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size1,u8 mode);
void OLED_ShowChinese(u8 x,u8 y,u8 num,u8 size1,u8 mode);
void OLED_ShowPicture(u8 x,u8 y,u8 sizex,u8 sizey,u8 BMP[],u8 mode);
void OLED_Init(void);

#endif
