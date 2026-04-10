#ifndef AGENT_H
#define AGENT_H

#include <stdint.h>

// Initialize the agentic subsystem (creates the /agent/db directories on FAT32)
void agent_init(void);

// Set a context variable in the agent database
void agent_ctx_set(const char* key, const char* value);

// Get a context variable from the agent database
void agent_ctx_get(const char* key);

// Start an autonomous agent task
void agent_task(const char* instruction);

#endif // AGENT_H
