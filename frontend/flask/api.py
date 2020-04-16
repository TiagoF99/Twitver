from flask import Flask, jsonify, request, render_template
import socket
import sys
from ctypes import *

app = Flask(__name__)

@app.route('/test', methods=['GET', 'POST'])
def add():

    # POST request
    if request.method == 'POST':
        nums = request.get_json()
        # must return a string value and note nums is a dict of strings
        print(nums)
        return "hello"

    # GET request
    else:
        message = {"message": main()}
        print(message)
        return jsonify(message)  # serialize and use JSON headers


""" This class defines a C-like struct """
class Payload(Structure):
    _fields_ = [("id", c_uint32),
                ("counter", c_uint32),
                ("temp", c_float)]


def main():
    server_addr = ('localhost', 55248)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # if s < 0:
    #     print("Error creating socket")

    try:
        s.connect(server_addr)
        print("Connected to %s" % repr(server_addr))
    except:
        print("ERROR: Connection to %s refused" % repr(server_addr))
        sys.exit(1)

    try:
        # while True:
            # print("")
            # payload_out = Payload(1, i, random.uniform(-10, 30))
            # print("Sending id=%d, counter=%d, temp=%f" % (payload_out.id,
            #                                             payload_out.counter,
            #                                             payload_out.temp))
            # nsent = s.send(payload_out)
            # Alternative: s.sendall(...): coontinues to send data until either
            # all data has been sent or an error occurs. No return value.
            # print("Sent %d bytes" % nsent)


        buff = s.recv(256)
        # payload_in = Payload.from_buffer_copy(buff)

        server_output = str(buff).replace('\'b\'', '').strip('b').replace("\'", "")
        # print("Received id=%d, counter=%d, temp=%f" % (payload_in.id,
        #                                             payload_in.counter,
        #                                             payload_in.temp))
        return server_output

        #s.sendall(bytes(input(server_output), 'utf-8'))
        # Alternative: s.sendall(...): coontinues to send data until either
        # all data has been sent or an error occurs. No return value.
        # print("Sent %d bytes" % nsent)

    finally:
        print("Closing socket")
        s.close()

if __name__ == "__main__":
    app.run(debug=True, port=5000)
