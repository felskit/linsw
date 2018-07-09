#!/usr/bin/python

import socket
import datetime

print "Hello world!"

UDP_IP = "192.168.143.171"
UDP_PORT = 56000
MESSAGE = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

print "UDP target IP:", UDP_IP
print "UDP target port:", UDP_PORT
print "message:", MESSAGE

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
