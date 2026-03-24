#include "intent_config.h"
#include "intent_learner.h"

// ===== INTENT DATABASE =====
// Each intent has a name and training examples

IntentConfig intent_configs[] = {
    // ===== MEMORY INTENT =====
    {
        .intent_name = "mem",
        .description = "Show memory status",
        .training_examples = {
            "show memory",
            "how much ram",
            "check memory usage",
            "memory status",
            "display memory",
            "ram info",
            "free memory",
            "memory free"
        },
        .example_count = 8
    },
    
    // ===== PROCESS INTENT =====
    {
        .intent_name = "ps",
        .description = "List running processes",
        .training_examples = {
            "list processes",
            "show running tasks",
            "what is running",
            "show processes",
            "list tasks",
            "running tasks",
            "process list",
            "active processes"
        },
        .example_count = 8
    },
    
    // ===== CLEAR INTENT =====
    {
        .intent_name = "clear",
        .description = "Clear the screen",
        .training_examples = {
            "clear screen",
            "clean display",
            "reset screen",
            "erase screen",
            "clear",
            "clean screen",
            "refresh screen"
        },
        .example_count = 7
    },
    
    // ===== REBOOT INTENT =====
    {
        .intent_name = "reboot",
        .description = "Restart the system",
        .training_examples = {
            "restart system",
            "reboot",
            "reboot now",
            "shutdown",
            "restart",
            "power off",
            "system restart"
        },
        .example_count = 7
    },
    
    // ===== HELP INTENT =====
    {
        .intent_name = "help",
        .description = "Show available commands",
        .training_examples = {
            "help",
            "show help",
            "what can you do",
            "available commands",
            "show commands",
            "list commands",
            "help me",
            "command help"
        },
        .example_count = 8
    },
    
    // ===== UPTIME INTENT =====
    {
        .intent_name = "uptime",
        .description = "Show system uptime",
        .training_examples = {
            "uptime",
            "how long running",
            "system uptime",
            "how long up",
            "running time",
            "time running"
        },
        .example_count = 6
    },
    
    // ===== VERSION INTENT =====
    {
        .intent_name = "version",
        .description = "Show system version",
        .training_examples = {
            "version",
            "show version",
            "system version",
            "kernel version",
            "what version"
        },
        .example_count = 5
    },
    
    // ===== VOCABULARY INTENT (Debug) =====
    {
        .intent_name = "vocab",
        .description = "Show learned vocabulary",
        .training_examples = {
            "show vocab",
            "vocabulary",
            "learned words",
            "word list"
        },
        .example_count = 4
    }
};

// Calculate total intents
int intent_config_count = sizeof(intent_configs) / sizeof(IntentConfig);

void load_all_intents(void) {
    for (int i = 0; i < intent_config_count; i++) {
        IntentConfig* config = &intent_configs[i];
        for (int j = 0; j < config->example_count; j++) {
            train_intent(config->intent_name, config->training_examples[j]);
        }
    }
}