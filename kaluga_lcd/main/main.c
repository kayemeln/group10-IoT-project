/* LCD + MQTT Example
 *
 * Connects to WiFi, subscribes to an MQTT topic, parses the
 * "match/no_match,True/False,age" payload and displays a fun message.
 *
 * Configure WiFi credentials, broker URI, and topic via:
 *   idf.py menuconfig  ->  Example Configuration
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "lcd.h"
#include "board.h"
#include "fonts.h"

static const char *TAG = "main";

#define IMAGE_WIDTH  320
#define IMAGE_HEIGHT 240

/* ── Font rendering ──────────────────────────────────────────────────────── *
 * Each glyph in fontDataMap is 5 bytes where each BYTE is one visual ROW    *
 * and each BIT in that byte is one visual COLUMN (bit 0 = leftmost col).    *
 * FONT_SCALE=3 → 15×15 px glyphs, ~17 chars per row.                       */
#define FONT_SCALE    4
#define GLYPH_ROWS    5   /* bytes per glyph  = visual rows    */
#define GLYPH_COLS    5   /* bits used / byte = visual columns */
#define FONT_CHAR_W   (GLYPH_COLS * FONT_SCALE)
#define FONT_CHAR_H   (GLYPH_ROWS * FONT_SCALE)
#define FONT_COL_GAP  (2 * FONT_SCALE)
#define FONT_ROW_GAP  (2 * FONT_SCALE)
#define FONT_STEP_X   (FONT_CHAR_W + FONT_COL_GAP)
#define FONT_STEP_Y   (FONT_CHAR_H + FONT_ROW_GAP)

/* ── WiFi ────────────────────────────────────────────────────────────────── */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_MAX_RETRY     5

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

/* ── MQTT message buffer ─────────────────────────────────────────────────── */
#define MSG_BUF_LEN 512
static char              s_mqtt_msg[MSG_BUF_LEN];
static bool              s_new_message = false;
static SemaphoreHandle_t s_msg_mutex;

/* ── LCD frame buffer ────────────────────────────────────────────────────── */
static uint16_t *lcd_buf = NULL;

/* ── Colour helper ───────────────────────────────────────────────────────── */
static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t c = ((uint16_t)r << 11) | ((uint16_t)g << 6) | b;
    return (c << 8) | (c >> 8);
}

/* ── Draw a single scaled glyph ──────────────────────────────────────────── *
 * glyph[gy] = visual row gy; bit gx of that byte = visual column gx.       */
static void draw_char(int x, int y, char ch, uint16_t fg)
{
    const uint8_t *glyph = getCharData(ch);
    if (!glyph) return;

    for (int gy = 0; gy < GLYPH_ROWS; gy++) {      /* byte index  = row    */
        for (int gx = 0; gx < GLYPH_COLS; gx++) {  /* bit index   = column */
            if (!((glyph[gy] >> gx) & 0x01)) continue;
            for (int sy = 0; sy < FONT_SCALE; sy++) {
                for (int sx = 0; sx < FONT_SCALE; sx++) {
                    int px = x + (GLYPH_COLS - 1 - gx) * FONT_SCALE + sx;
                    int py = y + gy * FONT_SCALE + sy;
                    if (px >= 0 && px < IMAGE_WIDTH &&
                        py >= 0 && py < IMAGE_HEIGHT) {
                        lcd_buf[px + IMAGE_WIDTH * py] = fg;
                    }
                }
            }
        }
    }
}

/* ── Render a string to the LCD with word-wrap ───────────────────────────── */
static void display_message(const char *msg)
{
    if (!lcd_buf) return;

    uint16_t bg = color565(0, 0, 0);     /* black background */
    uint16_t fg = color565(31, 63, 31);  /* white text       */

    for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) lcd_buf[i] = bg;

    int x = 6;
    int y = 8;

    for (const char *p = msg; *p; p++) {
        if (y + FONT_CHAR_H > IMAGE_HEIGHT) break;

        if (*p == '\n') {
            x = 6;
            y += FONT_STEP_Y;
            continue;
        }

        if (*p == ' ' && x == 6) continue;  /* skip leading spaces */

        if (x + FONT_STEP_X > IMAGE_WIDTH) {
            x = 6;
            y += FONT_STEP_Y;
            if (y + FONT_CHAR_H > IMAGE_HEIGHT) break;
            if (*p == ' ') continue;
        }

        draw_char(x, y, *p, fg);
        x += FONT_STEP_X;
    }

    lcd_set_index(0, 0, IMAGE_WIDTH - 1, IMAGE_HEIGHT - 1);
    lcd_write_data((uint8_t *)lcd_buf, IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(uint16_t));
}

/* ── Parse MQTT payload and show the appropriate message ─────────────────── *
 * Expected format:  "match,False,23"  or  "no_match,True,17"               */
static void parse_and_display(const char *payload)
{
    char buf[MSG_BUF_LEN];
    strncpy(buf, payload, MSG_BUF_LEN - 1);
    buf[MSG_BUF_LEN - 1] = '\0';

    char *f1 = strtok(buf, ",");
    char *f2 = f1 ? strtok(NULL, ",") : NULL;
    char *f3 = f2 ? strtok(NULL, ",") : NULL;

    if (!f1 || !f2 || !f3) {
        /* Unexpected format – show raw payload */
        display_message(payload);
        return;
    }

    /* Trim leading spaces */
    while (*f1 == ' ') f1++;
    while (*f2 == ' ') f2++;
    while (*f3 == ' ') f3++;

    int is_match = (strcmp(f1, "match") == 0);
    int is_drunk = (strcmp(f2, "True") == 0 || strcmp(f2, "true") == 0);
    int age      = atoi(f3);
    int over_18  = (age > 18);

    const char *text;

    if (is_match && !is_drunk && over_18) {
        text = "Welcome to\nthe Party!";

    } else if (is_match && !is_drunk && !over_18) {
        text = "Sorry, too\nyoung for\nthis party!";

    } else if (is_match && is_drunk && over_18) {
        text = "You are drunk!\nGet a coffee\nand sober up!";

    } else if (is_match && is_drunk && !over_18) {
        text = "Drunk AND\nunderage?!\nGo home NOW!";

    } else if (!is_match && !is_drunk && over_18) {
        text = "No match!\nWere you even\ninvited?!";

    } else if (!is_match && !is_drunk && !over_18) {
        text = "No match AND\ntoo young.\nNot a chance!";

    } else if (!is_match && is_drunk && over_18) {
        text = "No match AND\ndrunk? Bold\nmove. No.";

    } else {
        /* no_match, drunk, under 18 */
        text = "No match,\ndrunk AND\nunderage.\nAbsolutely\nnot!";
    }

    display_message(text);
}

/* ── WiFi event handler ──────────────────────────────────────────────────── */
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "WiFi retry %d/%d", s_retry_num, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }

    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static bool wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t h_any, h_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, &h_any));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, &h_ip));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid     = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}

/* ── MQTT event handler ──────────────────────────────────────────────────── */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t id, void *event_data)
{
    esp_mqtt_event_handle_t  event  = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)id) {

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected, subscribing to '%s'", CONFIG_MQTT_TOPIC);
            esp_mqtt_client_subscribe(client, CONFIG_MQTT_TOPIC, 0);
            display_message("MQTT ready.\nWaiting for\nmessage...");
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            display_message("MQTT\ndisconnected.\nReconnecting...");
            break;

        case MQTT_EVENT_DATA: {
            int len = event->data_len < MSG_BUF_LEN - 1
                      ? event->data_len : MSG_BUF_LEN - 1;
            if (xSemaphoreTake(s_msg_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
                memcpy(s_mqtt_msg, event->data, len);
                s_mqtt_msg[len] = '\0';
                s_new_message = true;
                xSemaphoreGive(s_msg_mutex);
            }
            ESP_LOGI(TAG, "MQTT [%.*s]: %.*s",
                     event->topic_len, event->topic,
                     event->data_len,  event->data);
            break;
        }

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;

        default:
            break;
    }
}

/* ── Task: poll for new MQTT messages and refresh LCD ────────────────────── */
static void mqtt_display_task(void *arg)
{
    char local[MSG_BUF_LEN];
    while (1) {
        if (xSemaphoreTake(s_msg_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (s_new_message) {
                memcpy(local, s_mqtt_msg, MSG_BUF_LEN);
                s_new_message = false;
                xSemaphoreGive(s_msg_mutex);
                parse_and_display(local);
            } else {
                xSemaphoreGive(s_msg_mutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ── Entry point ─────────────────────────────────────────────────────────── */
void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    lcd_config_t lcd_config = {
#ifdef CONFIG_LCD_ST7789
        .clk_fre         = 80 * 1000 * 1000,
#endif
#ifdef CONFIG_LCD_ILI9341
        .clk_fre         = 40 * 1000 * 1000,
#endif
        .pin_clk         = LCD_CLK,
        .pin_mosi        = LCD_MOSI,
        .pin_dc          = LCD_DC,
        .pin_cs          = LCD_CS,
        .pin_rst         = LCD_RST,
        .pin_bk          = LCD_BK,
        .max_buffer_size = 2 * 1024,
        .horizontal      = 2,
        .swap_data       = 1,
    };
    lcd_init(&lcd_config);

    lcd_buf = heap_caps_malloc(IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(uint16_t),
                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!lcd_buf) {
        lcd_buf = malloc(IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(uint16_t));
    }
    if (!lcd_buf) {
        ESP_LOGE(TAG, "Failed to allocate LCD frame buffer");
        return;
    }

    s_msg_mutex = xSemaphoreCreateMutex();

    display_message("Connecting to\nWiFi...");

    if (!wifi_init_sta()) {
        display_message("WiFi failed.\nCheck SSID and\npassword.");
        return;
    }

    display_message("WiFi connected.\nConnecting to\nMQTT broker...");

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_MQTT_BROKER_URI,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    xTaskCreate(mqtt_display_task, "mqtt_disp", 4096, NULL, 5, NULL);
}
