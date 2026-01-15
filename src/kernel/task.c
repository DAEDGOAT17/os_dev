#include "task.h"
#include "kmalloc.h"
#include "screen.h"
#include "timer.h"
#include "string.h"

static task_t tasks[MAX_TASKS];
static uint32_t next_task_id = 0;
static uint32_t current_task_id = 0;

void task_init() {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_DEAD;
        tasks[i].id = 0;
    }
    
    // Create idle task (ID 0)
    tasks[0].id = next_task_id++;
    tasks[0].state = TASK_RUNNING;
    tasks[0].priority = 0;
    tasks[0].ticks_used = 0;
    for (int i = 0; i < 32; i++) tasks[0].name[i] = 0;
    tasks[0].name[0] = 'i'; tasks[0].name[1] = 'd'; 
    tasks[0].name[2] = 'l'; tasks[0].name[3] = 'e';
}

int task_create(const char* name, void (*function)(void), uint32_t priority) {
    // Find free slot
    for (int i = 1; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_DEAD) {
            tasks[i].id = next_task_id++;
            tasks[i].state = TASK_READY;
            tasks[i].priority = priority;
            tasks[i].function = function;
            tasks[i].ticks_used = 0;
            tasks[i].wake_time = 0;
            
            // Copy name
            int j;
            for (j = 0; j < 31 && name[j]; j++) {
                tasks[i].name[j] = name[j];
            }
            tasks[i].name[j] = '\0';
            
            return tasks[i].id;
        }
    }
    return -1;  // No free slots
}

void task_kill(uint32_t id) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].id == id) {
            tasks[i].state = TASK_DEAD;
            return;
        }
    }
}

task_t* task_get(uint32_t id) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].id == id && tasks[i].state != TASK_DEAD) {
            return &tasks[i];
        }
    }
    return NULL;
}

void task_list() {
    print_string("PID  State    Name\n");
    print_string("---  -------  ----------------\n");
    
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state != TASK_DEAD) {
            kprint_hex(tasks[i].id);
            print_string("  ");
            
            switch (tasks[i].state) {
                case TASK_READY:   print_string("READY   "); break;
                case TASK_RUNNING: print_string("RUNNING "); break;
                case TASK_WAITING: print_string("WAITING "); break;
            }
            
            print_string(" ");
            print_string(tasks[i].name);
            print_char('\n');
        }
    }
}

void task_yield() {
    // Simple round-robin scheduler
    int start = current_task_id;
    
    do {
        current_task_id = (current_task_id + 1) % MAX_TASKS;
        
        // Check if task can run
        if (tasks[current_task_id].state == TASK_READY) {
            // Check if sleeping
            if (tasks[current_task_id].wake_time > 0) {
                if (timer_get_ticks() >= tasks[current_task_id].wake_time) {
                    tasks[current_task_id].wake_time = 0;
                    break;
                }
            } else {
                break;
            }
        }
    } while (current_task_id != start);
    
    // Execute task function
    if (tasks[current_task_id].function) {
        tasks[current_task_id].state = TASK_RUNNING;
        tasks[current_task_id].function();
        tasks[current_task_id].state = TASK_READY;
    }
}

void task_sleep(uint32_t ticks) {
    task_t* current = &tasks[current_task_id];
    current->wake_time = timer_get_ticks() + ticks;
    current->state = TASK_WAITING;
}