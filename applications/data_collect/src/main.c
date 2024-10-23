#include <init.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main_modbus, LOG_LEVEL_INF);

int main(void)
{
    while (1) {
    	k_msleep(500);
    }
    return 0;
}

