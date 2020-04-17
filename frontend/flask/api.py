from flask import Flask, jsonify, request, render_template
import socket
import sys
from ctypes import *

app = Flask(__name__)
s_sock = None

"""
POST REQUEST:
    if request.method == 'post':
        nums = request.get_json()
        return "hello"

s.sendall(bytes(input(server_output), 'utf-8'))
"""

@app.route('/connect', methods=['GET', 'POST'])
def connect():
    global s_sock

    # POST request
    if request.method == 'GET':
        server_addr = ('localhost', 55248)
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            s.connect(server_addr)
            print("Connected to %s" % repr(server_addr))
            if not s_sock:
                s_sock = s
        except:
            print("ERROR: Connection to %s refused" % repr(server_addr))
            sys.exit(1)

        try:
            buff = s.recv(256)
            server_output = str(buff).replace('\'b\'', '').strip('b').replace(
                "\'", "").replace("\\r\\n", "").strip()
            print({"message": server_output})
            return jsonify({"message": server_output})  # serialize and use JSON headers
        except:
            print("Closing socket")
            s.close()


@app.route('/write', methods=['GET', 'POST'])
def write():
    global s_sock
    # POST request
    if request.method == 'POST':
        if s_sock:
            data = request.get_json()
            print("data recieved: {}".format(data))
            try:
                # send your input
                print(data["input"])
                s_sock.sendall(bytes(data["input"] + "\r\n", 'utf-8'))

                print("here")
                # receive back a response
                buff = s_sock.recv(256)
                print("now here")
                server_output = str(buff).replace('\'b\'', '').strip('b').replace("\'", "").replace("\\r\\n", "").strip()
                print({"message": server_output})
                return jsonify({"message": server_output})
            except:
                print("Closing socket in write()")
                s_sock.close()
        else:
            print("s_sock is NULL")


class Payload(Structure):
    """
    This class defines a C-like struct
    """
    _fields_ = [("id", c_uint32),
                ("counter", c_uint32),
                ("temp", c_float)]


if __name__ == "__main__":
    app.run(debug=True, port=5000)
