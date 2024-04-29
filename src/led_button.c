#include "led_button.h"

void led_all_set(uint8_t state)
{
    if(state == 1)
    {
        LED_BLE_L;
        LED_RADIO_L;
        LED_WIFI_L;
        LED_NFC_L;
        LED_MARK_L;
    }
    else if(state == 0)
    {
        LED_BLE_H;
        LED_RADIO_H;
        LED_WIFI_H;
        LED_NFC_H;
        LED_MARK_H;   
    }

}
void pow_init()
{

    nrf_gpio_cfg_output(POW_WIFI);
    nrf_gpio_cfg_output(POW_VADS);
    k_msleep(10);      
    POW_WIFI_L;
    POW_VADS_H;    

}
void Led_Button_init()
{
    nrf_gpio_cfg_output(LED_BLE);
    nrf_gpio_cfg_output(LED_RADIO);
    nrf_gpio_cfg_output(LED_WIFI);   
    nrf_gpio_cfg_output(LED_NFC);
    nrf_gpio_cfg_output(LED_MARK); 

    led_all_set(0);  
    k_msleep(300);         
    led_all_set(1);  
    k_msleep(300);     
    led_all_set(0);     
    k_msleep(300);             
    led_all_set(1);  
    k_msleep(300);      
    led_all_set(0);  
}










