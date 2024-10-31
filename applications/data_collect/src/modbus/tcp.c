/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <zephyr/sys/util.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/kernel.h>

#include <zephyr/net/socket.h>

#define MODBUS_TCP_PORT 502
static uint8_t data_buf[256];

static struct modbus_adu tmp_adu;
K_SEM_DEFINE(received, 0, 1);
static int server_iface;

static int server_raw_cb(const int iface, const struct modbus_adu *adu, void *user_data)
{
    LOG_DBG("Server raw callback from interface %d", iface);

    tmp_adu.trans_id = adu->trans_id;
    tmp_adu.proto_id = adu->proto_id;
    tmp_adu.length = adu->length;
    tmp_adu.unit_id = adu->unit_id;
    tmp_adu.fc = adu->fc;
    memcpy(tmp_adu.data, adu->data,
	   MIN(adu->length, CONFIG_MODBUS_BUFFER_SIZE));

    LOG_HEXDUMP_DBG(tmp_adu.data, tmp_adu.length, "resp");
    k_sem_give(&received);

    return 0;
}

static struct modbus_iface_param server_param = {
    .mode = MODBUS_MODE_RAW,
    .server = {
	.user_cb = &mbs_cbs,
	.unit_id = 1,
    },
    .rawcb.raw_tx_cb = server_raw_cb,
    .rawcb.user_data = NULL
};

static int init_modbus_server(void)
{
    char iface_name[] = "RAW_0";
    int err;

    server_iface = modbus_iface_get_by_name(iface_name);

    if (server_iface < 0) {
	LOG_ERR("Failed to get iface index for %s",
	 iface_name);
	return -ENODEV;
    }
    server_param.server.unit_id = get_holding_reg(HOLDING_SLAVE_ID_IDX);
    err = modbus_init_server(server_iface, server_param);

    if (err < 0) {
	return err;
    }
    return err;
}

static int modbus_tcp_reply(int client, struct modbus_adu *adu)
{
    modbus_raw_put_header(adu, data_buf);
    memcpy(data_buf + MODBUS_MBAP_AND_FC_LENGTH, adu->data, adu->length);

    if (send(client, data_buf, MODBUS_MBAP_AND_FC_LENGTH + adu->length, 0) < 0) {
	return -errno;
    }

    return 0;
}

static int modbus_tcp_connection(int client)
{
    uint8_t header[MODBUS_MBAP_AND_FC_LENGTH];
    int rc;
    int data_len;

    rc = recv(client, header, sizeof(header), MSG_WAITALL);
    if (rc <= 0) {
	return rc == 0 ? -ENOTCONN : -errno;
    }

    LOG_HEXDUMP_DBG(header, sizeof(header), "h:>");
    modbus_raw_get_header(&tmp_adu, header);
    data_len = tmp_adu.length;

    rc = recv(client, tmp_adu.data, data_len, MSG_WAITALL);
    if (rc <= 0) {
	return rc == 0 ? -ENOTCONN : -errno;
    }

    LOG_HEXDUMP_DBG(tmp_adu.data, tmp_adu.length, "d:>");
    if (modbus_raw_submit_rx(server_iface, &tmp_adu)) {
	LOG_ERR("Failed to submit raw ADU");
	return -EIO;
    }

    if (k_sem_take(&received, K_MSEC(1000)) != 0) {
	LOG_ERR("MODBUS RAW wait time expired");
	modbus_raw_set_server_failure(&tmp_adu);
    }

    return modbus_tcp_reply(client, &tmp_adu);
}

void tcp_poll(void)
{
    int serv;
    struct sockaddr_in bind_addr;
    static int counter;

    if (init_modbus_server()) {
	LOG_ERR("Modbus TCP server initialization failed");
	return;
    }


    serv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serv < 0) {
	LOG_ERR("error: socket: %d", errno);
	return;
    }

    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(MODBUS_TCP_PORT);

    if (bind(serv, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
	LOG_ERR("error: bind: %d", errno);
	return;
    }

    if (listen(serv, 5) < 0) {
	LOG_ERR("error: listen: %d", errno);
	return;
    }

    LOG_INF("Started MODBUS TCP server example on port %d", MODBUS_TCP_PORT);

    while (1) {
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	char addr_str[INET_ADDRSTRLEN];
	int client;
	int rc;

	client = accept(serv, (struct sockaddr *)&client_addr,
		 &client_addr_len);

	if (client < 0) {
	    LOG_ERR("error: accept: %d", errno);
	    continue;
	}

	inet_ntop(client_addr.sin_family, &client_addr.sin_addr,
	   addr_str, sizeof(addr_str));
	LOG_INF("Connection #%d from %s", counter++, addr_str);

	do {
	    rc = modbus_tcp_connection(client);
	} while (!rc);

	close(client);
	LOG_INF("Connection from %s closed, errno %d", addr_str, rc);
    }
}

K_THREAD_DEFINE(tcp_id, CONFIG_MODBUS_TCP_STACK_SIZE, tcp_poll, NULL, NULL, NULL, CONFIG_MODBUS_TCP_PRIORITY, 0, 0);

