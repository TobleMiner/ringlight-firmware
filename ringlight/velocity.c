#include <stdbool.h>
#include <stdint.h>

#include "button.h"
#include "os.h"
#include "util.h"
#include "velocity.h"

#define UPDATE_INTERVAL_MS 100
#define VELOCITY_FRACTION 1000

typedef struct {
	unsigned button_inc;
	unsigned button_dec;
	int min;
	int max;
	int step;
	uint32_t acceleration; // 1/1000 units per second
	uint32_t default_speed; // 1/1000 units per second
	uint32_t max_speed; // speed limit
	uint32_t current_speed;
	bool inc_pressed;
	bool dec_pressed;
	int32_t value;
	os_task_t update_task;
} velocity_control_t;

#define VELOCITY_CONTROL(btn_up, btn_down, min, max, step, accel, default_speed, max_speed, default) \
	{ btn_up, btn_down, min, max, step, accel, default_speed, max_speed, default_speed, false, false, default * VELOCITY_FRACTION, OS_TASK_INITIALIZER }

static velocity_control_t controls_g[] = {
	VELOCITY_CONTROL(BUTTON_BRIGHTER, BUTTON_DIMMER, 0, 1000, 10, 1000, 20000, 100000, 10),
	VELOCITY_CONTROL(BUTTON_WARMER, BUTTON_COOLER, 0, 1000, 10, 1000, 20000, 100000, 500),
};

int velocity_get_value(unsigned velocity_id) {
	velocity_control_t *velocity = &controls_g[velocity_id];

	return velocity->value / VELOCITY_FRACTION;
}

void velocity_set_value(unsigned velocity_id, int value) {
	velocity_control_t *velocity = &controls_g[velocity_id];

	velocity->value = value * VELOCITY_FRACTION;
}

bool velocity_both_pressed(unsigned velocity_id) {
	velocity_control_t *velocity = &controls_g[velocity_id];

	return velocity->inc_pressed && velocity->dec_pressed;
}

static void velocity_task_cb(void *ctx) {
	velocity_control_t *velocity = ctx;

	if (velocity->inc_pressed || velocity->dec_pressed) {
		os_schedule_task_relative(&velocity->update_task, velocity_task_cb, MS_TO_US(1000 / UPDATE_INTERVAL_MS), velocity);
	}

	if (velocity->inc_pressed) {
		velocity->value += velocity->current_speed / (1000 / UPDATE_INTERVAL_MS);
	}

	if (velocity->dec_pressed) {
		velocity->value -= velocity->current_speed / (1000 / UPDATE_INTERVAL_MS);
	}

	velocity->value = MIN(velocity->value, velocity->max * VELOCITY_FRACTION);
	velocity->value = MAX(velocity->value, velocity->min * VELOCITY_FRACTION);

	velocity->current_speed += velocity->acceleration / (1000 / UPDATE_INTERVAL_MS);
	velocity->current_speed = MIN(velocity->current_speed, velocity->max_speed);
}

void velocity_update(void) {
	unsigned i;

	for (i = 0; i < ARRAY_SIZE(controls_g); i++) {
		velocity_control_t *velocity = &controls_g[i];
		bool inc_pressed = button_get_state(velocity->button_inc);
		bool dec_pressed = button_get_state(velocity->button_dec);

		if (inc_pressed) {
			if (!velocity->inc_pressed) {
				velocity->value += velocity->step * VELOCITY_FRACTION;
				velocity->value = MIN(velocity->max * VELOCITY_FRACTION, velocity->value);
				velocity->current_speed = velocity->default_speed;
				os_schedule_task_relative(&velocity->update_task, velocity_task_cb, MS_TO_US(1000 / UPDATE_INTERVAL_MS), velocity);
			}
		}
		velocity->inc_pressed = inc_pressed;

		if (dec_pressed) {
			if (!velocity->dec_pressed) {
				velocity->value -= velocity->step * VELOCITY_FRACTION;
				velocity->value = MAX(velocity->min * VELOCITY_FRACTION, velocity->value);
				velocity->current_speed = velocity->default_speed;
				os_schedule_task_relative(&velocity->update_task, velocity_task_cb, MS_TO_US(1000 / UPDATE_INTERVAL_MS), velocity);
			}
		}
		velocity->dec_pressed = dec_pressed;
	}
}
