#include "who_camera.h"
#include "who_human_face_detection.hpp"
#include "app_wifi.h"
#include "app_httpd.hpp"
#include "app_mdns.h"
#include "http_client_camera.h"
#include "esp_log.h"

static const char *TAG = "app_main";

static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueHttpFrame = NULL;

extern "C" void app_main()
{
    ESP_LOGI(TAG, "Starting Human Face Detection Web Application");
    
    // Initialize WiFi connection
    app_wifi_main();

    // Create queues for frame processing
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueHttpFrame = xQueueCreate(2, sizeof(camera_fb_t *));

    // Register camera (feeding directly to HTTP queue for now)
    register_camera(PIXFORMAT_RGB565, FRAMESIZE_QVGA, 2, xQueueHttpFrame);
    
    // Initialize mDNS
    app_mdns_main();
    
    // Optional: Enable face detection
    // register_human_face_detection(xQueueAIFrame, NULL, NULL, xQueueHttpFrame);
    
    // Start web server for camera interface
    register_httpd(xQueueHttpFrame, NULL, true);
    
    // Start HTTP client to upload images to external server
    start_http_camera_client();
    
    ESP_LOGI(TAG, "Application initialization complete");
}
