## stm32f103 c8t6/cbt6


### build

```shell
cmake -B build/Debug
cmake --build build/Debug
```

#### rtt debug:

get rtt base address

```shell
grep _SEGGER_RTT build/Debug/uart_led.map
```
get rtt output with pyocd

```shell
pyocd rtt -t STM32F103C8 -a 0x20000b5c
```