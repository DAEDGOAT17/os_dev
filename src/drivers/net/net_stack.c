#include "net_stack.h"
#include "screen.h"
#include "string.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/ip4_addr.h"
#include "lwip/etharp.h"
#include "lwip/netif.h"

// Will expose shell_execute for autonomous callback hooks
extern void shell_execute(char* cmd);

// *** CHANGE THIS to match the exact output of 'ollama list' on your server ***
#define OLLAMA_MODEL "phi3:latest"

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

// Decode a \uXXXX Unicode escape sequence into a single char (ASCII only)
static char decode_unicode_escape(const char* p) {
    // p points to the 4 hex digits after \u
    unsigned int val = 0;
    for (int i = 0; i < 4; i++) {
        val <<= 4;
        char c = p[i];
        if (c >= '0' && c <= '9') val |= (c - '0');
        else if (c >= 'a' && c <= 'f') val |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val |= (c - 'A' + 10);
    }
    return (char)(val & 0x7F); // ASCII only
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
            // Handle \uXXXX Unicode escapes (e.g. \u003c = '<', \u003e = '>')
            if (*resp == '\\' && *(resp+1) == 'u' &&
                *(resp+2) && *(resp+3) && *(resp+4) && *(resp+5)) {
                char decoded = decode_unicode_escape(resp + 2);
                resp += 6; // consume \uXXXX
                // Now treat 'decoded' as the current character
                if (decoded == '<') {
                    // Check for EXEC_CMD tag
                    if (resp[0]=='E' && resp[1]=='X' && resp[2]=='E' &&
                        resp[3]=='C' && resp[4]=='_' && resp[5]=='C' &&
                        resp[6]=='M' && resp[7]=='D' && resp[8]==':') {
                        catching = 1;
                        resp += 9; // Skip EXEC_CMD:
                    } else {
                        if (catching) intercept_cmd[intercept_idx++] = decoded;
                        else print_char(decoded);
                    }
                } else if (decoded == '>' && catching) {
                    catching = 0;
                    intercept_cmd[intercept_idx] = '\0';
                    intercept_idx = 0;
                    set_text_color(MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
                    print_string("\n[System: AI Agent Executing Autonomous Task: `");
                    print_string(intercept_cmd);
                    print_string("`]\n");
                    reset_text_color();
                    shell_execute(intercept_cmd);
                    set_text_color(MAKE_COLOR(COLOR_LIGHT_MAGENTA, COLOR_BLACK));
                } else {
                    if (catching) intercept_cmd[intercept_idx++] = decoded;
                    else print_char(decoded);
                }
                continue;
            }

            // Handle \n literal escape
            if (*resp == '\\' && *(resp+1) == 'n') {
                print_char('\n');
                resp += 2;
                continue;
            }

            // Handle literal < (fallback if model sends raw angle brackets)
            if (*resp == '<' && *(resp+1) == 'E' && *(resp+2) == 'X') {
                catching = 1;
                resp += 10; // Skip <EXEC_CMD:
                continue;
            }
            if (catching && *resp == '>') {
                catching = 0;
                intercept_cmd[intercept_idx] = '\0';
                intercept_idx = 0;
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

static char rx_tcp_buffer[4096];

static err_t ollama_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg;
    (void)err;
    if (!p) {
        tcp_close(tpcb);
        print_string("\nNetwork: Connection closed by Ollama.\nJARVIS [/] $ ");
        return ERR_OK;
    }
    
    // Safely copy payload to a null-terminated buffer before parsing
    int copy_len = p->tot_len;
    if (copy_len > 4095) copy_len = 4095;
    
    pbuf_copy_partial(p, rx_tcp_buffer, copy_len, 0);
    rx_tcp_buffer[copy_len] = '\0';
    
    ollama_parse_json(rx_tcp_buffer);
    
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

void ollama_mock_intercept() {
    print_string("Simulating AI Server Network Response...\n");
    char* test_payload = "HTTP/1.1 200 OK\r\n\r\n{\"model\":\"" OLLAMA_MODEL "\",\"response\":\"I have investigated your hardware state. I will now autonomously scan the PCI bus for you.\\n<EXEC_CMD:pci storage>\"}";
    ollama_parse_json(test_payload);
}

static err_t ollama_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
    (void)arg;
    if (err != ERR_OK) {
        print_string("Network: Connection to Ollama FAILED (err=");
        kprint_dec((int)err);
        print_string(").\nJARVIS [/] $ ");
        return err;
    }
    
    print_string("Network: Connected! Using model: " OLLAMA_MODEL "\n");
    
    char json_body[512];
    strcpy(json_body, "{\"model\": \"" OLLAMA_MODEL "\", \"prompt\": \"You are the core intelligence of Jarvis OS. Memory is stable. You can control this terminal. Wrap OS commands in <EXEC_CMD:command>. User says: ");
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
    strcat(request, "Connection: close\r\n");       // CRITICAL: tells server not to wait for more data
    strcat(request, "Content-Length: ");
    strcat(request, len_str);
    strcat(request, "\r\n\r\n");
    strcat(request, json_body);
    
    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    return ERR_OK;
}

// Error callback — called by lwIP when a TCP connection is aborted or reset
static void ollama_err_cb(void *arg, err_t err) {
    (void)arg;
    print_string("\nNetwork: TCP connection error (err=");
    kprint_dec((int)err);
    print_string(") — Ollama unreachable or connection reset.\nJARVIS [/] $ ");
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

    // --- ARP Pre-warm ---
    // Sending etharp_query() BEFORE tcp_connect() forces lwIP to immediately
    // send an ARP request for the server's MAC. The shell_task() poll loop
    // will process the ARP reply in the next few milliseconds, so that by
    // the time tcp_connect() fires its SYN, the ARP table is already populated
    // and the SYN goes out immediately on the wire instead of being queued
    // behind an ARP round-trip.
    extern struct netif *netif_default;
    if (netif_default) {
        ip_addr_t server_ipaddr;
        ip_addr_copy_from_ip4(server_ipaddr, server);
        etharp_query(netif_default, ip_2_ip4(&server_ipaddr), NULL);
        print_string("Network: ARP pre-warm sent for ");
        print_string(ip_str);
        print_string("\n");
    }
    
    tcp_recv(pcb, ollama_recv_cb);
    tcp_err(pcb, ollama_err_cb);   // Register error callback to catch refused/reset connections
    err_t err = tcp_connect(pcb, &server, 11434, ollama_connected_cb);
    
    if (err != ERR_OK) {
        print_string("Network: Failed to initiate TCP routing.\n");
        tcp_close(pcb);
    }
}

// --- ICMP PING IMPLEMENTATION ---
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "timer.h"

static struct raw_pcb *ping_pcb = NULL;
static uint32_t ping_start_time = 0;
static ip4_addr_t ping_target;

static u8_t ping_recv_cb(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr) {
    (void)arg;
    (void)pcb;
    
    if (p->tot_len >= sizeof(struct icmp_echo_hdr)) {
        struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)p->payload;
        if (iecho->type == ICMP_ER) { // Echo Reply
            uint32_t end_time = timer_get_ticks();
            uint32_t rtt = (end_time - ping_start_time) * 10; // Assuming 100Hz PIT = 10ms per tick
            
            print_string("64 bytes from ");
            print_string(ip4addr_ntoa(ip_2_ip4(addr)));
            print_string(": icmp_seq=");
            kprint_dec(lwip_ntohs(iecho->seqno));
            print_string(" time=");
            kprint_dec(rtt);
            print_string("ms\n");
            
            pbuf_free(p);
            return 1; // Ate it
        }
    }
    return 0; // Didn't eat it
}

void ping_request(const char* ip_str) {
    if (!ip4addr_aton(ip_str, &ping_target)) {
        print_string("Ping: Invalid IP address.\n");
        return;
    }

    if (!ping_pcb) {
        ping_pcb = raw_new(IP_PROTO_ICMP);
        if (!ping_pcb) {
            print_string("Ping: Could not create RAW PCB.\n");
            return;
        }
        raw_recv(ping_pcb, ping_recv_cb, NULL);
        raw_bind(ping_pcb, IP4_ADDR_ANY);
    }

    struct pbuf *p = pbuf_alloc(PBUF_IP, sizeof(struct icmp_echo_hdr), PBUF_RAM);
    if (!p) {
        print_string("Ping: Out of memory for pbuf.\n");
        return;
    }

    struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)p->payload;
    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id = lwip_htons(0xBEEF);
    iecho->seqno = lwip_htons(1);
    iecho->chksum = inet_chksum(iecho, p->len);

    print_string("PING ");
    print_string(ip_str);
    print_string(" (");
    print_string(ip_str);
    print_string("): 56 data bytes\n");

    ping_start_time = timer_get_ticks();
    raw_sendto(ping_pcb, p, (ip_addr_t *)&ping_target);
    pbuf_free(p);
}

