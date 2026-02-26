from flask import Flask, request
import requests

app = Flask(__name__)
presigned_url = "https://robo-bouncer-bucket95e20-dev.s3.amazonaws.com/public/poll/uploaded_image.jpeg?AWSAccessKeyId=ASIA4OI6LYNOHEP4SJQB&Signature=E1GPbn9k37XagQPdDJPA%2BpbmInc%3D&content-type=image%2Fjpeg&x-amz-security-token=IQoJb3JpZ2luX2VjEF8aCWV1LXdlc3QtMSJHMEUCIQDTO3ZKZapLstBs6CPysjnNziNsWKxujCS2pdGhDjgr%2BgIgf5UqaLMy6TcFoJT5vzrdr1FwJrGyGUH1SYsanGww7Mkq%2BgIIKBAAGgw4NTUyOTkwNDgyODQiDE%2FcLtsDLHotJmQEoyrXArxIzLfz2zx3IAJdRVL0XJ1t2XIk8zcVmpi9GVKXQ%2B3HwjCZErnWUbs1NawR2p2kWrOC5fqslA0cNQt7Whud%2Fq7DYF03EQiA2mdg7MZrxtKGx1rlZ9yW4qIw6ZrpdO5ebt0Nq6uNVP5EHN3yMwh6%2Fdkj1%2B7oxK0xlVFO7qiWYsNzzqN5OV2MYRbCMkBJGX1hL8XcNvpy8IKuU72%2BtzJw%2BoChApGFEbDALHBzJrdCj7l0EBbMpbhA%2FsX4FNW1IRX4e3qdPM%2FN2WV7dbBWE8n8mdkrSry3t9sxl%2BQPNh%2FdSwZoGhJS2E6u%2FGwoolhTvjXJb%2BQzW251%2Fhw%2Ftec1Z8OGhZsG97Y9%2BfMkAWvlP0e3qnF1gvL2wKbDczZquN%2B8R4tf%2FW70rUQ%2B4brSdtaxy%2BjKR0oGGu8Bu5Q0V1bCHTEpaQT6WgIMuUHl1mE%2FHGJjFouAKHWzFTOS7sUwl6%2BBzQY6pQH6plX3JBxLnAWijorVfsk56A6reT%2FM%2Bt7xwyakRkhTXN%2F5FCu%2BDLcizUCAUAoTzoSJKVVFUEZ4Y6cSFgcnlyW29PaGBu18bxdcZJchYEXNTUDO9UZCzWCw3Vdq6b%2Fyx4vwP%2FeKgfZ36KzTuD76HWlJ%2F3zxxnDjN64rw9ExtQnMkx6YZfeyJ%2FRb%2B5snDn95Zm0I4TblNTemyn%2Fo7fYm7SdXym10hiY%3D&Expires=1772720662"
@app.route("/")
def home():
    return "Server is running!"

@app.route("/upload", methods=["POST"])
def upload():
    image_bytes = request.data
    if not image_bytes:
        return "No image uploaded", 400
    
    with open("uploaded_image.jpg", "wb") as f:
        f.write(image_bytes)

    response = requests.put(
        presigned_url,
        data=image_bytes,
        headers={"Content-Type": "image/jpeg", },
    )

    if response.status_code != 200:
        return f"Upload failed: {response.text}", 500

    return f"Forwarded with status {response.status_code}", response.status_code

@app.route("/image", methods=["PUT"])
def put():
    if "image" not in request.files:
        return "No image uploaded", 400
    
    image = request.files["image"]

    response = requests.put(
        presigned_url,
        data=image.read(),
        headers={"Content-Type": "image/jpeg", },
    )

    return f"Forwarded with status {response.status_code}", response.status_code

if __name__ == "__main__":
    app.run(host="172.20.10.11", port=5000, debug=True)

@app.route("/message", methods=["POST"])
def message():
    message = request.data

    response = requests.put()
    return f"Message recieved and forwarded to ESP32 {response.status_code}", 