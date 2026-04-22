idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "include"
                       REQUIRES nvs_flash esp_wifi esp_netif mqtt esp_event esp_system lcd board)

# Create a SPIFFS image from the contents of the 'spiffs_image' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)