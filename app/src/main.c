/*
 * Copyright (c) 2022, thedjnK
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <device.h>
#include <sys/ring_buffer.h>
#include <drivers/uart.h>
#include <drivers/led_strip.h>
#include <usb/usb_device.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(adalight_rgb, CONFIG_ADALIGHT_RGB_LOG_LEVEL);

/* USB CDC device setup */
#define USB_CDC_NODE		DT_ALIAS(usb_cdc)

/* LED strip device setup */
#define LED_STRIP_NODE		DT_ALIAS(led_strip)
#define LED_STRIP_PIXELS	DT_PROP(LED_STRIP_NODE, chain_length)

/* 3 bytes per LED, 6 bytes header, allow enough for 6 messages in ring buffer and 2 messages for processing */
#define HEADER_SIZE 6
#define BUFFER_SIZE ((3 * LED_STRIP_PIXELS) + HEADER_SIZE)
#define RING_BUF_SIZE (BUFFER_SIZE * 6)
#define DATA_BUF_SIZE (BUFFER_SIZE * 2)

#if !DT_NODE_HAS_STATUS(USB_CDC_NODE, okay)
#error "Missing usb_cdc alias"
#endif

#if !DT_NODE_HAS_STATUS(LED_STRIP_NODE, okay)
#error "Missing led_strip alias"
#endif

static uint8_t ring_buffer[RING_BUF_SIZE];
static struct ring_buf ringbuf;
static struct led_rgb pixels[LED_STRIP_PIXELS];
static const struct device *led_strip = DEVICE_DT_GET(LED_STRIP_NODE);

static void interrupt_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			int recv_len, rb_len;
			uint8_t buffer[DATA_BUF_SIZE];
			size_t len = MIN(ring_buf_space_get(&ringbuf), sizeof(buffer));

			recv_len = uart_fifo_read(dev, buffer, len);

			rb_len = ring_buf_put(&ringbuf, buffer, recv_len);

			if (rb_len < recv_len) {
				LOG_ERR("Drop %u bytes", recv_len - rb_len);
			}

			if (rb_len >= 1 && ring_buf_size_get(&ringbuf) > HEADER_SIZE) {
				uint8_t i = 0;
				struct led_rgb colour;

				/* Check received data and search for 'Ada' header */
				rb_len = ring_buf_peek(&ringbuf, buffer, sizeof(buffer));

				while (i < rb_len && (buffer[i] != 'A' || buffer[i+1] != 'd' || buffer[i+2] != 'a')) {
					++i;
				}

				if ((rb_len - i) <= HEADER_SIZE) {
					/* Insufficient data for a single message, skip */
					continue;
				}

				if (buffer[i] == 'A' && buffer[i+1] == 'd' && buffer[i+2] == 'a' && buffer[i+5] == (buffer[i+3] ^ buffer[i+4] ^ 0x55)) {
					/* Received valid header, check for number of LEDs */
					uint16_t led_count = ((((uint16_t)buffer[i+3]) << 8) | (uint16_t)buffer[i+4]) + 1;

					if (led_count > LED_STRIP_PIXELS) {
						led_count = LED_STRIP_PIXELS;
					}

					rb_len = ring_buf_get(&ringbuf, buffer, sizeof(buffer));

					/* We have the required data */
					memset(&pixels, 0x00, (sizeof(pixels) / sizeof(pixels[0]) * led_count));

					/* Move to start of data */
					i += HEADER_SIZE;
					uint8_t pos = 0;

					while (pos < led_count) {
						colour.r = buffer[i];
						colour.g = buffer[i+1];
						colour.b = buffer[i+2];
						memcpy(&pixels[pos], &colour, sizeof(struct led_rgb));
						i += 3;
						++pos;
					}

					int rc = led_strip_update_rgb(led_strip, pixels, led_count);

					if (rc != 0) {
						LOG_ERR("Update failed: %d", rc);
					}
				} else {
					if (buffer[i] == 'A' && buffer[i+1] == 'd' && buffer[i+2] == 'a' && buffer[i+5] != (buffer[i+3] ^ buffer[i+4] ^ 0x55)) {
						LOG_ERR("Invalid checksum: %02x, expected: %02x", buffer[i+5], (buffer[i+3] ^ buffer[i+4] ^ 0x55));
					} else {
						LOG_ERR("Invalid data: %02x%02x%02x", buffer[i], buffer[i+1], buffer[i+2]);
					}
				}
			}
		}
	}
}

void main(void)
{
	int ret;
	const struct device *usb_cdc = DEVICE_DT_GET(USB_CDC_NODE);

	if (!device_is_ready(usb_cdc)) {
		LOG_ERR("CDC ACM device is not ready");
		return;
	}

	ret = usb_enable(NULL);

	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	ring_buf_init(&ringbuf, sizeof(ring_buffer), ring_buffer);

	/* Delay a short time for USB setup to complete */
	k_busy_wait(500000);

	uart_irq_callback_set(usb_cdc, interrupt_handler);
	uart_irq_rx_enable(usb_cdc);
}
