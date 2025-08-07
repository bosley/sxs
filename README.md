# ASAN

Fixing ASLR (linux) may need to happen depending on your kernel for ASAN builds. If You get "

`echo 0 | sudo tee /proc/sys/kernel/randomize_va_space`