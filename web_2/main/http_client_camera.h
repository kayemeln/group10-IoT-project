#ifndef HTTP_CLIENT_CAMERA_H
#define HTTP_CLIENT_CAMERA_H

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

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENT_CAMERA_H