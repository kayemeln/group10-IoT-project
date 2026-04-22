set(requires esp-tls nvs_flash esp_event esp_netif esp_http_client modules esp32-camera button esp_lcd)

# Embed certificate for HTTPS verification
set(CERTS cert.pem)

# Embed binary assets
set(ASSETS loading_screen.jpg)

idf_component_register(SRCS "app_main.cpp" "http_client_camera.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ${CERTS}
                    EMBED_FILES ${ASSETS}
                    PRIV_REQUIRES ${requires})
