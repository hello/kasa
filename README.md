To build:
```
cd ambarella/
source build/env/armv7ahf-linaro-gcc.env
cd boards/s2lm_ironman/
make sync_build_mkcfg
make s2lm_ironman_config
make defconfig_public_linux
make -j 8
```
