#pragma once

#include <stdint.h>

#define GPIO_DIMMER	0
#define GPIO_BRIGHTER	1
#define GPIO_PWM_WARM	2
#define GPIO_PWM_COLD	3
#define GPIO_WARMER	4
#define GPIO_COLDER	5

typedef struct {
	uint32_t port;
	uint16_t gpio;
	uint8_t mode;
	uint8_t pullcfg;
	uint8_t otype;
	uint8_t ospeed;
	uint8_t af;
	uint8_t flags;
} gpio_t;

void gpiod_init(void);
void gpiod_set(uint8_t gpionum, uint8_t value);
uint16_t gpiod_get(uint8_t gpionum);
void gpiod_toggle(uint8_t gpionum);

uint32_t gpiod_get_port(uint8_t gpionum);
uint16_t gpiod_get_gpio(uint8_t gpionum);
