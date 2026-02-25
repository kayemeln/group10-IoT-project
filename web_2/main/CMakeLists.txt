# Set required dependencies for HTTP client functionality
set(requires esp-tls nvs_flash esp_event esp_netif esp_http_client modules esp32-camera)

idf_component_register(SRCS "app_main.cpp" "http_client_camera.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES ${requires})
