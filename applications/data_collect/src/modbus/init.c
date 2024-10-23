#include <init.h>
#include <zephyr/kernel.h>

static const uint16_t holding_regs[CONFIG_MODBUS_HOLDING_REGISTER_NUMBERS] = {
    [HOLDING_DI_EN_IDX] = 0xffff,
    [HOLDING_AI_EN_IDX] = 0xf,
    [HOLDING_DI_SI_IDX] = 200,
    [HOLDING_AI_SI_IDX] = 200,
    [HOLDING_HIS_SAVE_IDX] = 0,
    [HOLDING_CAN_ID_IDX] = 0x111,
    [HOLDING_CAN_BPS_IDX] = 10,
    [HOLDING_RS485_BPS_IDX] = 9600,
    [HOLDING_SLAVE_ID_IDX] = 0x1,
    [HOLDING_IP_ADDR_1_IDX] = 192,
    [HOLDING_IP_ADDR_2_IDX] = 168,
    [HOLDING_IP_ADDR_3_IDX] = 12,
    [HOLDING_IP_ADDR_4_IDX] = 101,

};

int modbus_init(void)
{
    update_input_reg(INPUT_VER_IDX, 0x204);
    for (size_t i = 0; i < ARRAY_SIZE(holding_regs); i++) {
        if (holding_regs[i])
            update_holding_reg(i, holding_regs[i]);
    }
    return 0;
}

SYS_INIT(modbus_init, POST_KERNEL, 90);
