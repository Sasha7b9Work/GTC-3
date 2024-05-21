#include "defines.h"
#include "Hardware/CDC/CDC.h"
#include "Hardware/HAL/HAL.h"
#include "Modules/HC12/HC12.h"
#include "Hardware/Beeper.h"
#include <stm32f3xx_hal.h>


void NMI_Handler(void)
{
}


void HardFault_Handler(void)
{
//    static int line = Debug::line[0];
//    static pchar file = Debug::file[0];
//    static int type = Debug::type;

    while (1) //-V776
    {
//        line = line; //-V570
//        file = file; //-V570
//        type = type; //-V570
    }
}


void MemManage_Handler(void)
{
    while (1)
    {
    }
}


void BusFault_Handler(void)
{
    while (1)
    {
    }
}


void UsageFault_Handler(void)
{
    while (1)
    {
    }
}


void SVC_Handler(void)
{

}


void DebugMon_Handler(void)
{

}


void PendSV_Handler(void)
{

}


void SysTick_Handler(void)
{
    HAL_IncTick();
}


void USB_LP_CAN_RX0_IRQHandler(void)
{
    HAL_PCD_IRQHandler((PCD_HandleTypeDef *)handlePCD);
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler((UART_HandleTypeDef *)HAL_USART_HC12::handle);
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *handle)
{
    if (handle == HAL_USART_HC12::handle)
    {
        HC12::_ReceiveCallback();
    }
}


void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler((ADC_HandleTypeDef *)HAL_ADC::handle);
}


void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler((TIM_HandleTypeDef *)Beeper::handleTIM3);
}


void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8 | GPIO_PIN_9);
}
