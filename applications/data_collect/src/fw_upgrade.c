#include <zephyr/kernel.h>
#include <zephyr/posix/sys/select.h>
#include <zephyr/net/socket.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#define SLOT1_PARTITION		slot1_partition
#define SLOT1_PARTITION_ID	FIXED_PARTITION_ID(SLOT1_PARTITION)
#define SLOT1_PARTITION_DEV	FIXED_PARTITION_DEVICE(SLOT1_PARTITION)
#define SLOT1_PARTITION_NODE	DT_NODELABEL(SLOT1_PARTITION)
#define TCP_PORT 20001
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fw_boot, LOG_LEVEL_INF);

static uint32_t data_buf[512];
static struct fw_msg {
    uint32_t total_size;
    uint32_t frame_size;
} msg_hdr;

void fw_upgrade(void)
{
    int serv;
	const struct flash_area *fa;
    struct sockaddr_in bind_addr, client_addr;
    socklen_t client_addr_len;
    int rc;

	rc = flash_area_open(SLOT1_PARTITION_ID, &fa);
    if (rc) {
        LOG_ERR("flash area open failed");
        return;
    }

    serv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv < 0) {
        LOG_ERR("error: socket: %d", errno);
        return;
    }

    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(TCP_PORT);

    if (bind(serv, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        LOG_ERR("error: bind: %d", errno);
        return;
    }

    if (listen(serv, 5) < 0) {
        LOG_ERR("error: listen: %d", errno);
        return;
    }

    while (1) {
        int offset = 0;
        int count = 0;
        bool erase = false;
        bool upgrade_finished = false;

        int client = accept(serv, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client < 0) {
            LOG_ERR("accept error");
            k_msleep(300);
            continue;
        }
        do {
            rc = recv(client, &msg_hdr, sizeof(msg_hdr), MSG_WAITALL);
            if (rc < 0) break;

            if (msg_hdr.frame_size > sizeof(data_buf)) {
                LOG_ERR("frame size need <= %d", sizeof(data_buf));
                break;
            }

            rc = recv(client, data_buf, msg_hdr.frame_size, MSG_WAITALL);
            if (rc < 0) break;
            if (!erase) {
                flash_area_erase(fa, 0, fa->fa_size);
                LOG_INF("starting upgrade firmware, size: %d", msg_hdr.total_size);
                erase = true;
            }
            if (flash_area_write(fa, offset, data_buf, msg_hdr.frame_size)) {
                LOG_ERR("flash area write error");
                break;
            }
            offset += msg_hdr.frame_size;
            count ++;
            if (offset == msg_hdr.total_size) {
                LOG_INF("write finished, total size: %d, count: %d", msg_hdr.total_size, count);
                send(client, &count, sizeof(count), 0);
                upgrade_finished = true;
                break;
            }
        } while (rc > 0);

        close(client);
        if (upgrade_finished) {
            extern void set_reboot_status(bool val);
            set_reboot_status(true);
        }

    }
}

K_THREAD_DEFINE(fw_up, CONFIG_FW_UPGRADE_STACK_SIZE, fw_upgrade, NULL, NULL, NULL, CONFIG_FW_UPGRADE_PRIORITY, 0, 0);
