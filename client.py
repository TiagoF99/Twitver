#!/usr/bin/env python

""" client.py - Echo client for sending/receiving C-like structs via socket
References:
- Ctypes fundamental data types: https://docs.python.org/2/library/ctypes.html#ctypes-fundamental-data-types-2
- Ctypes structures: https://docs.python.org/2/library/ctypes.html#structures-and-unions
- Sockets: https://docs.python.org/2/howto/sockets.html
"""

import socket
import sys
import random
from ctypes import *


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
        while True:
            # print("")
            # payload_out = Payload(1, i, random.uniform(-10, 30))
            # print("Sending id=%d, counter=%d, temp=%f" % (payload_out.id,
            #                                             payload_out.counter,
            #                                             payload_out.temp))
            # nsent = s.send(payload_out)
            # Alternative: s.sendall(...): coontinues to send data until either
            # all data has been sent or an error occurs. No return value.
            # print("Sent %d bytes" % nsent)

            print(3)

            buff = s.recv(256)
            # payload_in = Payload.from_buffer_copy(buff)

            print(4)
            server_output = str(buff).replace('\'b\'', '').strip('b')
            # print("Received id=%d, counter=%d, temp=%f" % (payload_in.id,
            #                                             payload_in.counter,
            #                                             payload_in.temp))

            print(1)
            s.sendall(bytes(input(server_output), 'utf-8'))
            # Alternative: s.sendall(...): coontinues to send data until either
            # all data has been sent or an error occurs. No return value.
            # print("Sent %d bytes" % nsent)
            print(2)
    finally:
        print("Closing socket")
        s.close()


if __name__ == "__main__":
    main()
