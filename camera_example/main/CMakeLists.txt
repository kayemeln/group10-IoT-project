set(requires esp_timer nvs_flash esp_psram esp-tls nvs_flash esp_netif esp_http_server)
idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    list(APPEND requires esp_stubs protocol_examples_common)
else()
    list(APPEND requires esp_wifi esp_eth)
endif()

idf_component_register(SRCS take_picture.c
                        PRIV_INCLUDE_DIRS .
                        PRIV_REQUIRES ${requires}) 
