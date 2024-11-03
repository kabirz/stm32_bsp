from socket import socket, AF_INET, SOCK_STREAM
import struct
import os
import sys


if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <fw_path>")
    exit(-1)

if not os.path.isfile(sys.argv[1]):
    print(f"File {sys.argv[1]} not found")
    exit(-1)

s = socket(AF_INET, SOCK_STREAM)
s.connect(('192.168.12.101', 20001))

f_size = os.path.getsize(sys.argv[1])
print(f"{sys.argv[1]} size: {f_size}")

with open(sys.argv[1], 'rb') as f:
    count = 0
    offset = f_size
    while offset > 0:
        l = min(512, offset)
        hdr = struct.pack('II', f_size, l)
        data = f.read(l)
        offset -= l
        s.send(hdr + data)
        count += 1
    data = s.recv(4)
    if int.from_bytes(data, 'little') == count:
        print("firmware upgrade successfully")
    else:
        print("firmware upgrade failed")
    
s.close()
