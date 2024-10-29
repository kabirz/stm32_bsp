#include <init.h>
#include <zephyr/net/socket.h>
#include <zephyr/data/json.h>

#define UDP_BUF_SIZE 256
static uint8_t recv_buffer[UDP_BUF_SIZE];
static uint8_t send_buffer[UDP_BUF_SIZE];

struct udp_data_json {
    const char *ip;
    uint16_t slave_id;
    uint32_t timestamp;
};

static int parse_udp_msg(char *msg)
{
    
}

static void udp_poll(void)
{
    int serv;
    struct sockaddr_in bind_addr;
    struct sockaddr client_addr;
    socklen_t client_addr_len;
    int buf_len;


    serv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serv < 0) {
    	LOG_ERR("udp socket create error");
    	return;
    }

    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(9001);
    if (bind(serv, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
	LOG_ERR("error: bind: %d", errno);
	return;
    }
    while (1) {
    	client_addr_len = sizeof(client_addr);
    	buf_len = recvfrom(serv, recv_buffer, UDP_BUF_SIZE, 0, &client_addr, &client_addr_len);
    	if (buf_len > 0) {
    	    recv_buffer[buf_len] = '\0';
    	    LOG_DBG("udp data: %s", recv_buffer);
    	} else {
    	    LOG_ERR("udp error!");
    	}
    }
}

K_THREAD_DEFINE(udp_id, 1024, udp_poll, NULL, NULL, NULL, 20, 0, 0);
