/*
 * dbflow.c: the debug flow checker. Code marks progress with checkpoints;
 * dbFlowCheck() warns when no checkpoint was passed since the last frame, and
 * dbFlowCheckSleepTime() exits the program after the automatic exit time
 * (boot option) of stalled frames.
 */

#include "sh2.h"
#include "sdk/eekernel.h"

static long call_count;
static long sleep_max_report = 119;
static long sleep_max;
static const long sleep_check_interval = 3600;
static long sleep_time;
static long sleep_timeout;
static const int check_halt = 0;
static unsigned long warn_wait;
static unsigned long warn_check_count;
static unsigned long prv_check_count;
static unsigned long check_count;
static char *last_check;

/** Records that the checkpoint `check_point` (a "`name'(file:line)" string) was reached. */
void ___dbFlowSetCheckPoint(char *check_point) {
    check_count++;
    last_check = check_point;
}

/**
 * Checks for progress since the last call: returns 1 when a new checkpoint was reached (or none
 * was ever set), 0 otherwise, with a warning after 120 calls in a row without one.
 */
int dbFlowCheck(void) {
    int ret;

    ret = 1;
    if (last_check) {
        if (prv_check_count == check_count) {
            if (warn_check_count != check_count) {
                if (++warn_wait >= 120) {
                    char *str;

                    str = last_check;
                    if (!str) { str = "(unknown)"; }
                    printf("dbflow.c:54> warning!! flow stopped after `%s'\n", str);
                    warn_check_count = check_count;
                }
            }
            ret = 0;
        } else {
            if (prv_check_count == warn_check_count) {
                char *str;

                str = last_check;
                if (!str) { str = "(unknown)"; }
                printf("dbflow.c:76> reach at `%s'\n", str);
            }
            warn_wait = 0;
        }
        prv_check_count = check_count;
    }
    return ret;
}

/**
 * Arms (`enable` non-zero) or disarms the automatic exit after execEnv_auto_exit_time seconds of
 * stalled frames.
 */
void dbFlowStartCheck(int enable) {
    if (enable) {
        sleep_timeout = execEnv_auto_exit_time * 60;
    } else {
        sleep_timeout = 0;
    }
}

/**
 * Counts stalled frames (`flow_ok` zero) and exits the program when the automatic exit time is
 * reached.
 */
void dbFlowCheckSleepTime(int flow_ok) {
    int exec_timeout;

    call_count++;
    if (flow_ok) {
        sleep_time = 0;
    } else {
        sleep_time++;
    }
    if (sleep_time > sleep_max) {
        sleep_max = sleep_time;
    }
    exec_timeout = sleep_timeout && sleep_time >= sleep_timeout;
    if (!sleep_time || !(call_count % sleep_check_interval) || exec_timeout) {
        if (sleep_max_report < sleep_max) {
            sleep_max_report = sleep_max;
        }
    }
    if (exec_timeout) {
        for (sleep_time = 60; sleep_time > 0; sleep_time--) {
            shSyncVStart(0);
        }
        Exit(0);
    }
}
