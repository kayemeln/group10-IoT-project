from flask import Flask, request

app = Flask(__name__)

@app.route("/")
def home():
    return "Server is running!"

@app.route("/upload", methods=["POST"])
def upload():
    if "image" not in request.files:
        return "No image uploaded", 400

    image = request.files["image"]

    # Save the image
    image.save("uploaded_image.png")

    return "POST request received successfully", 200


if __name__ == "__main__":
    app.run(host="172.20.10.4", port=5000, debug=False)