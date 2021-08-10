from enum import IntEnum

# Header codes
class Code(IntEnum):
    none = 0,
    command = 1,
    repeat = 2,
    setColumn = 3,
    setRow = 4,
    writeStart = 5,
    writeData = 6,
    writeDataBuffer = 7,
    readStart = 8,
    read = 9,
    callback = 10,

# Virtual command codes
class Command(IntEnum):
    setSize = 0,
    copyPixels = 1,
    scroll = 2,
    fill = 3,

class DisplayList():
    def __init__(self, data):
        self.data = data
        self.offset = 0

    def done(self):
        return self.offset >= len(self.data)

    # Header structure
    #   uint8_t cmd : 4;
    #   uint8_t len : 4; // Set to 0x1f if length follows header
    def readHeader(self):
        cmd = self.data[self.offset]
        self.offset += 1
        datalen = cmd >> 4
        if datalen == 0x0f:
            datalen = self.readVar()
        return cmd & 0x0f, datalen

    def readByte(self):
        value = self.data[self.offset]
        self.offset += 1
        return value

    def read(self, datalen):
        data = self.data[self.offset : self.offset + datalen]
        self.offset += datalen
        return data

    def readVar(self):
        count = self.data[self.offset]
        self.offset += 1
        if count & 0x80:
            count = (count & 0x7f) << 8
            count |= self.data[self.offset]
            self.offset += 1
        return count
