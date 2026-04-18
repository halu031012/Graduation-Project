#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "oled.h"
 
//OLED显示实验-库函数版本 
//STM32F4工程-库函数版本


int main(void)
{
    delay_init(168);
    LED_Init();
    OLED_Init();
    OLED_Clear();
    delay_ms(100);  // 延时确保清屏完成
    
    // 从第2列开始显示（偏移2像素）
    OLED_ShowString(2, 0, "Hello World!", 16, 1);
    OLED_ShowString(2, 20, "STM32F407ZGT6", 16, 1);
    OLED_ShowString(2, 40, "OLED Test OK!", 16, 1);
    OLED_Refresh();  // 所有显示完成后统一刷新
    
    while(1)
    {
        LED1 = !LED1;
        delay_ms(500);
    }
}
