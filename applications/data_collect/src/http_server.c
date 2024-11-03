#include <stdio.h>
#include <inttypes.h>

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include "zephyr/device.h"
#include "zephyr/sys/util.h"
#include <zephyr/drivers/led.h>
#include <zephyr/data/json.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/posix/time.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_http_server_app, LOG_LEVEL_INF);

struct led_command {
	int led_num;
	bool led_state;
};

static const struct json_obj_descr led_command_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct led_command, led_num, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct led_command, led_state, JSON_TOK_TRUE),
};

static const struct device *leds_dev = DEVICE_DT_GET_ANY(gpio_leds);

static const uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static const uint8_t main_js_gz[] = {
#include "main.js.gz.inc"
};

static struct http_resource_detail_static index_html_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/html",
		},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

static struct http_resource_detail_static main_js_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/javascript",
		},
	.static_data = main_js_gz,
	.static_data_len = sizeof(main_js_gz),
};

static int echo_handler(struct http_client_ctx *client, enum http_data_status status,
			uint8_t *buffer, size_t len, struct http_response_ctx *response_ctx,
			void *user_data)
{
#define MAX_TEMP_PRINT_LEN 32
	static char print_str[MAX_TEMP_PRINT_LEN];
	enum http_method method = client->method;
	static size_t processed;

	__ASSERT_NO_MSG(buffer != NULL);

	if (status == HTTP_SERVER_DATA_ABORTED) {
		LOG_DBG("Transaction aborted after %zd bytes.", processed);
		processed = 0;
		return 0;
	}

	processed += len;

	snprintf(print_str, sizeof(print_str), "%s received (%zd bytes)",
		 http_method_str(method), len);
	LOG_HEXDUMP_DBG(buffer, len, print_str);

	if (status == HTTP_SERVER_DATA_FINAL) {
		LOG_DBG("All data received (%zd bytes).", processed);
		processed = 0;
	}

	/* Echo data back to client */
	response_ctx->body = buffer;
	response_ctx->body_len = len;
	response_ctx->final_chunk = (status == HTTP_SERVER_DATA_FINAL);

	return 0;
}

static struct http_resource_detail_dynamic echo_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
		},
	.cb = echo_handler,
	.user_data = NULL,
};

static int uptime_handler(struct http_client_ctx *client, enum http_data_status status,
			  uint8_t *buffer, size_t len, struct http_response_ctx *response_ctx,
			  void *user_data)
{
	int ret;
	static uint8_t uptime_buf[sizeof(STRINGIFY(INT64_MAX))];

	LOG_DBG("Uptime handler status %d", status);

	/* A payload is not expected with the GET request. Ignore any data and wait until
	 * final callback before sending response
	 */
	if (status == HTTP_SERVER_DATA_FINAL) {
		ret = snprintf(uptime_buf, sizeof(uptime_buf), "%" PRId64, time(NULL)*1000);
		if (ret < 0) {
			LOG_ERR("Failed to snprintf uptime, err %d", ret);
			return ret;
		}

		response_ctx->body = uptime_buf;
		response_ctx->body_len = ret;
		response_ctx->final_chunk = true;
	}

	return 0;
}

static struct http_resource_detail_dynamic uptime_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = uptime_handler,
	.user_data = NULL,
};

static void parse_led_post(uint8_t *buf, size_t len)
{
	int ret;
	struct led_command cmd;
	const int expected_return_code = BIT_MASK(ARRAY_SIZE(led_command_descr));

	ret = json_obj_parse(buf, len, led_command_descr, ARRAY_SIZE(led_command_descr), &cmd);
	if (ret != expected_return_code) {
		LOG_WRN("Failed to fully parse JSON payload, ret=%d", ret);
		return;
	}

	LOG_INF("POST request setting LED %d to state %d", cmd.led_num, cmd.led_state);

	if (leds_dev != NULL) {
		if (cmd.led_state) {
			led_on(leds_dev, cmd.led_num);
		} else {
			led_off(leds_dev, cmd.led_num);
		}
	}
}

static int led_handler(struct http_client_ctx *client, enum http_data_status status,
		       uint8_t *buffer, size_t len, struct http_response_ctx *response_ctx,
		       void *user_data)
{
	static uint8_t post_payload_buf[32];
	static size_t cursor;

	LOG_DBG("LED handler status %d, size %zu", status, len);

	if (status == HTTP_SERVER_DATA_ABORTED) {
		cursor = 0;
		return 0;
	}

	if (len + cursor > sizeof(post_payload_buf)) {
		cursor = 0;
		return -ENOMEM;
	}

	/* Copy payload to our buffer. Note that even for a small payload, it may arrive split into
	 * chunks (e.g. if the header size was such that the whole HTTP request exceeds the size of
	 * the client buffer).
	 */
	memcpy(post_payload_buf + cursor, buffer, len);
	cursor += len;

	if (status == HTTP_SERVER_DATA_FINAL) {
		parse_led_post(post_payload_buf, cursor);
		cursor = 0;
	}

	return 0;
}

static struct http_resource_detail_dynamic led_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_POST),
		},
	.cb = led_handler,
	.user_data = NULL,
};

static uint16_t test_http_service_port = 80;
HTTP_SERVICE_DEFINE(test_http_service, NULL, &test_http_service_port, 1,
		    10, NULL);

HTTP_RESOURCE_DEFINE(index_html_gz_resource, test_http_service, "/",
		     &index_html_gz_resource_detail);

HTTP_RESOURCE_DEFINE(main_js_gz_resource, test_http_service, "/main.js",
		     &main_js_gz_resource_detail);

HTTP_RESOURCE_DEFINE(echo_resource, test_http_service, "/dynamic", &echo_resource_detail);

HTTP_RESOURCE_DEFINE(uptime_resource, test_http_service, "/uptime", &uptime_resource_detail);

HTTP_RESOURCE_DEFINE(led_resource, test_http_service, "/led", &led_resource_detail);

int init_http_server(void)
{
    return http_server_start();
}

SYS_INIT(init_http_server, APPLICATION, 15);
