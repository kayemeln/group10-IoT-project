# Automatic Party Entry - CS7NS2

- **Branches:** Perhaps we can all create our own branch for the things we are working on?

---

I've toyed around a little with the ESP32-S3-EYE. I read the [ESP32-S3-EYE Getting Started Guide](https://github.com/espressif/esp-who/blob/master/docs/en/get-started/ESP32-S3-EYE_Getting_Started_Guide.md) to get started using it. 
### `camera_example`
This doesn't do much - just capturing a JPEG and uploading to a HTTP server on the ESP32. You can view the image on your computer through `curl` or just on your browser. It's mostly a combination of the [ESP-IDF Simple HTTPD Server Example](https://github.com/espressif/esp-idf/tree/v5.5.2/examples/protocols/http_server/simple) and the [ESP32 Camera Driver Example](https://github.com/espressif/esp32-camera/tree/master/examples/camera_example). To get started using it follow these:
- Firstly, you'll need to get ESP-IDF. [Here](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/linux-macos-setup.html#get-started-linux-macos-first-steps) are some detailed instructions for this.
- Next, run the ESP_IDF `export.sh` script while in the `camera_example` directory.
- Set the device target to the ESP32-S3 - `idf.py set-target esp32s3`.
- Open the configuration menu with `idf.py menuconfig`:
    - In **Example Connection Configuration**, enter your WiFi SSID and password.
    - In **Component config** &#8594; **ESP PSRAM** &#8594; **SPI RAM config** &#8594; **Mode (QUAD/OCT)**, ensure that **Octal Mode** is chosen.
    - Save and exit.
- Build the project with `idf.py all`.
- Flash and monitor with `idf.py -p PORT flash monitor`.
    - This should display the IP address of the server and the port number.
    - In your browser enter `http://<ip-address>:<port>/stream`.
    - You will be blessed with an image like this:
![Selfie](selfie.jpg)
