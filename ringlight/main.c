#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#include "button.h"
#include "gpiod.h"
#include "isr.h"
#include "os.h"
#include "velocity.h"

const uint16_t gamma16[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
    4,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  7,  7,  7,  7,  8,
    8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9, 10, 10,
   10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12,
   12, 12, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 15, 15, 15,
   15, 15, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 18, 18, 18, 18,
   18, 19, 19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22,
   22, 22, 23, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 26, 26,
   26, 26, 27, 27, 27, 27, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30,
   31, 31, 31, 32, 32, 32, 32, 33, 33, 33, 34, 34, 34, 35, 35, 35,
   36, 36, 36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 40, 40, 40, 41,
   41, 42, 42, 42, 43, 43, 43, 44, 44, 44, 45, 45, 46, 46, 46, 47,
   47, 48, 48, 48, 49, 49, 50, 50, 50, 51, 51, 52, 52, 52, 53, 53,
   54, 54, 55, 55, 55, 56, 56, 57, 57, 58, 58, 59, 59, 59, 60, 60,
   61, 61, 62, 62, 63, 63, 64, 64, 65, 65, 66, 66, 67, 67, 68, 68,
   69, 69, 70, 70, 71, 71, 72, 72, 73, 73, 74, 74, 75, 75, 76, 76,
   77, 77, 78, 78, 79, 80, 80, 81, 81, 82, 82, 83, 84, 84, 85, 85,
   86, 86, 87, 88, 88, 89, 89, 90, 90, 91, 92, 92, 93, 94, 94, 95,
   95, 96, 97, 97, 98, 98, 99,100,100,101,102,102,103,104,104,105,
  106,106,107,108,108,109,110,110,111,112,112,113,114,114,115,116,
  116,117,118,119,119,120,121,121,122,123,124,124,125,126,127,127,
  128,129,130,130,131,132,133,133,134,135,136,136,137,138,139,140,
  140,141,142,143,144,144,145,146,147,148,148,149,150,151,152,153,
  153,154,155,156,157,158,159,159,160,161,162,163,164,165,165,166,
  167,168,169,170,171,172,173,174,174,175,176,177,178,179,180,181,
  182,183,184,185,186,187,188,188,189,190,191,192,193,194,195,196,
  197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,
  213,214,215,217,218,219,220,221,222,223,224,225,226,227,228,229,
  230,232,233,234,235,236,237,238,239,240,241,243,244,245,246,247,
  248,249,251,252,253,254,255,256,258,259,260,261,262,263,265,266,
  267,268,269,271,272,273,274,275,277,278,279,280,282,283,284,285,
  287,288,289,290,292,293,294,295,297,298,299,301,302,303,305,306,
  307,308,310,311,312,314,315,316,318,319,320,322,323,324,326,327,
  329,330,331,333,334,335,337,338,340,341,342,344,345,347,348,350,
  351,352,354,355,357,358,360,361,362,364,365,367,368,370,371,373,
  374,376,377,379,380,382,383,385,386,388,389,391,392,394,396,397,
  399,400,402,403,405,406,408,410,411,413,414,416,417,419,421,422,
  424,426,427,429,430,432,434,435,437,439,440,442,444,445,447,449,
  450,452,454,455,457,459,460,462,464,465,467,469,471,472,474,476,
  478,479,481,483,485,486,488,490,492,493,495,497,499,501,502,504,
  506,508,510,511,513,515,517,519,521,522,524,526,528,530,532,533,
  535,537,539,541,543,545,547,549,550,552,554,556,558,560,562,564,
  566,568,570,572,574,576,578,580,582,584,586,588,590,591,593,596,
  598,600,602,604,606,608,610,612,614,616,618,620,622,624,626,628,
  630,632,634,637,639,641,643,645,647,649,651,653,656,658,660,662,
  664,666,668,671,673,675,677,679,681,684,686,688,690,692,695,697,
  699,701,704,706,708,710,713,715,717,719,722,724,726,728,731,733,
  735,738,740,742,745,747,749,751,754,756,759,761,763,766,768,770,
  773,775,777,780,782,785,787,789,792,794,797,799,801,804,806,809,
  811,814,816,819,821,824,826,828,831,833,836,838,841,843,846,848,
  851,854,856,859,861,864,866,869,871,874,876,879,882,884,887,889,
  892,895,897,900,902,905,908,910,913,916,918,921,924,926,929,932,
  934,937,940,942,945,948,950,953,956,959,961,964,967,970,972,975,
  978,981,983,986,989,992,994,997,1000 };

static const uint8_t gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

static void clock_init(void) {
	rcc_clock_setup_in_hsi_out_48mhz();
}

#define TIMER_TOP	1000
#define TIMER_DEFAULT	0

static void pwm_init(void) {
	rcc_periph_clock_enable(RCC_TIM1);
	rcc_periph_reset_pulse(RST_TIM1);

	timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_prescaler(TIM1, 47); // prescaler runs at 48 MHz, timer at 1Mhz
	timer_disable_preload(TIM1);
	timer_continuous_mode(TIM1);
	timer_set_period(TIM1, TIMER_TOP);
	timer_enable_break_main_output(TIM1);
	timer_update_on_overflow(TIM1);
	timer_set_oc_mode(TIM1, TIM_OC2, TIM_OCM_PWM1);
	timer_set_oc_mode(TIM1, TIM_OC3, TIM_OCM_PWM1);
	timer_enable_oc_output(TIM1, TIM_OC2);
	timer_enable_oc_output(TIM1, TIM_OC3);
	timer_set_oc_value(TIM1, TIM_OC2, TIMER_DEFAULT);
	timer_set_oc_value(TIM1, TIM_OC3, TIMER_DEFAULT);
	timer_enable_counter(TIM1);
}

static void exti_init(void) {
	rcc_periph_clock_enable(RCC_SYSCFG_COMP);

	nvic_set_priority(NVIC_EXTI0_1_IRQ, ISR_PRIO_HIGH);
	nvic_enable_irq(NVIC_EXTI0_1_IRQ);
	nvic_set_priority(NVIC_EXTI2_3_IRQ, ISR_PRIO_HIGH);
	nvic_enable_irq(NVIC_EXTI2_3_IRQ);
	nvic_set_priority(NVIC_EXTI4_15_IRQ, ISR_PRIO_HIGH);
	nvic_enable_irq(NVIC_EXTI4_15_IRQ);

	exti_set_trigger(EXTI0, EXTI_TRIGGER_FALLING);
	exti_set_trigger(EXTI1, EXTI_TRIGGER_FALLING);
	exti_set_trigger(EXTI5, EXTI_TRIGGER_FALLING);
	exti_set_trigger(EXTI6, EXTI_TRIGGER_FALLING);
	exti_enable_request(EXTI0);
	exti_enable_request(EXTI1);
	exti_enable_request(EXTI5);
	exti_enable_request(EXTI6);

	exti_select_source(EXTI0, GPIOF);
	exti_select_source(EXTI1, GPIOF);
	exti_select_source(EXTI5, GPIOA);
	exti_select_source(EXTI6, GPIOA);
}

static void exti_common(void) {
	uint32_t pending = EXTI_PR;
//	while (1) { };
	EXTI_PR = pending;
}

void exti0_1_isr(void) {
	exti_common();
}

void exti2_3_isr(void) {
	exti_common();
}

void exti4_15_isr(void) {
	exti_common();
}

int main(void) {
	int off_x = 0, off_y = 0, cycles = 0;
	int led_count = 0;

	clock_init();
	gpiod_init();
	pwm_init();
	exti_init();
	os_init();

	while (1) {
		int brightness;
		int temperature;
		int brightness_warm;
		int brightness_cold;

		os_run();
		button_update();
		velocity_update();

		if (velocity_both_pressed(VELOCITY_BRIGHTNESS)) {
			velocity_set_value(VELOCITY_BRIGHTNESS, 5);
		}

		if (velocity_both_pressed(VELOCITY_TEMPERATURE)) {
			velocity_set_value(VELOCITY_TEMPERATURE, 50);
		}

		brightness = velocity_get_value(VELOCITY_BRIGHTNESS);
		temperature = velocity_get_value(VELOCITY_TEMPERATURE);
		if (temperature >= 500) {
			brightness_warm = brightness;
			brightness_cold = brightness * (1000 - temperature) / 500;
		} else {
			brightness_warm = brightness * temperature / 500;
			brightness_cold = brightness;
		}

		timer_set_oc_value(TIM1, TIM_OC2, gamma16[brightness_warm * TIMER_TOP / 1000]);
		timer_set_oc_value(TIM1, TIM_OC3, gamma16[brightness_cold * TIMER_TOP / 1000]);
/*
		timer_set_oc_value(TIM1, TIM_OC2, brightness_warm * TIMER_TOP / 100);
		timer_set_oc_value(TIM1, TIM_OC3, brightness_cold * TIMER_TOP / 100);
*/
	}

	return 0;
}
