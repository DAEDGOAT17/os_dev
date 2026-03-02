#include <stdint.h>
#include "string.h"
#include "predictor.h"

#define MAX_COMMANDS 32
#define MAX_CMD_LEN  32
#define MAX_MATCHES 16

typedef struct {
    char name[MAX_CMD_LEN];
    uint32_t usage;
} command_t;

/* --- Command table matching your shell --- */
static command_t commands[MAX_COMMANDS] = {
    {"help", 0},
    {"clear", 0},
    {"mem", 0},
    {"heap", 0},
    {"echo", 0},
    {"version", 0},
    {"ps", 0},
    {"uptime", 0},
    {"reboot", 0},
    {"crash", 0},
    {"pci-scan", 0},
    {"mat-test", 0}
};

static int total_commands = 12;

/* --- Prefix match --- */
static int starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str == 0) return 0;
        if (*str != *prefix) return 0;
        str++;
        prefix++;
    }
    return 1;
}

/* --- Learn usage after execution --- */
void predictor_learn(const char* cmd) {

    /* For commands like "echo hello", extract only first token */
    char temp[MAX_CMD_LEN];
    int i = 0;

    while (cmd[i] && cmd[i] != ' ' && i < MAX_CMD_LEN - 1) {
        temp[i] = cmd[i];
        i++;
    }
    temp[i] = 0;

    for (int j = 0; j < total_commands; j++) {
        if (strcmp(temp, commands[j].name) == 0) {
            commands[j].usage++;
            return;
        }
    }
}

/* --- Suggest command based on prefix + frequency --- */
const char* predictor_suggest(const char* input) {

    if (input[0] == 0)
        return 0;

    int best_index = -1;
    uint32_t best_usage = 0;

    for (int i = 0; i < total_commands; i++) {

        if (starts_with(commands[i].name, input)) {

            if (commands[i].usage >= best_usage) {
                best_usage = commands[i].usage;
                best_index = i;
            }
        }
    }

    if (best_index == -1)
        return 0;

    return commands[best_index].name;
}


#define MAX_MATCHES 16

int predictor_get_matches(const char* input,
                          const char** results,
                          int max_results)
{
    int count = 0;

    for (int i = 0; i < total_commands; i++) {

        if (starts_with(commands[i].name, input)) {

            if (count < max_results)
                results[count] = commands[i].name;

            count++;
        }
    }

    return count;
}


/* Longest common prefix among matches */
int predictor_longest_common_prefix(const char* input,
                                    char* output)
{
    const char* matches[MAX_MATCHES];
    int count = predictor_get_matches(input, matches, MAX_MATCHES);

    if (count == 0)
        return 0;

    int pos = 0;

    while (1) {

        char current = matches[0][pos];
        if (current == 0)
            break;

        for (int i = 1; i < count; i++) {
            if (matches[i][pos] != current)
                goto done;
        }

        output[pos] = current;
        pos++;
    }

done:
    output[pos] = 0;
    return pos;
}