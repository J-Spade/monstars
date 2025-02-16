import hashlib
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import os
import pathlib
import socket
import ssl
import urllib

SRV_PORT = 443
BUPKUS_CERT = pathlib.Path(__file__).parent / "bupkus.pem"
BUPKUS_ENDPOINT = "/bupkus/paste/"


class BupkusSrvHandler(BaseHTTPRequestHandler):
    """basic HTTP request handler"""

    def do_GET(self):
        # never accept GET requests
        self.send_response(404)  # 404 Not Found
        self.end_headers()

    def do_POST(self):
        if self.path != BANG_ENDPOINT:
            self.send_response(404)  # 404 Not Found
            self.end_headers()
            return
            
        content_length = int(self.headers["Content-Length"])
        post_data = self.rfile.read(content_length)
        
        try:
            data = json.loads(post_data)
            print(data)
        except:
            self.send_response(400)  # 400 Bad Request
        else:
            self.send_response(200)  # 200 OK
        self.end_headers()
        
    def do_PUT(self):
        # never accept PUT requests
        self.send_response(403)  # 403 Forbidden
        self.end_headers()


if __name__ == "__main__":
    ssl_ctx = ssl.SSLContext(protocol=ssl.PROTOCOL_TLSv1_2)
    ssl_ctx.load_cert_chain(BUPKUS_CERT)

    httpd = HTTPServer(("0.0.0.0", SRV_PORT), BupkusSrvHandler)
    httpd.socket = ssl_ctx.wrap_socket(httpd.socket, server_side=True)
    
    httpd.serve_forever()
