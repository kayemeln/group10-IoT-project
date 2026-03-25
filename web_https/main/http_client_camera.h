#ifndef HTTP_CLIENT_CAMERA_H
#define HTTP_CLIENT_CAMERA_H

#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the HTTPS camera client task
 * 
 * This function starts a FreeRTOS task that will periodically capture
 * images from the camera and POST them to a configured HTTPS server.
 * Uses TLS encryption with certificate validation disabled.
 */
void start_http_camera_client(void);

/**
 * @brief Initialize the HTTPS camera client
 * 
 * This function initializes the HTTPS client configuration without starting
 * the continuous capture task. Uses TLS with certificate validation disabled.
 */
void init_http_camera_client(void);

/**
 * @brief Trigger a single camera capture and send via HTTPS POST
 * 
 * This function captures a single frame directly from the camera and sends
 * it via encrypted HTTPS POST to the configured server.
 */
void trigger_camera_capture_and_send(void);

/**
 * @brief Trigger HTTPS POST with provided frame
 * 
 * This function sends a provided camera frame via encrypted HTTPS POST to the 
 * configured server.
 * 
 * @param fb Pointer to camera frame buffer
 */
void trigger_camera_capture_and_send_frame(camera_fb_t *fb);

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENT_CAMERA_H