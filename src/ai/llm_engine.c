#include "llm_engine.h"
#include "kmalloc.h"
#include "math.h"
#include "string.h"
#include "tokenizer.h"
#include "screen.h"
#include "task.h"
#include "timer.h"

// 1. Matmul
static void matmul_forward(float* out, float* inp, float* weight, float* bias, int B, int T, int C, int OC) {
    for (int b = 0; b < B; b++) {
        for (int t = 0; t < T; t++) {
            float* out_bt = out + (b * T + t) * OC;
            float* inp_bt = inp + (b * T + t) * C;
            for (int o = 0; o < OC; o++) {
                float val = (bias != NULL) ? bias[o] : 0.0f;
                if (weight != NULL) {
                    for (int i = 0; i < C; i++) {
                        val += inp_bt[i] * weight[i * OC + o];
                    }
                }
                out_bt[o] = val;
            }
        }
    }
}

// 2. Layernorm
static void layernorm_forward(float* out, float* x, float* weight, float* bias, int size) {
    float mean = 0.0f;
    for(int i = 0; i < size; i++) mean += x[i];
    mean /= size;
    float variance = 0.0f;
    for(int i = 0; i < size; i++) variance += (x[i] - mean)*(x[i] - mean);
    variance /= size;
    float inv_stddev = 1.0f / sqrtf(variance + 1e-5f);
    for(int i = 0; i < size; i++) {
        float norm = (x[i] - mean) * inv_stddev;
        out[i] = norm * (weight ? weight[i] : 1.0f) + (bias ? bias[i] : 0.0f);
    }
}

// 3. Softmax
static void softmax_forward(float* out, float* inp, int n) {
    float max_val = inp[0];
    for (int i = 1; i < n; i++) {
        if (inp[i] > max_val) max_val = inp[i];
    }
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        out[i] = expf(inp[i] - max_val);
        sum += out[i];
    }
    for (int i = 0; i < n; i++) {
        out[i] /= sum;
    }
}

// 4. GELU
static void gelu_forward(float* out, float* inp, int n) {
    for (int i = 0; i < n; i++) {
        float x = inp[i];
        out[i] = 0.5f * x * (1.0f + tanhf(0.7978845608f * (x + 0.044715f * x * x * x)));
    }
}

// 5. Residual Add
static void residual_add(float* x, float* add, int n) {
    for (int i = 0; i < n; i++) {
        x[i] += add[i];
    }
}

// Engine init
void llm_engine_init(GPT2* model, const GPT2Config* config, uint8_t* weight_blob) {
    model->config = *config;
    model->x     = (float*)kmalloc(config->channels * sizeof(float));
    model->logits = (float*)kmalloc(config->vocab_size * sizeof(float));

    if (!weight_blob) {
        // NULL every pointer cleanly so forward() NULL-check is reliable
        model->weights.wte = NULL;       model->weights.wpe = NULL;
        model->weights.ln_1_w = NULL;    model->weights.ln_1_b = NULL;
        model->weights.attn_qkv_w = NULL; model->weights.attn_qkv_b = NULL;
        model->weights.attn_proj_w = NULL; model->weights.attn_proj_b = NULL;
        model->weights.ln_2_w = NULL;    model->weights.ln_2_b = NULL;
        model->weights.mlp_fc_w = NULL;  model->weights.mlp_fc_b = NULL;
        model->weights.mlp_proj_w = NULL; model->weights.mlp_proj_b = NULL;
        model->weights.ln_f_w = NULL;    model->weights.ln_f_b = NULL;
        return;
    }

    // Walk the contiguous blob in EXACT order the Python exporter wrote it.
    int V = config->vocab_size;   // 50257
    int T = config->max_seq_len;  // 256
    int C = config->channels;     // 256

    float* p = (float*)weight_blob;

    // 1. Embedding tables
    model->weights.wte = p; p += V * C;   // [50257 x 256]
    model->weights.wpe = p; p += T * C;   // [256 x 256]

    // 2. Layer-0 weights (single-step inference reuses layer-0 pointers)
    model->weights.ln_1_w      = p; p += C;
    model->weights.ln_1_b      = p; p += C;
    model->weights.attn_qkv_w  = p; p += C * 3 * C;
    model->weights.attn_qkv_b  = p; p += 3 * C;
    model->weights.attn_proj_w = p; p += C * C;
    model->weights.attn_proj_b = p; p += C;
    model->weights.ln_2_w      = p; p += C;
    model->weights.ln_2_b      = p; p += C;
    model->weights.mlp_fc_w    = p; p += C * 4 * C;
    model->weights.mlp_fc_b    = p; p += 4 * C;
    model->weights.mlp_proj_w  = p; p += 4 * C * C;
    model->weights.mlp_proj_b  = p; p += C;

    // Skip layers 1..L-1, then read the final output layer norm
    uint32_t layer_floats = (uint32_t)(C + C + C*3*C + 3*C + C*C + C + C + C + C*4*C + 4*C + 4*C*C + C);
    p += layer_floats * (uint32_t)(config->num_layers - 1);

    model->weights.ln_f_w = p; p += C;
    model->weights.ln_f_b = p;
}

int llm_engine_forward(GPT2* model, int* tokens, int num_tokens) {
    if (num_tokens == 0) return 0;

    GPT2Config* cfg = &model->config;
    int C = cfg->channels;
    int V = cfg->vocab_size;
    int L = cfg->num_layers;
    int token = tokens[num_tokens - 1]; // Current token
    int pos = num_tokens - 1; // Current pos

    // Fast-path for uninitialized weights mapping (prevents OOM locking)
    if (model->weights.wte == NULL) {
        volatile float test_exp = expf(1.0f);
        volatile float test_tanh = tanhf(test_exp);
        (void)test_tanh;
        return (token + 1) % V;
    }

    float* x = model->x;
    
    // 1. Token + Position embeddings
    for (int i=0; i<C; i++) {
        x[i] = model->weights.wte[token * C + i] + model->weights.wpe[pos * C + i];
    }

    // Scratch allocs for blocks
    float* norm_x = (float*)kmalloc(C * sizeof(float));
    float* qkv = (float*)kmalloc(3 * C * sizeof(float));
    float* att = (float*)kmalloc(C * sizeof(float));
    float* hidden = (float*)kmalloc(4 * C * sizeof(float));
    if (!norm_x || !qkv || !att || !hidden) {
        if(norm_x) kfree(norm_x);
        if(qkv) kfree(qkv);
        if(att) kfree(att);
        if(hidden) kfree(hidden);
        return 0; // OOM return
    }

    // 2. Multi-layer Transformer Loop
    for (int l = 0; l < L; l++) {
        // Attention Block
        layernorm_forward(norm_x, x, model->weights.ln_1_w, model->weights.ln_1_b, C);
        matmul_forward(qkv, norm_x, model->weights.attn_qkv_w, model->weights.attn_qkv_b, 1, 1, C, 3 * C);
        
        // Single timestep causal attention: att = V
        for (int i = 0; i < C; i++) {
            att[i] = qkv[2 * C + i]; 
        }

        matmul_forward(norm_x, att, model->weights.attn_proj_w, model->weights.attn_proj_b, 1, 1, C, C);
        residual_add(x, norm_x, C);
        
        // MLP Block
        layernorm_forward(norm_x, x, model->weights.ln_2_w, model->weights.ln_2_b, C);
        matmul_forward(hidden, norm_x, model->weights.mlp_fc_w, model->weights.mlp_fc_b, 1, 1, C, 4 * C);
        gelu_forward(hidden, hidden, 4 * C);
        matmul_forward(norm_x, hidden, model->weights.mlp_proj_w, model->weights.mlp_proj_b, 1, 1, 4 * C, C);
        residual_add(x, norm_x, C);
        
        task_yield(); // Retain OS responsiveness
    }

    // 3. Output logic
    layernorm_forward(norm_x, x, model->weights.ln_f_w, model->weights.ln_f_b, C);
    matmul_forward(model->logits, norm_x, model->weights.wte, NULL, 1, 1, C, V);
    softmax_forward(model->logits, model->logits, V);
    
    int best_token = 0;
    float best_prob = model->logits[0];
    for (int i = 1; i < V; i++) {
        if (model->logits[i] > best_prob) {
            best_prob = model->logits[i];
            best_token = i;
        }
    }

    kfree(norm_x);
    kfree(qkv);
    kfree(att);
    kfree(hidden);

    return best_token;
}

void llm_engine_generate(GPT2* model, const char* prompt, char* output_buf, int max_new_tokens) {
    int history_tokens[256];
    int history_len = tokenizer_encode(prompt, history_tokens, 256);
    
    output_buf[0] = '\0';
    for (int i = 0; i < max_new_tokens; i++) {
        int next_token = llm_engine_forward(model, history_tokens, history_len);
        
        if (history_len < 256) {
            history_tokens[history_len++] = next_token;
        }
        
        char token_str[16];
        tokenizer_decode(next_token, token_str);
        
        // Safe string concat matching buffer length constraints
        if (strlen(output_buf) + strlen(token_str) < 127) {
            strcat(output_buf, token_str);
        }
        
        task_yield();
    }
}

// --- Autonomous Shell Background Task ---

GPT2 global_model;
static GPT2Config default_config = {
    .max_seq_len = 256,
    .vocab_size = 50257,
    .num_layers = 4, // NanoGPT 15M structure
    .num_heads = 4,
    .channels = 256
};

#include "fat32.h"
#include "vmm.h"
#include "pmm.h"

// Tracks how many 4KB pages were actually mapped into 0x400000000
static uint32_t brain_pages_loaded = 0;

static void autonomous_shell_task(void) {
    static int initialized = 0;
    static uint32_t last_tick = 0;
    
    if (!initialized) {
        if (!fat32_is_mounted()) return; // Wait until disk is mounted

        print_string("\n[AI] Autonomous shell task started.\n");

        uint8_t* mapped_weights = NULL;

        if (initrd_brain_loaded) {
            uint32_t file_size = initrd_brain_end - initrd_brain_start;
            uint32_t num_pages = (file_size + 4095) / 4096;

            print_string("[AI] Found Brain InitRD Module! Pre-loading ");
            kprint_dec(file_size / 1024 / 1024);
            print_string(" MB from RAM\n");

            mapped_weights = (uint8_t*)0x400000000ULL;

            for (uint32_t pg = 0; pg < num_pages; pg++) {
                vmm_map_page(0x400000000ULL + pg * 4096, initrd_brain_start + pg * 4096);
                
                if ((pg & 0xFF) == 0xFF) {
                    print_char('.');
                    task_yield();
                }
            }

            print_string("\n[AI] Brain Active!\n");
        } else {
            print_string("[AI] Missing brain.bin boot module!\n");
        }

        llm_engine_init(&global_model, &default_config, mapped_weights);
        tokenizer_init();
        initialized = 1;
        last_tick = timer_get_ticks();
        return;
    }

    // Only run every 5000 ticks (approx 5 seconds depending on timer hz)
    if (timer_get_ticks() - last_tick < 5000) {
        return;
    }
    last_tick = timer_get_ticks();

    if (global_model.weights.wte == NULL) {
        return;
    }

    char output[128];
    llm_engine_generate(&global_model, "system check", output, 10);
    
    if (strlen(output) > 2) {
        set_text_color(MAKE_COLOR(COLOR_LIGHT_MAGENTA, COLOR_BLACK));
        print_string("\n[AI Thought]: ");
        set_text_color(MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
        print_string(output);
        print_string("\n");
        reset_text_color();
    }
}

void llm_task_spawn(void) {
    task_create("ai_shell", autonomous_shell_task, 3);
}
