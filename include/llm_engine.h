#ifndef LLM_ENGINE_H
#define LLM_ENGINE_H

#include <stdint.h>
#include <stddef.h>

// Simple config for a GPT-2 like model
typedef struct {
    int max_seq_len;
    int vocab_size;
    int num_layers;
    int num_heads;
    int channels; // embedding dimension
} GPT2Config;

typedef struct {
    float* wte; // Token embeddings
    float* wpe; // Positional embeddings
    float* ln_1_w;
    float* ln_1_b;
    float* attn_qkv_w;
    float* attn_qkv_b;
    float* attn_proj_w;
    float* attn_proj_b;
    float* ln_2_w;
    float* ln_2_b;
    float* mlp_fc_w;
    float* mlp_fc_b;
    float* mlp_proj_w;
    float* mlp_proj_b;
    float* ln_f_w;
    float* ln_f_b;
} GPT2Weights;

typedef struct {
    GPT2Config config;
    GPT2Weights weights;
    float* x;       // Activation buffer
    float* logits;  // Output logits buffer
} GPT2;

void llm_engine_init(GPT2* model, const GPT2Config* config, uint8_t* weight_blob);
int llm_engine_forward(GPT2* model, int* tokens, int num_tokens);
void llm_engine_generate(GPT2* model, const char* prompt, char* output_buf, int max_new_tokens);

void llm_task_spawn(void); // Function to spawn background AI task
extern GPT2 global_model;

#endif
