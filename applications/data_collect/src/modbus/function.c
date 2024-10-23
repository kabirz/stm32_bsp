#include <init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(func_modbus, LOG_LEVEL_INF);

static uint16_t holding_reg[CONFIG_MODBUS_HOLDING_REGISTER_NUMBERS];
static uint16_t input_reg[CONFIG_MODBUS_INPUT_REGISTER_NUMBERS];
static uint8_t coils_state[CONFIG_MODBUS_COLS_REGISTER_NUMBERS];


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

	LOG_DBG("Coil read, addr %u, %d", addr, (int)*state);

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

	LOG_DBG("Coil write, addr %u, %d", addr, (int)state);

	return 0;
}

static int holding_reg_rd(uint16_t addr, uint16_t *reg)
{
	if (addr >= ARRAY_SIZE(holding_reg)) {
		return -ENOTSUP;
	}

	*reg = holding_reg[addr];

	LOG_DBG("Holding register read, addr %u", addr);

	return 0;
}

uint16_t get_holding_reg(uint16_t addr)
{
	if (addr >= ARRAY_SIZE(holding_reg)) {
		return 0;
	}
	return holding_reg[addr];
}

int update_holding_reg(uint16_t addr, uint16_t reg)
{
	if (addr >= ARRAY_SIZE(holding_reg)) {
		return -ENOTSUP;
	}

	holding_reg[addr] = reg;

	return 0;
}

static int holding_reg_wr(uint16_t addr, uint16_t reg)
{
	if (addr >= ARRAY_SIZE(holding_reg)) {
		return -ENOTSUP;
	}
	switch (addr) {
	case HOLDING_DO_IDX:
		reg = reg & 0xff;
		mb_set_do(reg);
		break;
    case HOLDING_DI_EN_IDX:
    case HOLDING_AI_EN_IDX:
    case HOLDING_DI_SI_IDX:
    case HOLDING_AI_SI_IDX:
    case HOLDING_CAN_BPS_IDX:
    case HOLDING_RS485_BPS_IDX:
    case HOLDING_SLAVE_ID_IDX:

		break;
	case HOLDING_HIS_SAVE_IDX:
		/* TODO */
		break;
    case HOLDING_CAN_ID_IDX:
		break;
	case HOLDING_IP_ADDR_1_IDX:
    case HOLDING_IP_ADDR_2_IDX:
    case HOLDING_IP_ADDR_3_IDX:
    case HOLDING_IP_ADDR_4_IDX:
		break;
    case HOLDING_TIMESTAMPH_IDX:
    case HOLDING_TIMESTAMPL_IDX:
		break;
    case HOLDING_CFG_SAVE_IDX:
		break;
    case HOLDING_REBOOT_IDX:
		sys_reboot(SYS_REBOOT_COLD);
		break;
	default:
		break;
	}
	
	holding_reg[addr] = reg;

	LOG_DBG("Holding register write, addr %u", addr);

	return 0;
}

int update_input_reg(uint16_t addr, uint16_t reg)
{
	if (addr >= ARRAY_SIZE(input_reg)) {
		return -ENOTSUP;
	}
	input_reg[addr] = reg;

	LOG_DBG("set input register addr %u, value: %u", addr, reg);

	return 0;
}

static int input_reg_rd(uint16_t addr, uint16_t *reg)
{
	if (addr >= ARRAY_SIZE(input_reg)) {
		return -ENOTSUP;
	}

	*reg = input_reg[addr];

	LOG_DBG("Input register read, addr %u", addr);

	return 0;
}

struct modbus_user_callbacks mbs_cbs = {
	.coil_rd = coil_rd,
	.coil_wr = coil_wr,
	.holding_reg_rd = holding_reg_rd,
	.holding_reg_wr = holding_reg_wr,
	.input_reg_rd = input_reg_rd,
};

