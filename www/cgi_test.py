#!/usr/bin/env python3
import sys, os, cgi

# Emit CGI header so the webserver or client knows the content type
# Use explicit CRLF CRLF termination so the server's header parser sees the end of headers
sys.stdout.write("Content-Type: text/html\r\n\r\n")
sys.stdout.flush()

form = cgi.FieldStorage()
print("<html><body>")
print("<h1>Python CGI Test</h1>")
print("<p>Method: {}</p>".format(os.environ.get("REQUEST_METHOD")))
print("<p>Content Length: {}</p>".format(os.environ.get("CONTENT_LENGTH")))
print("<p>Query String: {}</p>".format(os.environ.get("QUERY_STRING")))
print("<p>Form data:</p>")
for key in form.keys():
    print("<p>{} = {}</p>".format(key, form[key].value))
print("</body></html>")
