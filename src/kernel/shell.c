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
#include "intent_config.h"
#include "neural_net.h"


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
    
    // Use neural network for prediction
    NeuralPrediction pred = neural_predict(cmd);
    
    if (pred.confidence > 30.0f) {
        print_string("\n[NEURAL AI] Predicted: ");
        print_string(pred.intent);
        print_string(" [");
        kprint_dec((uint32_t)pred.confidence);
        print_string("%]\n");
        
        // Show confidence for all intents (debugging)
        print_string("  Logits: ");
        for (int i = 0; i < 5; i++) {  // Show top 5
            kprint_dec((uint32_t)(pred.logits[i] * 100));
            print_string("% ");
        }
        print_string("\n\n");
        
        // Execute the predicted intent
        if (strcmp(pred.intent, "mem") == 0) {
            print_string("Physical Memory: 256 MB\n\n");
        }
        else if (strcmp(pred.intent, "ps") == 0) {
            task_list();
        }
        else if (strcmp(pred.intent, "clear") == 0) {
            clear_screen();
        }
        else if (strcmp(pred.intent, "reboot") == 0) {
            print_string("Rebooting...\n");
            sys_reboot();
        }
        else if (strcmp(pred.intent, "help") == 0) {
            print_string("Available commands:\n");
            print_string("- Show memory\n");
            print_string("- List processes\n");
            print_string("- Clear screen\n");
            print_string("- Restart system\n\n");
        }
        else if (cmd_starts_with(cmd, "train ")) {
        const char* rest = cmd + 6;  // Skip "train "
    
    // Parse: "train <intent> <example>"
    // For now, just print
        print_string("\n[NEURAL] Training command received: ");
        print_string(rest);
        print_string("\n\n");
}
    }
    else {
        print_string("\n[NEURAL AI] Not confident (");
        kprint_dec((uint32_t)pred.confidence);
        print_string("%). Try rephrasing.\n\n");
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
