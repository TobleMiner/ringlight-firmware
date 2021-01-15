#pragma once

#include <stdbool.h>

#define BUTTON_BRIGHTER 0
#define BUTTON_DIMMER 1
#define BUTTON_WARMER 2
#define BUTTON_COOLER 3

void button_update(void);
bool button_get_state(unsigned button_id);
