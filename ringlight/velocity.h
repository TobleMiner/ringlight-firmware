#pragma once

#define VELOCITY_BRIGHTNESS 0
#define VELOCITY_TEMPERATURE 1

int velocity_get_value(unsigned velocity_id);
void velocity_set_value(unsigned velocity_id, int value);
void velocity_update(void);
bool velocity_both_pressed(unsigned velocity_id);
