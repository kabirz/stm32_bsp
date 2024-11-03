#include <zephyr/kernel.h>
#include <zephyr/app_version.h>
#include <zephyr/drivers/gpio.h>
#define USER_NODE DT_PATH(zephyr_user)

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modbus_main, CONFIG_MODBUS_APP_LOG_LEVEL);

int main(void)
{
    const struct gpio_dt_spec status_gpios = GPIO_DT_SPEC_GET(USER_NODE, status_gpios);

    gpio_pin_configure_dt(&status_gpios, GPIO_OUTPUT_INACTIVE);
    LOG_INF("build time: %s-%s, board: %s, system clk: %dMHz, version: %s",
            __DATE__, __TIME__,
            CONFIG_BOARD_TARGET,
            CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC/1000000,
            APP_VERSION_STRING
    );
    while (1) {
        gpio_pin_set_dt(&status_gpios, 0);
    	k_msleep(2700);
    	gpio_pin_set_dt(&status_gpios, 1);
    	k_msleep(300);
    }
    return 0;
}

