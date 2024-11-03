#include <init.h>
#include <zephyr/net/socket.h>
#include <zephyr//net/net_if.h>
#include <cJSON.h>
#ifdef CONFIG_SETTINGS
#include <zephyr/settings/settings.h>
#endif

#define MULTICAST_GROUP "224.0.0.1"
#define MULTICAST_PORT  9002
#define UDP_BUF_SIZE 256
static uint8_t recv_buffer[UDP_BUF_SIZE];
static uint8_t send_buffer[UDP_BUF_SIZE];

static int parse_udp_msg(char *msg)
{
    cJSON *recv_root, *send_root, *ch1, *ch2;
    char *str;
    int len = 0;

    send_root = cJSON_CreateObject();
    recv_root = cJSON_Parse(msg);
    if (recv_root == NULL) {
    	LOG_ERR("json parse failed");
    	cJSON_AddStringToObject(send_root, "errorMsg", "parse json error");
    	goto out;
    }

    ch1 = recv_root->child;
    while (ch1) {
    	if (strcmp(ch1->string, "get_device_info") == 0 && cJSON_IsTrue(ch1)) {
    	    cJSON *dev_child = cJSON_AddObjectToObject(send_root, "device_info");
    	    uint32_t timestamp = (uint32_t)time(NULL);
            char tmp_buf[20];

            snprintf(tmp_buf, 20, "%d.%d.%d.%d",
                     (uint8_t)get_holding_reg(HOLDING_IP_ADDR_1_IDX),
                     (uint8_t)get_holding_reg(HOLDING_IP_ADDR_2_IDX),
                     (uint8_t)get_holding_reg(HOLDING_IP_ADDR_3_IDX),
                     (uint8_t)get_holding_reg(HOLDING_IP_ADDR_4_IDX));
            cJSON_AddStringToObject(dev_child, "ip", tmp_buf);
            cJSON_AddNumberToObject(dev_child, "slave_id", get_holding_reg(HOLDING_SLAVE_ID_IDX));
            cJSON_AddNumberToObject(dev_child, "rs485_bps", get_holding_reg(HOLDING_RS485_BPS_IDX));
            cJSON_AddNumberToObject(dev_child, "timestamp", timestamp);
    	} else if (strcmp(ch1->string, "set_device_info") == 0 && cJSON_IsObject(ch1)) {
    	    /* set device infomation, ip address, timestamp, modbus slave id */
    	    cJSON *dev_child = cJSON_AddObjectToObject(send_root, "device_info");
    	    bool reg_changed = false;

    	    ch2 = ch1->child;
    	    while (ch2) {
    	    	if (strcmp(ch2->string, "ip") == 0 && cJSON_IsString(ch2)) {
    	    	    char tmp_buf[20];
    	    	    uint16_t ip_addr[4];
    	    	    if (sscanf(ch2->valuestring, "%hd.%hd.%hd.%hd", ip_addr + 0, ip_addr + 1, ip_addr + 2, ip_addr + 3) != 4) {
    	    	    	cJSON_AddStringToObject(send_root, "errorMsg", "invalid ip address");
    	    	    } else {
    	    	    	update_holding_reg(HOLDING_IP_ADDR_1_IDX, ip_addr[0]);
    	    	    	update_holding_reg(HOLDING_IP_ADDR_2_IDX, ip_addr[1]);
    	    	    	update_holding_reg(HOLDING_IP_ADDR_3_IDX, ip_addr[2]);
    	    	    	update_holding_reg(HOLDING_IP_ADDR_4_IDX, ip_addr[3]);
    	    	    	reg_changed = true;

    	    	    	snprintf(tmp_buf, 20, "%d.%d.%d.%d",
    	    	    	      (uint8_t)get_holding_reg(HOLDING_IP_ADDR_1_IDX),
    	    	    	      (uint8_t)get_holding_reg(HOLDING_IP_ADDR_2_IDX),
    	    	    	      (uint8_t)get_holding_reg(HOLDING_IP_ADDR_3_IDX),
    	    	    	      (uint8_t)get_holding_reg(HOLDING_IP_ADDR_4_IDX));
    	    	    	cJSON_AddStringToObject(dev_child, "ip", tmp_buf);
    	    	    }
    	    	} else if (strcmp(ch2->string, "slave_id") == 0 && cJSON_IsNumber(ch2)) {
    	    	    update_holding_reg(HOLDING_SLAVE_ID_IDX, ch2->valueint);
    	    	    cJSON_AddNumberToObject(dev_child, "slave_id", get_holding_reg(HOLDING_SLAVE_ID_IDX));
    	    	    reg_changed = true;
    	    	} else if (strcmp(ch2->string, "rs485_bps") == 0 && cJSON_IsNumber(ch2)) {
    	    	    update_holding_reg(HOLDING_RS485_BPS_IDX, ch2->valueint);
    	    	    cJSON_AddNumberToObject(dev_child, "rs485_bps", get_holding_reg(HOLDING_RS485_BPS_IDX));
    	    	    reg_changed = true;
    	    	} else if (strcmp(ch2->string, "timestamp") == 0 && cJSON_IsNumber(ch2)) {
                    extern void set_timestamp(time_t);
    	    	    set_timestamp((time_t)ch2->valueint);
    	    	    cJSON_AddNumberToObject(dev_child, "timestamp", (uint32_t)time(NULL));
    	    	}
    	    	ch2 = ch2->next;
    	    }
    	    if (reg_changed) {
#ifdef CONFIG_SETTINGS
		settings_save();
#endif
	    }
	}
    	ch1 = ch1->next;
    }

    cJSON_Delete(recv_root);

out:
    str = cJSON_Print(send_root);
    len = strlen(str) + 1;
    strncpy(send_buffer, str, len);
    cJSON_free(str);
    cJSON_Delete(send_root);

    return len-1;
}

static void udp_poll(void)
{
    int serv;
    struct sockaddr_in bind_addr, client_addr;
    socklen_t client_addr_len;
    int buf_len;

    while (!net_if_is_admin_up(net_if_get_default())) {
    	k_msleep(200);
    }
    serv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serv < 0) {
    	LOG_ERR("udp socket create error");
    	return;
    }

    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(MULTICAST_PORT);
    if (bind(serv, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
    	LOG_ERR("error: bind: %d", errno);
    	return;
    }

    while (1) {
    	client_addr_len = sizeof(client_addr);
    	buf_len = recvfrom(serv, recv_buffer, UDP_BUF_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
    	if (buf_len > 0) {
    	    recv_buffer[buf_len] = '\0';
    	    LOG_DBG("udp data: %s", recv_buffer);
    	} else {
    	    LOG_ERR("udp error!");
    	    continue;
    	}
    	buf_len = parse_udp_msg(recv_buffer);
    	net_addr_pton(AF_INET, "224.0.0.1", &client_addr.sin_addr);
        if (sendto(serv, send_buffer, buf_len, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr)) == -1) {
            LOG_ERR("udp send message error: %d!", errno);
        }

        k_msleep(10);
    }
}

K_THREAD_DEFINE(udp_bcast, CONFIG_UDP_STACK_SIZE, udp_poll, NULL, NULL, NULL, CONFIG_UDP_PRIORITY, 0, 0);
