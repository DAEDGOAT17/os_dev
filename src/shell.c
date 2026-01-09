#include "screen.h"
#include "string.h"

#define CMD_BUF_SIZE 128
#define OS_VERSION   "Jarvis OS v0.1"

static char cmd_buffer[CMD_BUF_SIZE];
static int cmd_index = 0;

/* Print shell prompt */
void shell_prompt(void) {
    print_string("\njarvis> ");
}

/* Execute parsed command */
static void shell_execute(const char *cmd) {

    if (strcmp(cmd, "help") == 0) {
        print_string("\nAvailable commands:\n");
        print_string("  help     - Show this message\n");
        print_string("  clear    - Clear the screen\n");
        print_string("  version  - Show OS version\n");
        print_string("  echo     - Print text\n");
    }

    else if (strcmp(cmd, "clear") == 0) {
        clear_screen();
    }

    else if (strcmp(cmd, "version") == 0) {
        print_string("\n");
        print_string(OS_VERSION);
    }

    else if (strncmp(cmd, "echo ", 5) == 0) {
        print_string("\n");
        print_string(cmd + 5);
    }

    else if (cmd[0] != '\0') {
        print_string("\nUnknown command. Type 'help'");
    }
}

/* Receive characters from keyboard */
void shell_input(char c) {

    /* ENTER key */
    if (c == '\n') {
        cmd_buffer[cmd_index] = '\0';
        shell_execute(cmd_buffer);
        cmd_index = 0;
        shell_prompt();
        return;
    }

    /* BACKSPACE */
    if (c == '\b') {
        if (cmd_index > 0) {
            cmd_index--;
            backspace();
        }
        return;
    }

    /* Normal character */
    if (cmd_index < CMD_BUF_SIZE - 1) {
        cmd_buffer[cmd_index++] = c;
        print_char(c);
    }
}
