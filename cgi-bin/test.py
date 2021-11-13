#!/usr/bin/env python
# -*- coding: utf-8 -*-

import cgi, cgitb

form = cgi.FieldStorage()

print("Content-Type:text/html", end="\r\n\r\n")

print('''<html>
    <head>
        <meta charset=utf-8>
    </head>
    <body>
    <h1>Hello world!</h1>
    </body>
</html>''')

with open("test.txt", "w+") as f:
    f.write("Test");
    

