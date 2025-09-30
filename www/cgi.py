""" #!/usr/bin/env python3
import os, sys, urllib.parse

print("Content-type: text/html\r\n\r\n")
print("<html><body>")
print("<h1>POST Received</h1>")

content_length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(content_length)
params = urllib.parse.parse_qs(body)

print(f"<p>Raw body: {body}</p>")
for key, value in params.items():
    print(f"<p>{key} = {value[0]}</p>")

print("</body></html>") """

#!/usr/bin/env python3
import cgi, os, sys
form = cgi.FieldStorage()
print("Content-Type: text/html\r\n")
print("<html><body>")
print("POST data:", form)
print("</body></html>")
