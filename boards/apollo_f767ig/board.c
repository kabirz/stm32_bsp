/*
 * Copyright (c) 2024 kabirz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define USER_NODE DT_PATH(zephyr_user)

int phy_init(void)
{
    struct gpio_dt_spec phy_reset = GPIO_DT_SPEC_GET(USER_NODE, phy_reset_gpios);
    struct gpio_dt_spec beep = GPIO_DT_SPEC_GET(USER_NODE, beep_gpios);

    /* close beep */
    gpio_pin_configure_dt(&beep, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set_dt(&beep, 1);

    /* phy enable */
    gpio_pin_configure_dt(&phy_reset, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set_dt(&phy_reset, 1);
    k_msleep(100);
    gpio_pin_set_dt(&phy_reset, 0);
    k_msleep(100);
    return 0;
}

SYS_INIT(phy_init, POST_KERNEL, 71); /* 71 after i2c pcf, before ethernet driver */
