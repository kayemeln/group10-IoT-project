/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "driver/gpio.h"
#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mqtt_example";

#define CONTROL_TOPIC "access/control"
#define CONTROL_GPIO  GPIO_NUM_2
#define SPEAKER_GPIO  GPIO_NUM_4

static void speaker_init(void);
static void speaker_buzz(int freq, int duration_ms);
static void play_success_chirp(void);
static void play_success_jingle(void);
static void play_error_buzz(void);

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);

    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, CONTROL_TOPIC, 0);
        ESP_LOGI(TAG, "Subscribed to %s, msg_id=%d", CONTROL_TOPIC, msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA: {
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);

        if (event->topic_len == strlen(CONTROL_TOPIC) &&
            strncmp(event->topic, CONTROL_TOPIC, event->topic_len) == 0) {

            char msg[64];
            int n = event->data_len < (int)sizeof(msg) - 1 ? event->data_len : (int)sizeof(msg) - 1;
            memcpy(msg, event->data, n);
            msg[n] = '\0';

            // Lowercase whole payload
            for (int i = 0; msg[i]; i++) {
                msg[i] = (char)tolower((unsigned char)msg[i]);
            }

            // Trim trailing whitespace/newlines from whole message
            for (int i = (int)strlen(msg) - 1; i >= 0; i--) {
                if (msg[i] == ' ' || msg[i] == '\n' || msg[i] == '\r' || msg[i] == '\t') {
                    msg[i] = '\0';
                } else {
                    break;
                }
            }

            // Expected format: match/no_match,true/false,integer
            char *saveptr = NULL;
            char *result = strtok_r(msg, ",", &saveptr);
            char *mouth_str = strtok_r(NULL, ",", &saveptr);
            char *age_str = strtok_r(NULL, ",", &saveptr);

            if (result == NULL || mouth_str == NULL || age_str == NULL) {
                ESP_LOGW(TAG, "Invalid message format: '%s' (expected match/no_match,true/false,integer)", msg);
                play_error_buzz();
                break;
            }

            // Trim leading spaces
            while (*result == ' ' || *result == '\t') result++;
            while (*mouth_str == ' ' || *mouth_str == '\t') mouth_str++;
            while (*age_str == ' ' || *age_str == '\t') age_str++;

            // Trim trailing spaces/newlines on each token
            for (int i = (int)strlen(result) - 1; i >= 0; i--) {
                if (result[i] == ' ' || result[i] == '\n' || result[i] == '\r' || result[i] == '\t') result[i] = '\0';
                else break;
            }
            for (int i = (int)strlen(mouth_str) - 1; i >= 0; i--) {
                if (mouth_str[i] == ' ' || mouth_str[i] == '\n' || mouth_str[i] == '\r' || mouth_str[i] == '\t') mouth_str[i] = '\0';
                else break;
            }
            for (int i = (int)strlen(age_str) - 1; i >= 0; i--) {
                if (age_str[i] == ' ' || age_str[i] == '\n' || age_str[i] == '\r' || age_str[i] == '\t') age_str[i] = '\0';
                else break;
            }

            bool is_match = (strcmp(result, "match") == 0);
            bool mouth_open = (strcmp(mouth_str, "true") == 0);
            int age = atoi(age_str);

            ESP_LOGI(TAG, "Parsed result='%s', mouth_str='%s', mouth_open=%s, age=%d",
                     result, mouth_str, mouth_open ? "true" : "false", age);

            if (is_match && !mouth_open && age > 18) {
                ESP_LOGI(TAG, "Access granted — match, mouth closed, age over 18");

                play_success_jingle();
                gpio_set_level(CONTROL_GPIO, 0);
                vTaskDelay(pdMS_TO_TICKS(3000));
                gpio_set_level(CONTROL_GPIO, 1);
            } else {
                ESP_LOGI(TAG, "Access denied — conditions not met");
                play_error_buzz();
            }
        }
        break;
    }

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",
                                 event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)",
                     strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };

#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

static void speaker_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = 2000,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t channel_cfg = {
        .gpio_num   = SPEAKER_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));
}

static void speaker_buzz(int freq, int duration_ms)
{
    ESP_ERROR_CHECK(ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

static void play_success_chirp(void)
{
    speaker_buzz(1400, 120);
    vTaskDelay(pdMS_TO_TICKS(50));

    speaker_buzz(2000, 150);
    vTaskDelay(pdMS_TO_TICKS(50));

    speaker_buzz(2800, 250);
}

static void play_success_jingle(void)
{
    speaker_buzz(1500, 120);
    vTaskDelay(pdMS_TO_TICKS(60));

    speaker_buzz(2000, 120);
    vTaskDelay(pdMS_TO_TICKS(60));

    speaker_buzz(2500, 180);
    vTaskDelay(pdMS_TO_TICKS(80));

    speaker_buzz(2800, 300);
}

static void play_error_buzz(void)
{
    speaker_buzz(450, 100);
    vTaskDelay(pdMS_TO_TICKS(50));

    speaker_buzz(350, 150);
    vTaskDelay(pdMS_TO_TICKS(50));

    speaker_buzz(250, 400);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONTROL_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_set_level(CONTROL_GPIO, 1); // start HIGH / locked

    speaker_init();
    mqtt_app_start();
}