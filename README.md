# CedarC 1.04 README

## Introduction
Video Engine userspace driver of Allwinner H6, including H264,H265,VP9 decoder.

## How to Compile

```
./bootstrap
./configure --prefix=${PWD}/out --host=arm-linux-gnueabihf CFLAGS="-DCONF_KERNEL_VERSION_3_10" CPPFLAGS="-DCONF_KERNEL_VERSION_3_10"
make -j8 && make install
```


## Decoding with CedarX
```
TBD
```
