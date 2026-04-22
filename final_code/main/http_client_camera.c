#include "http_client_camera.h"
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_client.h"
#include "who_camera.h"
#include "img_converters.h"
#include "sdkconfig.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTPS_CAMERA_CLIENT";

// External reference to embedded certificate
extern const char cert_pem_start[] asm("_binary_cert_pem_start");
extern const char cert_pem_end[] asm("_binary_cert_pem_end");

// Build server URL from config
static char server_url[256];

// -------------------------------------------------------------------
// Async JPEG sender task
// -------------------------------------------------------------------

typedef struct {
    uint8_t *buf;
    size_t   len;
    int64_t  ts_sec;
    int64_t  ts_usec;
} jpg_send_item_t;

static QueueHandle_t s_jpg_queue = NULL;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;
    static int output_len;
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                int copy_len = 0;
                if (evt->user_data) {
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(MAX_HTTP_OUTPUT_BUFFER);
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_ERR_NO_MEM;
                        }
                    }
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        default:
            break;
    }
    return ESP_OK;
}

// Internal helper: POST a pre-encoded JPEG buffer to the server.
static void post_jpg(const uint8_t *jpg_buf, size_t jpg_len, int64_t ts_sec, int64_t ts_usec)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

    esp_http_client_config_t config = {
        .url = server_url,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,
        .disable_auto_redirect = true,
        .timeout_ms = 15000,
        .cert_pem = cert_pem_start,
        .cert_len = cert_pem_end - cert_pem_start,
        .use_global_ca_store = false,
        .skip_cert_common_name_check = false,
    };

    ESP_LOGI(TAG, "JPEG size: %d bytes", jpg_len);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_header(client, "X-Device-ID", "esp32-camera");

    char timestamp[64];
    snprintf(timestamp, sizeof(timestamp), "%lld.%06lld", ts_sec, ts_usec);
    esp_http_client_set_header(client, "X-Timestamp", timestamp);

    esp_http_client_set_post_field(client, (const char*)jpg_buf, jpg_len);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

// Background task: waits for a JPEG item on the queue and sends it.
static void jpg_sender_task(void *arg)
{
    jpg_send_item_t item;
    for (;;) {
        if (xQueueReceive(s_jpg_queue, &item, portMAX_DELAY) == pdTRUE) {
            post_jpg(item.buf, item.len, item.ts_sec, item.ts_usec);
            free(item.buf);
        }
    }
}

void start_jpg_sender_task(void)
{
    s_jpg_queue = xQueueCreate(1, sizeof(jpg_send_item_t));
    xTaskCreate(jpg_sender_task, "jpg_sender", 8192, NULL, 4, NULL);
    ESP_LOGI(TAG, "Async JPEG sender task started");
}

void enqueue_jpg_for_send(uint8_t *jpg_buf, size_t jpg_len, int64_t ts_sec, int64_t ts_usec)
{
    if (s_jpg_queue == NULL) {
        free(jpg_buf);
        return;
    }
    jpg_send_item_t item = {
        .buf    = jpg_buf,
        .len    = jpg_len,
        .ts_sec = ts_sec,
        .ts_usec = ts_usec,
    };
    if (xQueueSend(s_jpg_queue, &item, 0) != pdTRUE) {
        // A send is already queued; drop this one to avoid memory leak
        ESP_LOGW(TAG, "Sender queue full, dropping JPEG");
        free(jpg_buf);
    }
}

// -------------------------------------------------------------------
// Existing API (kept for compatibility)
// -------------------------------------------------------------------

void http_camera_post_task(void *pvParameters)
{
    snprintf(server_url, sizeof(server_url), "https://%s%s",
             CONFIG_HTTP_SERVER_ENDPOINT, CONFIG_HTTP_SERVER_PATH);

    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

    esp_http_client_config_t config = {
        .url = server_url,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,
        .disable_auto_redirect = true,
        .timeout_ms = 15000,
        .cert_pem = cert_pem_start,
        .cert_len = cert_pem_end - cert_pem_start,
        .use_global_ca_store = false,
        .skip_cert_common_name_check = false,
    };

    ESP_LOGI(TAG, "Starting HTTPS camera client task with certificate verification");
    ESP_LOGI(TAG, "Target URL: %s", server_url);

    while(1) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "Captured frame: %d bytes, format: %d", fb->len, fb->format);

        uint8_t *jpg_buf = NULL;
        size_t jpg_len = 0;

        if (fb->format == PIXFORMAT_JPEG) {
            jpg_buf = fb->buf;
            jpg_len = fb->len;
        } else {
            bool converted = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
            if (!converted) {
                ESP_LOGE(TAG, "Failed to convert frame to JPEG");
                esp_camera_fb_return(fb);
                vTaskDelay(5000 / portTICK_PERIOD_MS);
                continue;
            }
        }

        post_jpg(jpg_buf, jpg_len, fb->timestamp.tv_sec, fb->timestamp.tv_usec);

        if (fb->format != PIXFORMAT_JPEG && jpg_buf != NULL) {
            free(jpg_buf);
        }

        esp_camera_fb_return(fb);
        vTaskDelay((CONFIG_HTTP_CLIENT_UPLOAD_INTERVAL * 1000) / portTICK_PERIOD_MS);
    }
}

void start_http_camera_client(void)
{
    ESP_LOGI(TAG, "Starting HTTPS Camera Client");
    xTaskCreate(&http_camera_post_task, "http_camera_post_task", 8192, NULL, 5, NULL);
}

void init_http_camera_client(void)
{
    snprintf(server_url, sizeof(server_url), "https://%s%s",
             CONFIG_HTTP_SERVER_ENDPOINT, CONFIG_HTTP_SERVER_PATH);
    ESP_LOGI(TAG, "HTTPS camera client initialized");
    ESP_LOGI(TAG, "Target URL: %s", server_url);
}

void trigger_camera_capture_and_send(void)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return;
    }
    trigger_camera_capture_and_send_frame(fb);
    esp_camera_fb_return(fb);
}

void trigger_camera_capture_and_send_frame(camera_fb_t *fb)
{
    if (!fb) {
        ESP_LOGE(TAG, "Invalid frame buffer");
        return;
    }

    ESP_LOGI(TAG, "Captured frame: %d bytes, format: %d", fb->len, fb->format);

    uint8_t *jpg_buf = NULL;
    size_t jpg_len = 0;
    bool needs_free = false;

    if (fb->format == PIXFORMAT_JPEG) {
        jpg_buf = fb->buf;
        jpg_len = fb->len;
    } else {
        needs_free = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
        if (!needs_free) {
            ESP_LOGE(TAG, "Failed to convert frame to JPEG");
            return;
        }
    }

    post_jpg(jpg_buf, jpg_len, fb->timestamp.tv_sec, fb->timestamp.tv_usec);

    if (needs_free) {
        free(jpg_buf);
    }
}
