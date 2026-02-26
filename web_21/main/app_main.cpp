#include "who_camera.h"
#include "who_human_face_detection.hpp"
#include "app_wifi.h"
#include "app_httpd.hpp"
#include "app_mdns.h"
#include "http_client_camera.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "bsp/esp32_s3_eye.h"

static const char *TAG = "app_main";

static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueHttpFrame = NULL;
static QueueHandle_t xQueueButtonCapture = NULL;

// Function declarations
void capture_and_send_image(void);
esp_err_t init_buttons(void);
void button_capture_task(void *pvParameters);

// Button names for logging (from working button_sample)
static const char* button_names[BSP_BUTTON_NUM] = {
    "MENU",
    "PLAY", 
    "DOWN",
    "UP",
    "BOOT"
};

// Button event handler - triggers HTTP capture
static void button_event_handler(void *button_handle, void *usr_data)
{
    int button_index = (int)usr_data;
    
    if (button_index >= 0 && button_index < BSP_BUTTON_NUM) {
        ESP_LOGI(TAG, "🔘 Button '%s' pressed! Queuing image capture...", 
                 button_names[button_index]);
        
        // Send button event to processing task (non-blocking from interrupt)
        int signal = 1;
        xQueueSendFromISR(xQueueButtonCapture, &signal, NULL);
    } else {
        ESP_LOGW(TAG, "Unknown button index: %d", button_index);
    }
}

void button_capture_task(void *pvParameters)
{
    int signal;
    for (;;) {
        if (xQueueReceive(xQueueButtonCapture, &signal, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Processing button capture request...");
            capture_and_send_image();
        }
    }
}

void capture_and_send_image(void)
{
    ESP_LOGI(TAG, "Processing button press for image capture...");
    
    // Get frame from the same queue used by HTTP server
    camera_fb_t *fb = NULL;
    if (xQueueReceive(xQueueHttpFrame, &fb, pdMS_TO_TICKS(2000))) {
        ESP_LOGI(TAG, "Frame received from queue, sending via HTTP...");
        // Trigger HTTP POST with this frame
        trigger_camera_capture_and_send_frame(fb);
        // Return frame to camera driver
        esp_camera_fb_return(fb);
        ESP_LOGI(TAG, "Frame processed and returned");
    } else {
        ESP_LOGW(TAG, "No frame available in queue, trying direct capture");
        trigger_camera_capture_and_send();
    }
}

// Initialize buttons (from working button_sample)
esp_err_t init_buttons(void)
{
    ESP_LOGI(TAG, "Initializing ESP32-S3-Eye buttons...");
    
    // Create button array
    button_handle_t buttons[BSP_BUTTON_NUM] = {NULL};
    int button_count = 0;
    
    // Create all available buttons
    esp_err_t ret = bsp_iot_button_create(buttons, &button_count, BSP_BUTTON_NUM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create buttons: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Successfully created %d buttons", button_count);
    
    // Register event callbacks for each button
    for (int i = 0; i < button_count; i++) {
        if (buttons[i] != NULL) {
            // Register short press callback for HTTP capture
            ret = iot_button_register_cb(buttons[i], BUTTON_PRESS_DOWN,
                                       button_event_handler, (void*)i);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to register press callback for button %d", i);
                continue;
            }
            
            ESP_LOGI(TAG, "Button %d (%s) initialized successfully", i, button_names[i]);
        }
    }
    
    ESP_LOGI(TAG, "✅ Button initialization complete!");
    ESP_LOGI(TAG, "Available buttons:");
    ESP_LOGI(TAG, "- MENU (ADC): Capture image");
    ESP_LOGI(TAG, "- PLAY (ADC): Capture image");  
    ESP_LOGI(TAG, "- DOWN (ADC): Capture image");
    ESP_LOGI(TAG, "- UP (ADC): Capture image");
    ESP_LOGI(TAG, "- BOOT (GPIO0): Capture image");
    ESP_LOGI(TAG, "📱 Press any button to capture and send images via HTTP POST!");
    
    return ESP_OK;
}

extern "C" void app_main()
{
    ESP_LOGI(TAG, "Starting Human Face Detection Web Application");
    
    // Initialize WiFi connection
    app_wifi_main();
    
    // Create queues for frame processing
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueHttpFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueButtonCapture = xQueueCreate(5, sizeof(int));

    // Register camera (feeding directly to HTTP queue for now)
    register_camera(PIXFORMAT_RGB565, FRAMESIZE_QVGA, 2, xQueueHttpFrame);
    
    // Initialize mDNS
    app_mdns_main();
    
    // Initialize HTTP client (but don't start continuous capture)
    init_http_camera_client();
    
    // Start web server for camera interface
    register_httpd(xQueueHttpFrame, NULL, true);
    
    // Create button capture processing task
    xTaskCreate(button_capture_task, "button_capture", 8192, NULL, 5, NULL);
    
    // Initialize buttons using working button_sample approach
    esp_err_t button_ret = init_buttons();
    if (button_ret != ESP_OK) {
        ESP_LOGE(TAG, "Button initialization failed!");
        // Continue anyway, app can still work via web interface
    }
    
    // Optional: Enable face detection
    // register_human_face_detection(xQueueAIFrame, NULL, NULL, xQueueHttpFrame);
    
    ESP_LOGI(TAG, "Application initialization complete");
    ESP_LOGI(TAG, "Press any ESP32-S3-EYE button to capture and send images via HTTP POST");
}
