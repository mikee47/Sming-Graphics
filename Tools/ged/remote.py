import socket, struct
from item import *
from gtypes import ColorFormat, FontStyle
import urllib.parse

class Client:
    def __init__(self, ipaddr, port):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((ipaddr, port))

    def send_line(self, data):
        if isinstance(data, str):
            data = data.encode()
        self.socket.send(data + b'\n')


def serialise(layout: list) -> list[str]:
    data = []
    for item in layout:
        line = f'i:{item.typename};'
        for name, value in item.asdict().items():
            if name == 'id':
                continue
            if name == 'halign':
                s = Align[value].value
            elif name == 'valign':
                s = Align[value].value
            elif name == 'fontstyle':
                n = 0
                for x in value:
                    n |= 1 << FontStyle[x].value
                s = hex(n)
            elif name == 'text':
                s = urllib.parse.quote(value.replace('\n', '\r\n').encode())
            elif type(value) in [int, float]:
                s = value
            elif type(value) is Color:
                s = value.value_str(ColorFormat.graphics)[1:]
            elif type(value) in [set, list]:
                s = ",".join(value)
            else:
                s = value
            line += f'{name}={s};'
        data.append(line)
    return data
