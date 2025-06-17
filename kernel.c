void puts(const char* str) {
  volatile char* video = (volatile char*) 0xB8000;
  int i = 0;
  while (str[i]) {
      video[i * 2] = str[i];      // character
      video[i * 2 + 1] = 0x07;    // light gray on black
      i++;
  }
}

void kmain(void) {
  puts("Hello, World!  pankaj");
  while (1);
}
