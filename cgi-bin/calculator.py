#!/usr/bin/env python
# -*- coding: utf-8 -*-

import urllib.parse
import sys
import cgi, cgitb

try:
    form = cgi.FieldStorage()
    exp = form.getvalue("exp")
    res = eval(exp)

    print("Content-Type:text/html", end="\r\n\r\n")

    print('''<html>
    <head>
        <meta charset=utf-8>
        <title>CGI Calculator</title>
    </head>
    <body>
    <h2>Result:{0}</h2>
    <hr>
		<form action="/cgi-bin/calculator.py" method="POST">
			Input an expression: <input type="text" name="exp" value="{1}" />
			<input type="submit" value="result">
		</form>
    </body>
</html>'''.format(res, exp))

except:
    print("Content-Type:text/html", end="\r\n\r\n")
    print('''<html>
    <head>
    </head>
    <body>
    <h2>Error: Please input a valid expression </h2>
    <hr>
		<form action="/cgi-bin/calculator.py" method="POST">
			Input an expression: <input type="text" name="exp" value="{0}"/>
			<input type="submit" value="result">
		</form>
    </body>
</html>'''.format(exp))
