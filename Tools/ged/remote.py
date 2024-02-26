import socket, struct
from item import *
import urllib.parse

class Client:
    def __init__(self, ipaddr, port):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((ipaddr, port))

    def send(self, data):
        self.socket.send(data)


def serialise(layout: list) -> list[str]:
    data = []
    for item in layout:
        line = item.typename + ':'
        for name, value in item.asdict().items():
            if name == 'id':
                continue
            if type(value) in [int, float]:
                s = value
            elif type(value) is Color:
                s = value.value_str()[1:]
            elif type(value) in [set, list]:
                s = ",".join(value)
            else:
                s = urllib.parse.quote(value.encode())
            line += f'{name}={s};'
        data.append(line)
    return data
