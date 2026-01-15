#ifndef TASK_H
#define TASK_H

#include <stdint.h>

// Task states
#define TASK_READY    0
#define TASK_RUNNING  1
#define TASK_WAITING  2
#define TASK_DEAD     3

// Maximum number of tasks
#define MAX_TASKS 32

// Task control block
typedef struct task {
    uint32_t id;
    char name[32];
    uint32_t state;
    uint32_t priority;
    
    // Function to execute
    void (*function)(void);
    
    // CPU state (for context switching later)
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    
    // Statistics
    uint32_t ticks_used;
    uint32_t wake_time;  // For sleep
    
} task_t;

// Initialize task system
void task_init();

// Create a new task
int task_create(const char* name, void (*function)(void), uint32_t priority);

// Kill a task
void task_kill(uint32_t id);

// Get task by ID
task_t* task_get(uint32_t id);

// List all tasks
void task_list();

// Yield CPU to next task
void task_yield();

// Sleep for ticks
void task_sleep(uint32_t ticks);

#endif