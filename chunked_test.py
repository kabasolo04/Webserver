import socket
import time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 8080))

req = (
    "POST /upload HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Transfer-Encoding: chunked\r\n"
    "\r\n"
)

s.sendall(req.encode())

# send first chunk
s.sendall(b"5\r\nhello\r\n")
time.sleep(0.5)

# send second chunk
s.sendall(b"6\r\n world\r\n")
time.sleep(0.5)

# final chunk
s.sendall(b"0\r\n\r\n")

print(s.recv(4096).decode())
s.close()