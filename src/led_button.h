#ifndef LED_BUTTON_H
#define LED_BUTTON_H

#include "main.h"


#define LED_BLE             NRF_GPIO_PIN_MAP(1,12)
#define LED_RADIO           NRF_GPIO_PIN_MAP(1,13)
#define LED_WIFI            NRF_GPIO_PIN_MAP(1,14)
#define LED_NFC             NRF_GPIO_PIN_MAP(1,15)
#define LED_MARK            NRF_GPIO_PIN_MAP(0,06)
/*******************/
#define POW_WIFI            NRF_GPIO_PIN_MAP(0,20)
#define POW_VADS            NRF_GPIO_PIN_MAP(0,19)


#define LED_BLE_H           nrf_gpio_pin_set(LED_BLE)
#define LED_BLE_L           nrf_gpio_pin_clear(LED_BLE)
#define LED_RADIO_H         nrf_gpio_pin_set(LED_RADIO)
#define LED_RADIO_L         nrf_gpio_pin_clear(LED_RADIO)
#define LED_WIFI_H          nrf_gpio_pin_set(LED_WIFI)
#define LED_WIFI_L          nrf_gpio_pin_clear(LED_WIFI)
#define LED_NFC_H           nrf_gpio_pin_set(LED_NFC)
#define LED_NFC_L           nrf_gpio_pin_clear(LED_NFC)
#define LED_MARK_H          nrf_gpio_pin_set(LED_MARK)
#define LED_MARK_L          nrf_gpio_pin_clear(LED_MARK)
/*******************/
#define POW_WIFI_H          nrf_gpio_pin_set(POW_WIFI)
#define POW_WIFI_L          nrf_gpio_pin_clear(POW_WIFI)
#define POW_VADS_H          nrf_gpio_pin_set(POW_VADS)
#define POW_VADS_L          nrf_gpio_pin_clear(POW_VADS)



void pow_init();
void led_all_set(uint8_t state);
void Led_Button_init();





#endif