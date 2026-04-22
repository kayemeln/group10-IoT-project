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

/**
 * @brief Start the background JPEG sender task.
 *
 * Creates an internal FreeRTOS task and queue that handles HTTPS POSTs
 * asynchronously, so the caller is never blocked waiting for the network.
 * Call once during initialisation, after init_http_camera_client().
 */
void start_jpg_sender_task(void);

/**
 * @brief Enqueue a pre-encoded JPEG for async HTTPS POST.
 *
 * Ownership of jpg_buf is transferred: the sender task (or this function on
 * queue-full) will free() it.  The camera frame buffer can be returned to
 * the driver immediately after this call returns.
 *
 * @param jpg_buf  Heap-allocated JPEG data (will be freed after send)
 * @param jpg_len  Length of jpg_buf in bytes
 * @param ts_sec   Frame timestamp seconds
 * @param ts_usec  Frame timestamp microseconds
 */
void enqueue_jpg_for_send(uint8_t *jpg_buf, size_t jpg_len, int64_t ts_sec, int64_t ts_usec);

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENT_CAMERA_H