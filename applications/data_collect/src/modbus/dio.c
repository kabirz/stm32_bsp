#include <init.h>
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

int mb_set_do(uint16_t reg)
{
    bool status;

    for (size_t i = 0; i < ARRAY_SIZE(do_gpios); i++) {
        status = !!(reg & BIT(i));
        gpio_pin_set_dt(&do_gpios[i], status);
        if (i < ARRAY_SIZE(doled_gpios))
            gpio_pin_set_dt(&doled_gpios[i], status);
    }

    return 0;
}

void di_process_handler(void)
{
    uint16_t val;

    while (1) {
        val = 0;
        for (uint8_t i = 0; i < ARRAY_SIZE(di_gpios); i++) {
            if ((get_holding_reg(HOLDING_DI_EN_IDX) & BIT(i)) && gpio_pin_get_dt(&di_gpios[i]))
                val |= BIT(i);
        }
        update_input_reg(INPUT_DI_IDX, val);
	    k_msleep(get_holding_reg(HOLDING_DI_SI_IDX));
    }
}

K_THREAD_DEFINE(di_process_id, CONFIG_MODBUS_TCP_STACK_SIZE, di_process_handler, NULL, NULL, NULL, 12, 0, 0);

int dio_init(void)
{
    LOG_INF("dio init");

    for (int i = 0; i < ARRAY_SIZE(di_gpios); i++) {
    	gpio_pin_configure_dt(&di_gpios[i], GPIO_INPUT|GPIO_PULL_DOWN);
    }

    for (int i = 0; i < ARRAY_SIZE(doled_gpios); i++) {
    	gpio_pin_configure_dt(&doled_gpios[i], GPIO_OUTPUT_INACTIVE);
    	gpio_pin_set_dt(&doled_gpios[i], 0);
    }
    return 0;
}

SYS_INIT(dio_init, APPLICATION, 12);
