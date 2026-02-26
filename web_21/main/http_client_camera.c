#include "http_client_camera.h"
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_client.h"
#include "who_camera.h"
#include "img_converters.h"
#include "sdkconfig.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTP_CAMERA_CLIENT";

// Build server URL from config
static char server_url[256];

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
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
                // If user_data buffer is configured, copy the response into the buffer
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
                // Response is accumulated in output_buffer
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

void http_camera_post_task(void *pvParameters)
{
    // Build the URL from configuration
    snprintf(server_url, sizeof(server_url), "http://%s%s", 
             CONFIG_HTTP_SERVER_ENDPOINT, CONFIG_HTTP_SERVER_PATH);
             
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    
    esp_http_client_config_t config = {
        .url = server_url,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,
        .disable_auto_redirect = true,
        .timeout_ms = 5000,
    };
    
    ESP_LOGI(TAG, "Starting HTTP camera client task");
    ESP_LOGI(TAG, "Target URL: %s", server_url);
    
    while(1) {
        // Get a frame from camera
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }
        
        ESP_LOGI(TAG, "Captured frame: %d bytes, format: %d", fb->len, fb->format);
        
        uint8_t *jpg_buf = NULL;
        size_t jpg_len = 0;
        
        // Convert to JPEG if not already in JPEG format
        if (fb->format == PIXFORMAT_JPEG) {
            jpg_buf = fb->buf;
            jpg_len = fb->len;
        } else {
            // Convert RGB565/other format to JPEG
            bool converted = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
            if (!converted) {
                ESP_LOGE(TAG, "Failed to convert frame to JPEG");
                esp_camera_fb_return(fb);
                vTaskDelay(5000 / portTICK_PERIOD_MS);
                continue;
            }
        }
        
        ESP_LOGI(TAG, "JPEG size: %d bytes", jpg_len);
        
        esp_http_client_handle_t client = esp_http_client_init(&config);
        
        // Set POST method and headers
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "image/jpeg");
        esp_http_client_set_header(client, "X-Device-ID", "esp32-camera");
        
        // Add timestamp header
        char timestamp[64];
        snprintf(timestamp, sizeof(timestamp), "%lld.%06ld", fb->timestamp.tv_sec, fb->timestamp.tv_usec);
        esp_http_client_set_header(client, "X-Timestamp", timestamp);
        
        // Set the JPEG data as POST body
        esp_http_client_set_post_field(client, (const char*)jpg_buf, jpg_len);
        
        // Perform the request
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                    esp_http_client_get_status_code(client),
                    esp_http_client_get_content_length(client));
        } else {
            ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        }
        
        esp_http_client_cleanup(client);
        
        // Free JPEG buffer if we converted the frame
        if (fb->format != PIXFORMAT_JPEG && jpg_buf != NULL) {
            free(jpg_buf);
        }
        
        esp_camera_fb_return(fb);  // Return the frame buffer
        
        // Wait for configured interval before next post
        vTaskDelay((CONFIG_HTTP_CLIENT_UPLOAD_INTERVAL * 1000) / portTICK_PERIOD_MS);
    }
}

void start_http_camera_client(void)
{
    ESP_LOGI(TAG, "Starting HTTP Camera Client");
    xTaskCreate(&http_camera_post_task, "http_camera_post_task", 8192, NULL, 5, NULL);
}

void init_http_camera_client(void)
{
    // Build the URL from configuration
    snprintf(server_url, sizeof(server_url), "http://%s%s", 
             CONFIG_HTTP_SERVER_ENDPOINT, CONFIG_HTTP_SERVER_PATH);
    ESP_LOGI(TAG, "HTTP camera client initialized");
    ESP_LOGI(TAG, "Target URL: %s", server_url);
}

void trigger_camera_capture_and_send(void)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    
    esp_http_client_config_t config = {
        .url = server_url,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,
        .disable_auto_redirect = true,
        .timeout_ms = 5000,
    };

    // Get a frame from camera
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

    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    
    esp_http_client_config_t config = {
        .url = server_url,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,
        .disable_auto_redirect = true,
        .timeout_ms = 5000,
    };
    
    ESP_LOGI(TAG, "Captured frame: %d bytes, format: %d", fb->len, fb->format);
    
    uint8_t *jpg_buf = NULL;
    size_t jpg_len = 0;
    
    // Convert to JPEG if not already in JPEG format
    if (fb->format == PIXFORMAT_JPEG) {
        jpg_buf = fb->buf;
        jpg_len = fb->len;
    } else {
        // Convert RGB565/other format to JPEG
        bool converted = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
        if (!converted) {
            ESP_LOGE(TAG, "Failed to convert frame to JPEG");
            return;
        }
    }
    
    ESP_LOGI(TAG, "JPEG size: %d bytes", jpg_len);
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // Set POST method and headers
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_header(client, "X-Device-ID", "esp32-camera");
    
    // Add timestamp header
    char timestamp[64];
    snprintf(timestamp, sizeof(timestamp), "%lld.%06ld", fb->timestamp.tv_sec, fb->timestamp.tv_usec);
    esp_http_client_set_header(client, "X-Timestamp", timestamp);
    
    // Set the JPEG data as POST body
    esp_http_client_set_post_field(client, (const char*)jpg_buf, jpg_len);
    
    // Perform the request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    
    // Free JPEG buffer if we converted the frame
    if (fb->format != PIXFORMAT_JPEG && jpg_buf != NULL) {
        free(jpg_buf);
    }
}