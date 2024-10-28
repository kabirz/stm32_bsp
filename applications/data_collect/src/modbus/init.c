#include <init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2.h>

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
    struct in_addr addr, netmask;
    struct net_if *iface;

    iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
    if (!iface) {
        LOG_ERR("No ethernet interfaces found.");
        return -1;
    }

    addr.s4_addr[0] = holding_regs[HOLDING_IP_ADDR_1_IDX];
    addr.s4_addr[1] = holding_regs[HOLDING_IP_ADDR_2_IDX];
    addr.s4_addr[2] = holding_regs[HOLDING_IP_ADDR_3_IDX];
    addr.s4_addr[3] = holding_regs[HOLDING_IP_ADDR_4_IDX];

    netmask.s_addr = 0xffffff;
    if (net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0) == NULL) {
        LOG_ERR("Cannot add ip address to interface");
        return -1;
    }
    if (net_if_ipv4_set_netmask_by_addr(iface, &addr, &netmask) == false) {
        LOG_ERR("Cannot add netmask to interface");
        return -1;
    }
    return 0;
}

SYS_INIT(modbus_init, APPLICATION, 10);
