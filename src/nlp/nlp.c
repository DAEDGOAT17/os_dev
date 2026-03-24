#include "nlp.h"
#include "string.h"

/* ================= INTERNAL HELPERS ================= */

/* Simple substring check (assumes your string.c has strstr) */
static int contains(const char* sentence, const char* word)
{
    return strstr(sentence, word) != 0;
}

/* ================= CLASSIFIER ================= */

static intent_t classify_intent(const char* sentence)
{
    /* ---- REBOOT ---- */
    if (contains(sentence, "restart") ||
        contains(sentence, "reboot")  ||
        contains(sentence, "shutdown"))
    {
        return INTENT_REBOOT;
    }

    /* ---- MEMORY ---- */
    if (contains(sentence, "memory") ||
        contains(sentence, "ram"))
    {
        return INTENT_SHOW_MEMORY;
    }

    /* ---- CLEAR SCREEN ---- */
    if (contains(sentence, "clear") &&
        contains(sentence, "screen"))
    {
        return INTENT_CLEAR_SCREEN;
    }

    /* ---- UPTIME ---- */
    if (contains(sentence, "uptime") ||
        contains(sentence, "how long") ||
        contains(sentence, "running"))
    {
        return INTENT_SHOW_UPTIME;
    }

    return INTENT_UNKNOWN;
}

/* ================= INTENT → SHELL COMMAND ================= */

static const char* intent_to_shell(intent_t intent)
{
    switch (intent)
    {
        case INTENT_REBOOT:        return "reboot";
        case INTENT_SHOW_MEMORY:   return "mem";
        case INTENT_CLEAR_SCREEN:  return "clear";
        case INTENT_SHOW_UPTIME:   return "uptime";
        default:                   return "unknown";
    }
}

/* ================= PUBLIC API ================= */

nlp_result_t nlp_process(const char* sentence)
{
    nlp_result_t result;

    result.intent = classify_intent(sentence);
    result.mapped_command = intent_to_shell(result.intent);

    return result;
}