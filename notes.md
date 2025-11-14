Following the original tutorial to implement a multiboot-compliant kernel, some of the following tools were used. I noted some of the below commands to get the original tutorial running:

https://os.phil-opp.com/multiboot-kernel/

## Step 1

Install `nasm`:
```
brew install nasm
```

## Step 2

Cross compile binutils for x86_64:

https://os.phil-opp.com/cross-compile-binutils/


Solve zlib-issue:

```
brew install zlib
```

```
export ZPREFIX="$(brew --prefix zlib)"
export CPPFLAGS="-I$ZPREFIX/include"
export LDFLAGS="-L$ZPREFIX/lib"
```

Configure with the `--with-system-zlib` option:
```
../binutils-2.44.90/configure --target=x86_64-elf --with-system-zlib --prefix="$HOME/opt/cross" \
    --disable-nls --disable-werror \
    --disable-gdb --disable-libdecnumber --disable-readline --disable-sim
```

Patch the missing `doc` folder:

```
mkdir -p gas/doc
make
```

## Step 3

Install Grub (for `grub-mkrescue`)

```
brew install i686-elf-grub
```

Execute it with:
```
> i686-elf-grub-mkrescue
```

## Step 4

Install QEMU

```
brew install qemu
```

Execute it with:
```
> qemu-system-x86_64
```