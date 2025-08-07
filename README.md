# ASAN

Fixing ASLR (linux) may need to happen depending on your kernel for ASAN builds. If You get
```
AddressSanitizer:DEADLYSIGNAL
AddressSanitizer:DEADLYSIGNAL
```

Run:

`echo 0 | sudo tee /proc/sys/kernel/randomize_va_space`