#include "screen.h"
#include "string.h"
#include "io.h"
#include <stdbool.h>
#include "shell.h"
#include "kmalloc.h"
#include "task.h"
#include "timer.h"
#include "pci.h"
#include "math.h"
#include "predictor.h"
#include "nlp.h"
#include <stdint.h>
#include "intent_learner.h"


char shell_buffer[256];
int buffer_idx = 0;
int tab_press_count = 0;

// Helper function to compare command with argument for shell
bool cmd_starts_with(const char* cmd, const char* prefix) {
    int i = 0;
    while (prefix[i] && cmd[i] == prefix[i]) i++;
    return prefix[i] == '\0';
}

void shell_execute(char* cmd) {
    if (cmd[0] == '\0') return;
    
    // Predict using learned model
    IntentPrediction pred = predict_intent_learned(cmd);
    
    if (pred.confidence > 40.0f) {
        print_string("\n[AI] I think you mean: ");
        print_string(pred.intent);
        print_string(" [");
        kprint_dec((uint32_t)pred.confidence);
        print_string("%]\n");
        
        // Execute
        if (strcmp(pred.intent, "mem") == 0) {
            print_string("Physical Memory: 256 MB\n");
        }
        else if (strcmp(pred.intent, "ps") == 0) {
            task_list();
        }
        // ... etc ...
        
        print_string("\n");
    }
    else if (cmd_starts_with(cmd, "correct ")) {
        const char* correct_intent = cmd + 8;  // Skip "correct "
        
        print_string("\n[LEARNING] You say it should be: ");
        print_string(correct_intent);
        print_string("\n");
        
        // Get the last input (from context)
        train_intent(correct_intent, shell_buffer);
        
        print_string("[LEARNING] I'll remember that!\n\n");
    }
    else if (strcmp(cmd, "show vocab") == 0) {
        show_learned_vocabulary();
        show_learned_intents();
    }
    else {
        print_string("\n[AI] Not sure what you mean :(");
        print_string("\n[AI] Try: 'correct <intent>' to teach me\n");
        print_string("Example: correct mem\n\n");
    }
}
//shell input function
void shell_input(char c)
{
    if (c == '\n') {

        shell_buffer[buffer_idx] = '\0';
        print_char('\n');

        shell_execute(shell_buffer);

        print_string("JARVIS $ ");

        buffer_idx = 0;
        return;
    }

    if (c == '\b' && buffer_idx > 0) {

        buffer_idx--;

        print_char('\b');
        print_char(' ');
        print_char('\b');

        return;
    }

    if (buffer_idx < 255 && c >= ' ') {

        shell_buffer[buffer_idx++] = c;
        print_char(c);
    }
}

//shell task function
void shell_task(void)
{

    print_string("\nJARVIS OS - Semantic Mode Enabled\n");
    print_string("Type natural language commands.\n\n");
    print_string("JARVIS $ ");

    while (1) {
        task_sleep(1);
    }
}
