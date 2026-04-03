#!/usr/bin/env python3
"""
NanoGPT 15M Weight Exporter for Jarvis OS
Downloads a standard tiny GPT config (like Andrej Karpathy's NanoGPT), 
initializes weights (or downloads pre-trained), and serializes them linearly for C.
"""
import struct
import numpy as np

# A typical NanoGPT config (15M params)
# config: vocab_size=50257, block_size=256, n_layer=4, n_head=4, n_embd=256
config = {
    'vocab_size': 50257,
    'block_size': 256,
    'n_layer': 4,
    'n_head': 4,
    'n_embd': 256
}

def write_tensor(fp, tensor, name):
    print(f"[{name}] {tensor.shape} -> {tensor.size * 4 / 1024 / 1024:.2f} MB")
    # Tensors are cast to float32 natively, ensure C continuously packing
    fp.write(tensor.astype(np.float32).tobytes())

def generate_mock_weights():
    """
    Since huggingface might be tricky depending on the user environment,
    we'll generate mock localized parameters initialized with standard deviations
    just to prove the math OS loop works before downloading a real HF checkpoint.
    """
    V = config['vocab_size']
    T = config['block_size']
    C = config['n_embd']
    L = config['n_layer']
    
    print(f"Generating binary payload for {C} width, {L} layers...")
    
    with open("nanogpt_15m.bin", "wb") as f:
        # 1. wte and wpe
        write_tensor(f, np.random.randn(V, C) * 0.02, "wte")
        write_tensor(f, np.random.randn(T, C) * 0.02, "wpe")
        
        # 2. Transformer layers
        for i in range(L):
            write_tensor(f, np.ones(C), f"ln_1_w_{i}")
            write_tensor(f, np.zeros(C), f"ln_1_b_{i}")
            
            write_tensor(f, np.random.randn(C, 3*C) * 0.02, f"attn_qkv_w_{i}")
            write_tensor(f, np.zeros(3*C), f"attn_qkv_b_{i}")
            
            write_tensor(f, np.random.randn(C, C) * 0.02, f"attn_proj_w_{i}")
            write_tensor(f, np.zeros(C), f"attn_proj_b_{i}")
            
            write_tensor(f, np.ones(C), f"ln_2_w_{i}")
            write_tensor(f, np.zeros(C), f"ln_2_b_{i}")
            
            write_tensor(f, np.random.randn(C, 4*C) * 0.02, f"mlp_fc_w_{i}")
            write_tensor(f, np.zeros(4*C), f"mlp_fc_b_{i}")
            
            write_tensor(f, np.random.randn(4*C, C) * 0.02, f"mlp_proj_w_{i}")
            write_tensor(f, np.zeros(C), f"mlp_proj_b_{i}")
            
        # 3. Output layer norm
        write_tensor(f, np.ones(C), "ln_f_w")
        write_tensor(f, np.zeros(C), "ln_f_b")
        
    print("\nSaved -> nanogpt_15m.bin")
    print("Please use `mcopy -i disk.img nanogpt_15m.bin ::/` to inject into your FAT32 disk!")

if __name__ == "__main__":
    generate_mock_weights()