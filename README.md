## code for stm32

### zephyr enviroment

[setup BKM](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)

```shell
    mkdir rtos_ze
    cd rtos_ze
    west init
    west update
    export ZEPHYR_BASE=`pwd`/zephyr
```
zephyr commit id is `829c03bcdca` in my code.

### build for blue pill

```shell
    west build -b stm32f103_bluepill ${ZEPHYR_BASE}/samples/basic/blinky -DBOARD_ROOT=`pwd`
```

### flash

1. use [probe-rs](https://probe.rs) (default)

```shell
    west flash
```

2. use [pyocd](https://pyocd.io)

```shell
    west flash -r pyocd
```

3. use openocd

```shell
    west flash -r openocd
```

### rtt viewer

#### 1. use west rtt(pyocd/openocd)

```shell
    west rtt -r pyocd/openocd
```

#### 2. raw method

##### first. get segger base address

```shell
   export RTT_BASE_ADDR=`grep _SEGGER_RTT build/zephyr/zephyr.map | awk '{print $1}'`
   ```

##### last. run

```shell
   pyocd rtt  -t STM32F103C8 -a ${RTT_BASE_ADDR}
```
