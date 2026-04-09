#include "net_stack.h"
#include "screen.h"
#include "string.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/ip4_addr.h"

// Will expose shell_execute for autonomous callback hooks
extern void shell_execute(char* cmd);

static char global_prompt[256];
static char global_ip_str[32];

// Helper to calculate Content-Length
static void itoa(int n, char s[]) {
    int i = 0, j, sign;
    if ((sign = n) < 0) n = -n;
    do { s[i++] = n % 10 + '0'; } while ((n /= 10) > 0);
    if (sign < 0) s[i++] = '-';
    s[i] = '\0';
    for (j = i - 1, i = 0; i < j; i++, j--) {
        char c = s[i]; s[i] = s[j]; s[j] = c;
    }
}

void ollama_parse_json(char* payload) {
    char* resp = strstr(payload, "\"response\":\"");
    if (resp) {
        resp += 12; // Skip the JSON key
        set_text_color(MAKE_COLOR(COLOR_LIGHT_MAGENTA, COLOR_BLACK));
        print_string("\nAI: ");
        
        char intercept_cmd[64];
        int intercept_idx = 0;
        int catching = 0;
        
        while (*resp && *resp != '\"') {
            if (*resp == '\\' && *(resp+1) == 'n') {
                print_char('\n');
                resp += 2;
                continue;
            } else if (*resp == '<' && *(resp+1) == 'E' && *(resp+2) == 'X') {
                catching = 1;
                resp += 10; // Skip <EXEC_CMD:
                continue;
            } else if (catching && *resp == '>') {
                catching = 0;
                intercept_cmd[intercept_idx] = '\0';
                set_text_color(MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
                print_string("\n[System: AI Agent Executing Autonomous Task: `");
                print_string(intercept_cmd);
                print_string("`]\n");
                reset_text_color();
                shell_execute(intercept_cmd);
                set_text_color(MAKE_COLOR(COLOR_LIGHT_MAGENTA, COLOR_BLACK));
                resp++;
                continue;
            }
            
            if (catching) {
                intercept_cmd[intercept_idx++] = *resp;
            } else {
                print_char(*resp);
            }
            resp++;
        }
        reset_text_color();
    }
}

static err_t ollama_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg;
    (void)err;
    if (!p) {
        tcp_close(tpcb);
        print_string("\nNetwork: Connection closed by Ollama.\nJARVIS [/] $ ");
        return ERR_OK;
    }
    
    ollama_parse_json((char*)p->payload);
    
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

void ollama_mock_intercept() {
    print_string("Simulating AI Server Network Response...\n");
    char* test_payload = "HTTP/1.1 200 OK\r\n\r\n{\"model\":\"llama3\",\"response\":\"I have investigated your hardware state. I will now autonomously scan the PCI bus for you.\\n<EXEC_CMD:pci storage>\"}";
    ollama_parse_json(test_payload);
}

static err_t ollama_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
    (void)arg;
    if (err != ERR_OK) return err;
    
    print_string("Network: Connected! Assembling JSON & Streaming...\n");
    
    char json_body[512];
    strcpy(json_body, "{\"model\": \"llama3\", \"prompt\": \"You are the core intelligence of Jarvis OS. Memory is stable. You can control this terminal. Wrap OS commands in <EXEC_CMD:command>. User says: ");
    strcat(json_body, global_prompt);
    strcat(json_body, "\", \"stream\": false}");
    
    char len_str[16];
    itoa(strlen(json_body), len_str);
    
    char request[1024];
    strcpy(request, "POST /api/generate HTTP/1.1\r\n");
    strcat(request, "Host: ");
    strcat(request, global_ip_str);
    strcat(request, ":11434\r\n");
    strcat(request, "Content-Type: application/json\r\n");
    strcat(request, "Content-Length: ");
    strcat(request, len_str);
    strcat(request, "\r\n\r\n");
    strcat(request, json_body);
    
    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    return ERR_OK;
}

void ollama_request(const char* ip_str, const char* prompt) {
    int len = strlen(prompt);
    if (len > 255) len = 255;
    strncpy(global_prompt, prompt, len);
    global_prompt[len] = '\0';
    
    strncpy(global_ip_str, ip_str, 31);
    global_ip_str[31] = '\0';
    
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) { print_string("Network: Out of memory for Socket.\n"); return; }
    
    ip4_addr_t server;
    if (!ip4addr_aton(ip_str, &server)) {
        print_string("Network: Invalid IP address format!\n");
        tcp_close(pcb);
        return;
    }
    
    tcp_recv(pcb, ollama_recv_cb);
    err_t err = tcp_connect(pcb, &server, 11434, ollama_connected_cb);
    
    if (err != ERR_OK) {
        print_string("Network: Failed to initiate TCP routing.\n");
        tcp_close(pcb);
    }
}
