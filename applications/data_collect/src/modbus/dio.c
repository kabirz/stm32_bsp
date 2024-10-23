#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dio_modbus, LOG_LEVEL_INF);

#define USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec di_gpios[] = {
    DT_FOREACH_PROP_ELEM_SEP(USER_NODE, di_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};

static const struct gpio_dt_spec do_gpios[] = {
    DT_FOREACH_PROP_ELEM_SEP(USER_NODE, do_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};

static const struct gpio_dt_spec doled_gpios[] = {
    DT_FOREACH_PROP_ELEM_SEP(USER_NODE, doled_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};

void di_process_handler(void)
{
    while (1) {
    	for (int i = 0; i < ARRAY_SIZE(doled_gpios); i++) {
	        gpio_pin_toggle_dt(&doled_gpios[i]);
    	}
	    k_msleep(500);
    }
}

K_THREAD_DEFINE(di_process_id, CONFIG_MODBUS_TCP_STACK_SIZE, di_process_handler, NULL, NULL, NULL, 12, 0, 0);

int dio_init(void)
{
    LOG_INF("dio init");
    for (int i = 0; i < ARRAY_SIZE(di_gpios); i++) {
    	/* gpio_pin_configure_dt(&di_gpios[i], GPIO_INPUT); */
    }

    for (int i = 0; i < ARRAY_SIZE(doled_gpios); i++) {
    	gpio_pin_configure_dt(&doled_gpios[i], GPIO_OUTPUT_INACTIVE);
    	gpio_pin_set_dt(&doled_gpios[i], i % 2);
    }
    return 0;
}

SYS_INIT(dio_init, POST_KERNEL, 91);
