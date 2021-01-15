#pragma once

#include <stddef.h>
#include <stdint.h>

#include "os_time.h"

#define US_TO_TICKS(us) (us)
#define MS_TO_TICKS(ms) US_TO_TICKS((ms) * 1000)
#define TICKS_TO_US(ticks) (ticks)

typedef void (*os_task_f)(void *ctx);

struct os_task;

struct os_task {
	struct os_task *next;
	void *ctx;
	os_time_t deadline;
	os_task_f run;
};

#define OS_TASK_INITIALIZER { NULL, NULL, OS_TIME_INITIALIZER, NULL }

typedef struct os_task os_task_t;

void os_init(void);
void os_schedule_task_relative(os_task_t *task, os_task_f cb, uint32_t us, void *ctx);
void os_run(void);
void os_delay(uint32_t us);
