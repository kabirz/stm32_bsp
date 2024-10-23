#include <init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/modbus/modbus.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(func_modbus, LOG_LEVEL_INF);

static uint16_t holding_reg[CONFIG_MODBUS_HOLDING_REGISTER_NUMBERS];
static uint16_t input_reg[CONFIG_MODBUS_INPUT_REGISTER_NUMBERS];
static uint8_t coils_state[CONFIG_MODBUS_COLS_REGISTER_NUMBERS];

static const struct gpio_dt_spec led_dev[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
};

int init_leds(void)
{
	int err;

	for (int i = 0; i < ARRAY_SIZE(led_dev); i++) {
		if (!gpio_is_ready_dt(&led_dev[i])) {
			LOG_ERR("LED%u GPIO device not ready", i);
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&led_dev[i], GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("Failed to configure LED%u pin", i);
			return err;
		}
	}

	return 0;
}

static int coil_rd(uint16_t addr, bool *state)
{
	if (addr >= ARRAY_SIZE(coils_state)) {
		return -ENOTSUP;
	}

	if (coils_state[addr/8] & BIT(addr%8)) {
		*state = true;
	} else {
		*state = false;
	}

	LOG_INF("Coil read, addr %u, %d", addr, (int)*state);

	return 0;
}

static int coil_wr(uint16_t addr, bool state)
{
	bool on;

	if (addr >= ARRAY_SIZE(coils_state)) {
		return -ENOTSUP;
	}

	if (state == true) {
		coils_state[addr/8] |= BIT(addr%8);
		on = true;
	} else {
		coils_state[addr/8] &= ~BIT(addr%8);
		on = false;
	}

	gpio_pin_set(led_dev[addr].port, led_dev[addr].pin, (int)on);

	LOG_INF("Coil write, addr %u, %d", addr, (int)state);

	return 0;
}

static int holding_reg_rd(uint16_t addr, uint16_t *reg)
{
	if (addr >= ARRAY_SIZE(holding_reg)) {
		return -ENOTSUP;
	}

	*reg = holding_reg[addr];

	LOG_INF("Holding register read, addr %u", addr);

	return 0;
}

static int holding_reg_wr(uint16_t addr, uint16_t reg)
{
	if (addr >= ARRAY_SIZE(holding_reg)) {
		return -ENOTSUP;
	}

	holding_reg[addr] = reg;

	LOG_INF("Holding register write, addr %u", addr);

	return 0;
}

static int input_reg_rd(uint16_t addr, uint16_t *reg)
{
	if (addr >= ARRAY_SIZE(input_reg)) {
		return -ENOTSUP;
	}

	*reg = input_reg[addr];

	LOG_INF("Input register read, addr %u", addr);

	return 0;
}

struct modbus_user_callbacks mbs_cbs = {
	.coil_rd = coil_rd,
	.coil_wr = coil_wr,
	.holding_reg_rd = holding_reg_rd,
	.holding_reg_wr = holding_reg_wr,
	.input_reg_rd = input_reg_rd,
};

