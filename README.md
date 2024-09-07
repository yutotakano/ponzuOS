## hello.c -> hello.efi

```
clang -target x86_64-pc-win32-coff -mno-red-zone -fno-stack-protector -fshort-wchar -Wall -c hello.c
lld-link /subsytem:efi_application /entry:EfiMain /out:hello.efi hello.o
```

## PonzuLoaderPkg -> Loader.efi

```
cd edk2
source ./edksetup.sh
ln -s dir/to/PonzuLoaderPkg ./
build
```

### View file inside of image

```
hdiutil attach -mountpoint mnt disk.img # or mount
ls mnt
hdiutil detach
```
