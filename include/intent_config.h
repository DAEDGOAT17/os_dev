#ifndef INTENT_CONFIG_H
#define INTENT_CONFIG_H

// Define all available intents
typedef struct {
    const char* intent_name;        // "mem", "ps", "clear", etc.
    const char* description;        // What this intent does
    const char* training_examples[10];  // Example phrases
    int example_count;
} IntentConfig;

// Declare the intent list (defined in intent_list.c)
extern IntentConfig intent_configs[];
extern int intent_config_count;

// Load all intents into the learning system
void load_all_intents(void);

#endif