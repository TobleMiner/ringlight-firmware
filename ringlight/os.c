#include "os.h"

#include <stddef.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#define OS_TIMER TIM3
#define OS_TIMER_RCC RCC_TIM3
#define OS_TIMER_RST RST_TIM3
#define OS_TIMER_IRQ NVIC_TIM3_IRQ
#define OS_TIMER_IRQ_HANDLER tim3_isr
#define OS_TIMER_TOP (0xFFFF)

static uint32_t last_timer_counter_sync = 0;
static os_time_t last_os_time = { 0, 0 };
static uint32_t next_task_timer_counter = 0;
static uint32_t last_run_timer_counter = 0;

static os_task_t *os_tasks;

void os_init() {
	// 1us resolution RTOS timer
	rcc_periph_clock_enable(OS_TIMER_RCC);
	rcc_periph_reset_pulse(OS_TIMER_RST);
	// fABP / (47 + 1) = 1MHz
	timer_set_mode(OS_TIMER, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_prescaler(OS_TIMER, 47);
	timer_set_period(OS_TIMER, OS_TIMER_TOP);
	timer_update_on_overflow(OS_TIMER);
	timer_set_counter(OS_TIMER, 0);
	timer_generate_event(OS_TIMER, TIM_EGR_UG);
	timer_enable_counter(OS_TIMER);
}

static void os_sync_time(void) {
	uint32_t ticks_now = timer_get_counter(OS_TIMER);
	uint32_t ticks_delta;

	/*
	 * TODO : This is slightly flawed.
	 * if overflow happens and is checked too late
	 * it might not be caught. Add an additional
	 * overflow flag set from isr.
	 * Then check for the flag both, when overflow
	 * is detected and even if not. The check within
	 * the detection needs to have interrupts disabled
	 * and must set an additional overflow_handled flag
	 * to prevent races with the isr.
	 * The check outside is less critical. It can just
	 * check the flag and unset it if it was set. In theory
	 * there could be a race against the next overflow.
	 * However we assume that overflows are far enough apart
	 * for this routine to handle the first before another
	 * one happens
	 */
	if (ticks_now < last_timer_counter_sync) {
		// Overflow happened!
		ticks_delta = OS_TIMER_TOP;
		ticks_delta -= last_timer_counter_sync;
		ticks_delta += ticks_now;
	} else {
		ticks_delta = ticks_now - last_timer_counter_sync;
	}

	time_add_us(&last_os_time, TICKS_TO_US(ticks_delta));
	last_timer_counter_sync = ticks_now;
}

static void os_recalculate_next_deadline(void) {
	uint32_t ticks_now = timer_get_counter(OS_TIMER);
	os_time_t earliest;
	uint32_t delta_us, delta_ticks;

	if (!os_tasks) {
		next_task_timer_counter = OS_TIMER_TOP;
		return;
	}

	os_sync_time();
	// earliest deadline is guaranteed  to be list head
	earliest = os_tasks->deadline;

	if (TIME_GE(last_os_time, earliest)) {
		next_task_timer_counter = ticks_now;
		return;
	}

	delta_us = time_delta_us(&earliest, &last_os_time);
	delta_ticks = US_TO_TICKS(delta_us);
	if (OS_TIMER_TOP <= delta_ticks ||
	    OS_TIMER_TOP - delta_ticks < ticks_now) {
		next_task_timer_counter = OS_TIMER_TOP;
		return;
	}

	next_task_timer_counter = ticks_now + delta_ticks;
}

static void os_remove_task(os_task_t *removee) {
	if (os_tasks == removee) {
		os_tasks = removee->next;
	} else {
		os_task_t *task = os_tasks;
		while (task) {
			if (task->next == removee) {
				task->next = removee->next;
				break;
			}
			task = task->next;
		}
	}
	removee->next = NULL;
}

static void os_add_task(os_task_t *insertee)  {
	insertee->next = NULL;
	// Adding a task twice would be fatal
	os_remove_task(insertee);
	if (os_tasks) {
		os_task_t *task = os_tasks;

		// Special case if earlier than current first task
		if (TIME_GE(os_tasks->deadline, insertee->deadline)) {
			insertee->next = os_tasks;
			os_tasks = insertee;
			return;
		}

		while (task->next) {
			if (TIME_GE(insertee->deadline, task->deadline) &&
			    TIME_GE(task->next->deadline, insertee->deadline)) {
				break;
			}
			task = task->next;
		}
		insertee->next = task->next;
		task->next = insertee;
	} else {
		os_tasks = insertee;
	}
}

void os_schedule_task_relative(os_task_t *task, os_task_f cb, uint32_t us, void *ctx) {
	os_time_t deadline;

	os_sync_time();
	deadline = last_os_time;
	time_add_us(&deadline, us);

	task->run = cb;
	task->deadline = deadline;
	task->ctx = ctx;

	os_add_task(task);
	os_recalculate_next_deadline();
}

void os_run() {
	uint32_t ticks_now = timer_get_counter(OS_TIMER);
	os_task_t *task = os_tasks;
	os_task_t *next;

	if (ticks_now > last_run_timer_counter) {
		// Fast path
		if (ticks_now < next_task_timer_counter) {
			last_run_timer_counter = ticks_now;
			return;
		} else {
			// Ensure minimum latency by executing deadline task first
			if (task) {
				os_remove_task(task);
				task->run(task->ctx);
			}
		}
	}
	// slow path

	os_sync_time();
	task = os_tasks;
	// There could be more tasks scheduled at the same time, find them!
	while (task) {
		next = task->next;
		if (TIME_GE(last_os_time, task->deadline)) {
			os_remove_task(task);
			task->run(task->ctx);
		}
		task = next;
	}
	os_recalculate_next_deadline();
	last_run_timer_counter = ticks_now;
}

static void os_delay_slowpath(uint32_t us) {
	os_time_t deadline;

	os_sync_time();
	deadline = last_os_time;
	time_add_us(&deadline, us);
	while (TIME_GE(deadline, last_os_time)) {
		os_sync_time();
	}
}

void os_delay(uint32_t us) {
	uint32_t ticks_now = timer_get_counter(OS_TIMER);
	uint32_t ticks = US_TO_TICKS(us);
	if (OS_TIMER_TOP <= ticks ||
	    OS_TIMER_TOP - ticks < ticks_now) {
		os_delay_slowpath(us);
	} else {
		uint32_t ticks_then = ticks_now + ticks;
		while (timer_get_counter(OS_TIMER) < ticks_then) {
			// depend on ticks_then to make absolutely sure this not optimzed out
			__asm__ volatile("" : "+g" (ticks_then) : :);
		}
	}
}
