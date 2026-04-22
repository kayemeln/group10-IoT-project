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
#include "bsp/display.h"
#include "esp_lcd_panel_ops.h"
#include "esp_timer.h"
#include "img_converters.h"

static const char *TAG = "app_main";

static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueHttpFrame = NULL;
static QueueHandle_t xQueueButtonCapture = NULL;

// LCD panel handles for display
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static bool lcd_initialized = false;
static esp_timer_handle_t s_backlight_timer = NULL;

// Function declarations
void capture_and_send_image(void);
void display_image_on_lcd(camera_fb_t *fb);
esp_err_t init_lcd_display(void);
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

static void backlight_timer_callback(void *arg)
{
    bsp_display_backlight_off();
    ESP_LOGI(TAG, "LCD backlight off (power-save)");
}

static void restart_backlight_timer(void)
{
    if (s_backlight_timer == NULL) return;
    esp_timer_stop(s_backlight_timer); // returns ESP_ERR_INVALID_STATE if already stopped — harmless
    esp_timer_start_once(s_backlight_timer, 10ULL * 1000 * 1000); // 10 s in µs
}

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

    // Drain stale frames so the next receive gets a post-press frame
    camera_fb_t *stale = NULL;
    int drained = 0;
    while (xQueueReceive(xQueueHttpFrame, &stale, 0) == pdTRUE) {
        esp_camera_fb_return(stale);
        stale = NULL;
        drained++;
    }
    if (drained > 0) ESP_LOGI(TAG, "Drained %d stale frame(s) from queue", drained);

    // Wait for a fresh frame
    camera_fb_t *fb = NULL;
    if (xQueueReceive(xQueueHttpFrame, &fb, pdMS_TO_TICKS(2000))) {
        ESP_LOGI(TAG, "Fresh frame received");

        // 1. Update LCD immediately
        if (lcd_initialized) {
            bsp_display_backlight_off();
            display_image_on_lcd(fb);
            bsp_display_backlight_on();
            restart_backlight_timer();
        }

        // 2. Encode to JPEG while we still hold the frame buffer
        uint8_t *jpg_buf = NULL;
        size_t   jpg_len = 0;
        int64_t  ts_sec  = fb->timestamp.tv_sec;
        int64_t  ts_usec = fb->timestamp.tv_usec;

        if (frame2jpg(fb, 80, &jpg_buf, &jpg_len)) {
            // 3. Return camera buffer NOW — camera can capture next frame immediately
            esp_camera_fb_return(fb);
            ESP_LOGI(TAG, "Frame returned, queuing JPEG send (%d bytes)", jpg_len);
            // 4. Hand JPEG off to background sender — this call returns immediately
            enqueue_jpg_for_send(jpg_buf, jpg_len, ts_sec, ts_usec);
        } else {
            ESP_LOGE(TAG, "JPEG encode failed");
            esp_camera_fb_return(fb);
        }
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
    ESP_LOGI(TAG, "📱 Press any button to capture and send images via HTTPS POST (insecure mode)!");
    
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
    register_camera(PIXFORMAT_RGB565, FRAMESIZE_240X240, 2, xQueueHttpFrame);
    
    // Initialize mDNS
    app_mdns_main();
    
    // Initialize LCD display
    esp_err_t lcd_ret = init_lcd_display();
    if (lcd_ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ LCD display initialized successfully!");
    } else {
        ESP_LOGE(TAG, "❌ LCD display initialization failed: %s", esp_err_to_name(lcd_ret));
    }
    
    // Initialize HTTP client and start background JPEG sender task
    init_http_camera_client();
    start_jpg_sender_task();
    
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
    ESP_LOGI(TAG, "📹 Camera configured for 240x240 LCD-optimized display");
    ESP_LOGI(TAG, "📺 Press any button to capture and display image on LCD + send via HTTPS");
    ESP_LOGI(TAG, "🌐 Web interface available for remote camera viewing");
}

// Embedded loading screen JPEG (added to CMakeLists EMBED_FILES)
extern const uint8_t loading_screen_jpg_start[] asm("_binary_loading_screen_jpg_start");
extern const uint8_t loading_screen_jpg_end[]   asm("_binary_loading_screen_jpg_end");

static void display_loading_screen(void)
{
    size_t jpg_len = loading_screen_jpg_end - loading_screen_jpg_start;

    // Decode JPEG → RGB565 (240×240 × 2 bytes = 115 200 B; goes to PSRAM via malloc)
    const size_t out_size = 240 * 240 * sizeof(uint16_t);
    uint8_t *out_buf = (uint8_t *)malloc(out_size);
    if (!out_buf) {
        ESP_LOGE(TAG, "Failed to allocate loading screen buffer");
        return;
    }

    if (!jpg2rgb565(loading_screen_jpg_start, jpg_len, out_buf, JPEG_IMAGE_SCALE_0)) {
        ESP_LOGE(TAG, "Failed to decode loading screen JPEG");
        free(out_buf);
        return;
    }

    // jpg2rgb565 outputs little-endian RGB565; the ST7789 LCD expects big-endian — swap each pixel's bytes
    uint16_t *pixels = (uint16_t *)out_buf;
    for (size_t i = 0; i < 240 * 240; i++) {
        pixels[i] = (uint16_t)((pixels[i] << 8) | (pixels[i] >> 8));
    }

    // Draw to LCD in 40-line chunks (same pattern as display_image_on_lcd)
    const int lines_per_chunk = 40;
    const int bytes_per_line  = 240 * sizeof(uint16_t);
    for (int y = 0; y < 240; y += lines_per_chunk) {
        int lines = (y + lines_per_chunk > 240) ? (240 - y) : lines_per_chunk;
        esp_lcd_panel_draw_bitmap(panel_handle, 0, y, 240, y + lines,
                                  out_buf + (y * bytes_per_line));
    }

    free(out_buf);
    ESP_LOGI(TAG, "Loading screen displayed");
}

// Initialize LCD display for button-triggered image showing
esp_err_t init_lcd_display(void)
{
    ESP_LOGI(TAG, "Initializing 240x240 LCD display...");
    
    bsp_display_config_t bsp_config = {
        .max_transfer_sz = (240 * 40 * sizeof(uint16_t))  // Transfer 40 lines at a time instead of full frame
    };
    
    esp_err_t ret = bsp_display_new(&bsp_config, &panel_handle, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD panel initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Turn on display
    esp_lcd_panel_disp_on_off(panel_handle, true);
    
    // Set backlight  
    bsp_display_brightness_set(100);
    
    lcd_initialized = true;

    // Create backlight power-save timer (one-shot, 10 s).
    // Not started here — the loading screen stays on until first button press.
    esp_timer_create_args_t timer_args = {
        .callback        = backlight_timer_callback,
        .arg             = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "backlight_off",
    };
    esp_err_t timer_ret = esp_timer_create(&timer_args, &s_backlight_timer);
    if (timer_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create backlight timer: %s", esp_err_to_name(timer_ret));
    }

    // Show loading screen (stays on indefinitely until a button is pressed)
    display_loading_screen();

    ESP_LOGI(TAG, "LCD display ready");
    return ESP_OK;
}

// Display camera image on LCD when button is pressed
void display_image_on_lcd(camera_fb_t *fb)
{
    if (!lcd_initialized || !panel_handle || !fb) {
        ESP_LOGE(TAG, "LCD not ready for display");
        return;
    }
    
    ESP_LOGI(TAG, "Displaying image on LCD (240x240)");
    
    // Camera is already configured for 240x240 RGB565, perfect for LCD!
    if (fb->format == PIXFORMAT_RGB565 && fb->width == 240 && fb->height == 240) {
        // Transfer image in chunks to avoid SPI buffer overflow
        const int lines_per_chunk = 40;  // Transfer 40 lines at a time
        const int bytes_per_line = 240 * sizeof(uint16_t);
        const uint8_t* src_buf = (uint8_t*)fb->buf;
        
        for (int y = 0; y < 240; y += lines_per_chunk) {
            int lines_to_send = (y + lines_per_chunk > 240) ? (240 - y) : lines_per_chunk;
            
            esp_err_t ret = esp_lcd_panel_draw_bitmap(panel_handle, 
                                                     0, y,                    // start x,y  
                                                     240, y + lines_to_send, // end x,y
                                                     src_buf + (y * bytes_per_line));
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to draw bitmap chunk at line %d: %s", y, esp_err_to_name(ret));
                return;
            }
        }
        
        ESP_LOGI(TAG, "✅ Image displayed successfully on LCD");
    } else {
        ESP_LOGW(TAG, "Frame format %d (%dx%d) not optimal for LCD display", 
                 fb->format, fb->width, fb->height);
        // Could add format conversion here if needed in the future
    }
}
