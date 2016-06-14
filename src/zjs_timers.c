// Copyright (c) 2016, Intel Corporation.

#include <zephyr.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#include <string.h>

#include "jerry.h"
#include "jerry-api.h"

#include "zjs_util.h"

struct zjs_timer_t {
    struct nano_timer timer;
    void *timer_data;
    uint32_t interval;
    bool repeat;
    struct zjs_callback zjs_cb;
    struct zjs_timer_t *next;
};

static struct zjs_timer_t *zjs_timers = NULL;

static void zjs_timer_call_function(struct zjs_callback *cb)
{
    // requires: called only from task context
    //  effects: handles execution of the JS callback when ready
    jerry_value_t rval;
    if (jerry_call_function(cb->js_callback, NULL, &rval, NULL, 0))
        jerry_release_value(&rval);
}

static jerry_object_t *
add_timer(uint32_t interval,
          jerry_object_t *callback,
          bool repeat)
{
    // requires: interval is the time in ticks until expiration; callback is
    //             a JS callback function; repeat is true if the timer
    //             should be repeated until canceled, false if one-shot
    // effects: allocates a new timer item and adds it to the timers list
    struct zjs_timer_t *tm;
    tm = task_malloc(sizeof(struct zjs_timer_t));
    if (!tm) {
        PRINT("error: out of memory allocating timer struct\n");
        return NULL;
    }

    nano_timer_init(&tm->timer, &tm->timer_data);
    tm->zjs_cb.js_callback = jerry_acquire_object(callback);
    tm->zjs_cb.call_function = zjs_timer_call_function;
    tm->interval = interval;
    tm->repeat = repeat;
    tm->next = zjs_timers;
    zjs_timers = tm;

    nano_timer_start(&tm->timer, interval);
    return tm->zjs_cb.js_callback;
}

static bool
delete_timer(jerry_object_t *obj)
{
    // requires: obj is a pointer to a callback object reference acquired in
    //             add_timer earlier
    //  effects: removes the timer from the list and cleans up associated
    //             memory/resources; returns true if associated timer found and
    //             removed, false otherwise
    for (struct zjs_timer_t **ptm = &zjs_timers; *ptm; ptm = &(*ptm)->next) {
        struct zjs_timer_t *tm = *ptm;
        if (obj == tm->zjs_cb.js_callback) {
            jerry_release_object(tm->zjs_cb.js_callback);
            nano_task_timer_stop(&tm->timer);
            *ptm = tm->next;
            task_free(tm);
            return true;
        }
    };
    return false;
}

// native setInterval handler
static bool
native_setInterval_handler(const jerry_object_t *function_obj_p,
                           const jerry_value_t *this_p,
                           jerry_value_t *ret_val_p,
                           const jerry_value_t args_p[],
                           const jerry_length_t args_cnt)
{
    if (!ZJS_IS_OBJ(args_p[0]) || !ZJS_IS_FLOAT32(args_p[1])) {
        PRINT ("native_setInterval_handler: invalid arguments\n");
        return false;
    }

    uint32_t interval = (uint32_t)(args_p[1].u.v_float32 / 1000 *
                                   CONFIG_SYS_CLOCK_TICKS_PER_SEC);
    jerry_object_t *callback = args_p[0].u.v_object;

    jerry_object_t *tid = add_timer(interval, callback, true);
    if (!tid) {
        // TODO: should throw an exception
        PRINT ("Error: timer alloc failed\n");
        return false;
    }

    // FIXME: see if we can't return a real pointer to our struct here somehow
    //   because it's unambiguous
    zjs_init_value_object(ret_val_p, tid);
    return true;
}

// native setInterval handler
static bool
native_clearInterval_handler(const jerry_object_t *function_obj_p,
                             const jerry_value_t *this_p,
                             jerry_value_t *ret_val_p,
                             const jerry_value_t args_p[],
                             const jerry_length_t args_cnt)
{
    if (!ZJS_IS_OBJ(args_p[0])) {
        PRINT ("native_setInterval_handler: invalid arguments\n");
        return false;
    }

    jerry_object_t *tid = args_p[0].u.v_object;

    if (!delete_timer(tid)) {
        // TODO: should throw an exception
        PRINT ("Error: timer not found\n");
        return false;
    }

    return true;
}

void zjs_timers_process_events()
{
    for (struct zjs_timer_t *tm = zjs_timers; tm; tm = tm->next) {
        if (nano_task_timer_test(&tm->timer, TICKS_NONE)) {
            // timer has expired, queue up callback
            zjs_queue_callback(&tm->zjs_cb);

            // reschedule or remove timer
            if (tm->repeat)
                nano_timer_start(&tm->timer, tm->interval);
            else
                delete_timer(tm->zjs_cb.js_callback);
        }
    }
}

void zjs_timers_init()
{
    jerry_object_t *global_obj = jerry_get_global();

    // create the C handler for setInterval JS call
    zjs_obj_add_function(global_obj, native_setInterval_handler, "setInterval");

    // create the C handler for clearInterval JS call
    zjs_obj_add_function(global_obj, native_clearInterval_handler, "clearInterval");
}
