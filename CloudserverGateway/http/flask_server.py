from flask import Flask, request
import requests

app = Flask(__name__)
presigned_url = "https://robo-bouncer-bucket95e20-dev.s3.amazonaws.com/public/poll/uploaded_image.jpeg?AWSAccessKeyId=ASIA4OI6LYNOOPXTUQ5U&Signature=dz53SI%2BXEtR9K10KKAGQVN2Wa5o%3D&content-type=image%2Fjpeg&x-amz-security-token=IQoJb3JpZ2luX2VjEEgaCWV1LXdlc3QtMSJGMEQCIQC%2BWF7hnXUGQaCVgZfN0UiRGYVPZFLJRCrI926HTO%2FBbwIfB%2FD1VrBxbYAnmupRZBXADhq2QEaAGLwxPV0BGFOJhSr6AggREAAaDDg1NTI5OTA0ODI4NCIMW4HnuXVTiehAzstlKtcCAM26L6Yj7zKSp66wAc2%2Bsg2%2B4gLBv3oMT4FRUbTomT7x4FcYgxi2siiaNEOoedCXvnmavFDfkrNxwOc6FFHtOg3kWSrap1%2F%2BhZbooVWAIpTxBFY6ahIqQXVicxCErw%2BfLcfafdpinaB353vq6%2Bl8yYjtsugq96LlV4l0S3TewKCih6%2BLT0zXcuRYQPXwL8WCfNMyzNVy4%2FC91BztmBtPyf2V6NrNQpr%2BJKEWO7KvYrSdiniImrZWZard40LAngXT93i3D4H5Uhw4b6JN%2FjWQg4%2BSWmcuwnznzZGNlGDWftopcRiYQ7Rk1r0q4dceFU2QQE9luIvawq%2B%2BbymmJjbMKto8WhuncNsAO2JYQxmFGVqjWVfX%2FGCIXB%2F3%2BUxi%2F8%2BQ%2B1cU4wyrSpIpuSzxfQhjCFBIspFCXVZuuXX94jPNsv2Oy4r9ywQue0z0b5XeliOB7cbvF%2BF0%2BjDos%2FzMBjqmAc1D9a%2BIWrv8d3VFlwbt8aTbBTtAq1O7ZteeNc%2F7PCvuTCY33Zf9NrtK9svODn0lNdEd2sDvATKxFHfdytL4lB64LYpFFSCAfqFjXlCNbxiIRHDdYYwsO9eqWwDQLM%2FF9sqx9JhK%2FS%2Bk103%2BBmjyybrvQzWPnC4TgVR5GT6W%2FIg9ctug%2By5HV8CGptdeYMd%2BjlaDc1BRE38gCJyfBQUA4K26s0lyW7I%3D&Expires=1772639334"

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
    app.run(host="172.20.10.2", port=5000, debug=True)

@app.route("/message", methods=["POST"])
def message():
    message = request.data

    response = requests.put()
    return f"Message recieved and forwarded to ESP32 {response.status_code}", 