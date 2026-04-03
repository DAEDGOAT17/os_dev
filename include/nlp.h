#ifndef NLP_H
#define NLP_H

#include <stdint.h>

/* ================= INTENT TYPES ================= */

typedef enum {
    INTENT_REBOOT,
    INTENT_SHOW_MEMORY,
    INTENT_CLEAR_SCREEN,
    INTENT_SHOW_UPTIME,
    INTENT_UNKNOWN
} intent_t;

/* ================= NLP RESULT ================= */

typedef struct {
    intent_t intent;
    const char* mapped_command;   // shell command equivalent
} nlp_result_t;

/* ================= API ================= */

nlp_result_t nlp_process(const char* sentence);

#endif