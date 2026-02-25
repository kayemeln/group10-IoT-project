import http.server 
import socketserver
from flask import Flask, request 


app = Flask(__name__)
@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        f = request.files['file']

