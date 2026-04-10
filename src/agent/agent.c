#include "agent.h"
#include "fat32.h"
#include "screen.h"
#include "string.h"

// Initialize the Agent directory structure
void agent_init(void) {
    fat32_mkdir("/agent");
    fat32_mkdir("/agent/db");
}

void agent_ctx_set(const char* key, const char* value) {
    char path[128];
    strcpy(path, "/agent/db/");
    strcat(path, key);
    strcat(path, ".txt");

    int fd = fat32_open(path, 'w');
    if (fd >= 0) {
        fat32_write(fd, value, strlen(value));
        fat32_close(fd);
        set_text_color(MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
        print_string("Agent DB: Set '");
        print_string(key);
        print_string("' successfully.\n");
        reset_text_color();
    } else {
        set_text_color(MAKE_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
        print_string("Agent DB ERROR: Failed to write context.\n");
        reset_text_color();
    }
}

void agent_ctx_get(const char* key) {
    char path[128];
    strcpy(path, "/agent/db/");
    strcat(path, key);
    strcat(path, ".txt");

    int fd = fat32_open(path, 'r');
    if (fd >= 0) {
        char buf[513];
        int bytes;
        set_text_color(MAKE_COLOR(COLOR_YELLOW, COLOR_BLACK));
        print_string("Agent DB Context [");
        print_string(key);
        print_string("]:\n");
        reset_text_color();
        while ((bytes = fat32_read(fd, buf, 512)) > 0) {
            buf[bytes] = '\0';
            print_string(buf);
        }
        fat32_close(fd);
        print_string("\n");
    } else {
        set_text_color(MAKE_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
        print_string("Agent DB ERROR: Context '");
        print_string(key);
        print_string("' not found.\n");
        reset_text_color();
    }
}

void agent_task(const char* instruction) {
    set_text_color(MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    print_string(">> Spawning Agent Task: ");
    print_string(instruction);
    print_string("\n");
    reset_text_color();
    
    // In a full implementation, this triggers an auto-loop 
    // network request to Ollama with the instruction prepended
    extern void ollama_request(const char* ip_str, const char* prompt);
    // Hardcoded IP for current agent connection
    ollama_request("172.16.100.1", instruction);
}
