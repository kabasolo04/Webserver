#!/usr/bin/env python3
import sys, os, cgi

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
