import http.server 
import socketserver
from flask import Flask

app = Flask(__name__)

@app.route("/")
def hello_world():
    return "<p>Hello, World!</p>"

"""
PORT = 8080

class CustomHandler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-length'])
        post_data = self.rfile.read(content_length)
        
        print(self.headers['Content-Type'])

        print(f"Recieved POST data: {post_data.decode('utf-8')}")

        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

        response_message = b"POST request recieved successfully"
        self.wfile.write(response_message)
    
with socketserver.TCPServer(("", PORT), CustomHandler) as httpd:
    print(f"Serving at port {PORT}")
    httpd.serve_forever()
"""