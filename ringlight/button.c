#include <stdint.h>
#include <stdbool.h>

#include "button.h"
#include "gpiod.h"
#include "os.h"
#include "util.h"

#define DEBOUNCE_MS 10

typedef struct {
	uint8_t gpio;
	bool state;
	bool last_state;
	bool debounce;
	os_task_t debounce_task;
} button_t;

#define BUTTON(gpio) { gpio, false, false, false, OS_TASK_INITIALIZER }

static button_t buttons_g[] = {
	BUTTON(GPIO_BRIGHTER),
	BUTTON(GPIO_DIMMER),
	BUTTON(GPIO_WARMER),
	BUTTON(GPIO_COLDER),
};

static void debounce_cb(void *ctx) {
	button_t *button = ctx;

	button->debounce = false;
}

void button_update(void) {
	unsigned i;

	for (i = 0; i < ARRAY_SIZE(buttons_g); i++) {
		button_t *button = &buttons_g[i];
		bool state = gpiod_get(button->gpio);

		if (state != button->last_state) {
			if (!button->debounce) {
				button->state = state;
			}
			button->debounce = true;
			os_schedule_task_relative(&button->debounce_task, debounce_cb, MS_TO_US(DEBOUNCE_MS), button);
		} else {
			if (!button->debounce) {
				button->state = state;
			}
		}
		button->last_state = state;
	}
}

bool button_get_state(unsigned button_id) {
	button_t *button = &buttons_g[button_id];

	return button->state;
}
