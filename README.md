```
clang -target x86_64-pc-win32-coff -mno-red-zone -fno-stack-protector -fshort-wchar -Wall -c hello.c
lld-link /subsytem:efi_application /entry:EfiMain /out:hello.efi hello.o
```
