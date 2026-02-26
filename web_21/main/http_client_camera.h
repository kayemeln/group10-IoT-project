#ifndef HTTP_CLIENT_CAMERA_H
#define HTTP_CLIENT_CAMERA_H

#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the HTTP camera client task
 * 
 * This function starts a FreeRTOS task that will periodically capture
 * images from the camera and POST them to a configured HTTP server.
 */
void start_http_camera_client(void);

/**
 * @brief Initialize the HTTP camera client
 * 
 * This function initializes the HTTP client configuration without starting
 * the continuous capture task.
 */
void init_http_camera_client(void);

/**
 * @brief Trigger a single camera capture and send via HTTP POST
 * 
 * This function captures a single frame directly from the camera and sends
 * it via HTTP POST to the configured server.
 */
void trigger_camera_capture_and_send(void);

/**
 * @brief Trigger HTTP POST with provided frame
 * 
 * This function sends a provided camera frame via HTTP POST to the 
 * configured server.
 * 
 * @param fb Pointer to camera frame buffer
 */
void trigger_camera_capture_and_send_frame(camera_fb_t *fb);

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENT_CAMERA_H