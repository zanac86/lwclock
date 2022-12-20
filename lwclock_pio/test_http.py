#!/usr/bin/env python
import os
from http.server import HTTPServer
from http.server import SimpleHTTPRequestHandler
import json
import random


def rnd():
    return random.randint(0, 4000)


ROUTES = [
    ('/', 'data/')
]


class MyHandler(SimpleHTTPRequestHandler):

    def translate_path(self, path):
        # default root -> cwd
        root = os.getcwd()

        # look up routes and get root directory
        for patt, rootDir in ROUTES:
            if path.startswith(patt):
                path = path[len(patt):]
                root = rootDir
                break
        # new path
        print(root, "|", path)
        return os.path.join(root, path)

    def _set_headers_json(self):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()

    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    # def do_HEAD(self):
    #     self._set_headers()

    def do_POST(self):
        if self.path.endswith("/setconfig"):
            print(self.path)
            content_length = int(self.headers['Content-Length'])
            body = self.rfile.read(content_length)
            print('This is POST request. ')
            print(body)

            response = bytes('{"OK"}', "utf-8")  # create response
            self.send_response(200)  # create header
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(response)

        #SimpleHTTPRequestHandler.do_POST(self)

    def do_GET(self):
        print(f"{self.path=}")
        if self.path.endswith("/configs.json"):
            self._set_headers_json()
            j = {
                "clockFrom": "1.3",
                "clockTo": 16.30,
                "dateFrom": 0,
                "dateTo": 24,
                "ledText0": "Hello!",
                "ledText1": "",
                "ledText2": "",
                "ledText3": "",
                "isTxtOn0": "checked",
                "isTxtOn1": "",
                "isTxtOn2": "",
                "isTxtOn3": "",
                "txtFrom0": 0,
                "txtFrom1": 0,
                "txtFrom2": 0,
                "txtFrom3": 0,
                "txtTo0": 24,
                "txtTo1": 24,
                "txtTo2": 24,
                "txtTo3": 24,
                "isCrLine0": "checked",
                "isCrLine1": "",
                "isCrLine2": "",
                "isCrLine3": "",
                "global_start": 0,
                "global_stop": 24,
                "fontUsed": 1,
                "brightd": 9,
                "brightn": 2,
                "dmodefrom": 0,
                "dmodeto": 24,
                "speed_d": 10,
                "isLedClock": "checked",
                "isLedDate": "checked",
                "dataCorrect": "",
                "hpa": "????",
                "isLedTHP": "",
                "thpFrom": 0,
                "thpTo": 24,
                "cVersion": "0.0.0",
                "time": "18:45:30",
                "date": "31 December 2022",
                "date_day": 31,
                "date_month": 12,
                "date_year": 2022,
                "time_h": "18",
                "time_m": "45",
                "time_s": "30",
            }
            s = json.dumps(j)
            self.wfile.write(bytes(s, encoding="utf-8"))
            return

        self.extensions_map = {
            '.manifest': 'text/cache-manifest',
            '.html': 'text/html',
            '.png': 'image/png',
            '.jpg': 'image/jpg',
            '.svg':	'image/svg+xml',
            '.css':	'text/css',
            '.js':	'application/x-javascript',
            '': 'application/octet-stream',  # Default
            '.json': 'application/json',
            '.xml': 'application/xml'
        }
        SimpleHTTPRequestHandler.do_GET(self)


if __name__ == '__main__':
    httpd = HTTPServer(('127.0.0.1', 8000), MyHandler)
    httpd.serve_forever()
