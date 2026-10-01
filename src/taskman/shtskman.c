/*
 * shtskman.c: a small task manager. Tasks are 0x200-byte blocks carved from one
 * buffer; the first 8 are the heads of 8 circular task lists, the rest form a free list.
 */

#include "sh2.h"

struct _shTskTASK *shTskTaskListTop[8];
struct _shTskTASK *shTskTaskEmptyTop;
int shTskTaskEmptyNum;
int shTskTaskMax;

void shTSKInitTaskList(unsigned char *buffer, unsigned int size) {
    struct _shTskTASK *ptr;
    int *cptr;
    int i;

    shTskTaskMax = size / sizeof(struct _shTskTASK);
    cptr = (int *)buffer;
    for (i = shTskTaskMax * sizeof(struct _shTskTASK) / sizeof(int) - 1; i >= 0; i--) {
        *cptr++ = 0;
    }
    for (i = 0; i < 8; i++) {
        shTskTaskListTop[i] = (struct _shTskTASK *)buffer + i;
        shTskTaskListTop[i]->exe.next = shTskTaskListTop[i];
        shTskTaskListTop[i]->exe.prev = shTskTaskListTop[i];
    }
    ptr = (struct _shTskTASK *)buffer + 8;
    shTskTaskEmptyTop = ptr;
    for (i = 8; i < shTskTaskMax - 1; i++, ptr++) {
        ptr->exe.next = ptr + 1;
        ptr->exe.atr = 0;
    }
    ptr->exe.next = NULL;
    ptr->exe.atr = 0;
    shTskTaskEmptyNum = shTskTaskMax - 8;
}

struct _shTskTASK *shTSKSetTask(void (*func)(void *), unsigned char num) {
    return shTSKSetTaskPrev(shTskTaskListTop[num], func);
}

struct _shTskTASK *shTSKSetTaskPrev(struct _shTskTASK *task, void (*func)(void *)) {
    struct _shTskTASK *a;
    struct _shTskTASK *b;

    if (shTskTaskEmptyNum != 0) {
        a = shTskTaskEmptyTop;
        b = task->exe.prev;
        shTskTaskEmptyTop = a->exe.next;
        shTskTaskEmptyNum--;
        a->exe.prev = b;
        a->exe.next = task;
        a->exe.adrs = func;
        a->exe.mode = 0;
        a->exe.atr = 0;
        task->exe.prev = a;
        b->exe.next = a;
        return a;
    }
    return NULL;
}

void shTSKDelTask(struct _shTskTASK *task) {
    task->exe.adrs = NULL;
}

struct _shTskTASK *shTSKCutTask(struct _shTskTASK *task) {
    struct _shTskTASK *a;
    struct _shTskTASK *b;

    a = task->exe.prev;
    b = task->exe.next;
    a->exe.next = b;
    b->exe.prev = a;
    task->exe.adrs = NULL;
    task->exe.next = shTskTaskEmptyTop;
    shTskTaskEmptyTop = task;
    shTskTaskEmptyNum++;
    return b;
}

void shTSKExecuteTask(unsigned char num) {
    struct _shTskTASK *ptr;

    ptr = shTskTaskListTop[num]->exe.next;
    while (ptr != shTskTaskListTop[num]) {
        if (ptr->exe.adrs == NULL) {
            ptr = shTSKCutTask(ptr);
        } else {
            ptr->exe.adrs(ptr);
            ptr = ptr->exe.next;
        }
    }
}

struct _shTskTASK *shTSKSearchTaskWithAtr(int atr, struct _shTskTASK *start, unsigned char num) {
    struct _shTskTASK *ptr;

    ptr = start;
    while (ptr != shTskTaskListTop[num]) {
        if (ptr->exe.atr == atr) {
            return ptr;
        }
        ptr = ptr->exe.next;
    }
    return NULL;
}

void shTSKFreeTaskLine(unsigned char num) {
    struct _shTskTASK *ptr;

    ptr = shTskTaskListTop[num]->exe.next;
    while (ptr != shTskTaskListTop[num]) {
        ptr = shTSKCutTask(ptr);
    }
}
