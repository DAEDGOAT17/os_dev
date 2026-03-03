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


char shell_buffer[256];
int buffer_idx = 0;
int tab_press_count = 0;

// Helper function to compare command with argument for shell
bool cmd_starts_with(const char* cmd, const char* prefix) {
    int i = 0;
    while (prefix[i] && cmd[i] == prefix[i]) i++;
    return prefix[i] == '\0';
}

void shell_execute(char* cmd)
{
    if (cmd[0] == '\0')
        return;

    Prediction pred = predict_intent(cmd);

    if (pred.confidence > 20.0f) {

        if (strcmp(pred.intent, "mem") == 0) {

            print_string("\n=== Memory Status ===\n\n");
            print_string("Physical Memory : 256 MB\n\n");

        }
        else if (strcmp(pred.intent, "ps") == 0) {

            print_string("\nRunning Tasks:\n");
            task_list();
            print_string("\n");

        }
        else if (strcmp(pred.intent, "clear") == 0) {

            clear_screen();

        }
        else if (strcmp(pred.intent, "reboot") == 0) {

            print_string("Rebooting...\n");
            sys_reboot();

        }
        else if (strcmp(pred.intent, "help") == 0) {

            print_string("\nAvailable semantic commands:\n");
            print_string("- memory\n- processes\n- reboot\n- clear\n\n");
        }

        print_string("Intent ---> shell command : ");
        print_string(pred.intent);
        print_string("  (");
        kprint_dec((uint32_t)pred.confidence);
        print_string("% confidence)\n\n");
    }
    else {

        print_string("\nNot confident in understanding.\n");
        print_string("Try rephrasing.\n\n");
    }
}
//shell input function
void shell_input(char c)
{
    /* ================= ENTER ================= */
    if (c == '\n') {

        shell_buffer[buffer_idx] = '\0';
        print_char('\n');

        shell_execute(shell_buffer);

        print_string("JARVIS $ ");

        buffer_idx = 0;
        return;
    }

    /* ================= BACKSPACE ================= */
    if (c == '\b' && buffer_idx > 0) {

        buffer_idx--;

        print_char('\b');
        print_char(' ');
        print_char('\b');

        return;
    }

    /* ================= NORMAL CHAR ================= */
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
