import argparse, os, sys, struct, array, time, copy
from sdl2 import *
from ctypes import *
from pixels import *
from Util import debug
from DisplayList import DisplayList, Code, Command
from Server import Server, TouchMagic
from queue import Queue, Empty
from threading import Timer

app_name = 'Virtual Screen'
BYTES_PER_PIXEL = 3
BORDER_THICKNESS = 3
BORDER_SPACE = 8
USEREVENT_MOUSETIMER = 2

class AddressWindow:
    def __init__(self):
        self.initial = Rect()
        self.bounds = Rect()
        self.column = 0

    def setColumn(self, x, w):
        self.bounds.x, self.bounds.w = x, w
        self.initial.x, self.initial.w = x, w
        self.column = 0
    
    def setRow(self, y, h):
        self.bounds.y, self.bounds.y = y, h
        self.initial.y, self.initial.h = y, h
        self.column = 0

    def reset(self):
        self.bounds = copy.copy(self.initial)
        self.column = 0

    def step(self):
        self.column += 1
        if self.column < self.bounds.w:
            return
        self.column = 0
        if self.bounds.h == 0:
            return
        self.bounds.y += 1
        self.bounds.h -= 1

    def pos(self):
        return self.bounds.x + self.column, self.bounds.y

    def __bool__(self):
        return self.bounds.h != 0


class Screen:
    def __init__(self):
        self.terminated = False
        self.width, self.height = 0, 0
        self.title = app_name
        self.window = None
        self.destRect = None
        self.srcRect = None
        self.packetQueue = Queue(2)
        SDL_Init(SDL_INIT_VIDEO)
        self.window = SDL_CreateWindow(self.title.encode(),
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            640, 480, SDL_WINDOW_RESIZABLE)
        self.renderer = SDL_CreateRenderer(self.window, -1, 0)
        self.texture = None
        self.resize(320, 240)
        self.mouse_state = 0
        self.mouse_pos = (0, 0)
        self.mouse_timer = None # Rate-limit mouse messages
        self.mouse_notify = False

    def __del__(self):
        SDL_DestroyRenderer(self.renderer)
        SDL_DestroyTexture(self.texture)
        SDL_DestroyWindow(self.window)
        SDL_Quit()

    def resize(self, width, height):
        if self.width == width and self.height == height:
            return
        debug("SIZE (%u x %u) -> (%u x %u)" % (self.width, self.height, width, height))
        self.width, self.height = width, height
        self.addr = AddressWindow()
        if self.texture is not None:
            SDL_DestroyTexture(self.texture)
        # Off-screen texture blitted to window, but contents managed in separate pixel buffer
        self.texture = SDL_CreateTexture(self.renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_TARGET, width, height)
        self.pixels = PixelBuffer(width, height)
        self.windowChanged()

    def windowChanged(self):
        # Swapchain's been recreated so restore texture data and update screen
        self.pixels.updateTexture(self.texture)
        # Calculate scaling
        c_w, c_h = c_int(), c_int()
        SDL_GetWindowSize(self.window, byref(c_w), byref(c_h))
        m = (BORDER_SPACE + BORDER_THICKNESS) * 2
        dstWidth, dstHeight = c_w.value, c_h.value
        sw, sh = (dstWidth - m) / self.width, (dstHeight - m) / self.height
        scale = min(sw, sh)

        if scale < 1:
            w, h = min(self.width, dstWidth), min(self.height, dstHeight)
            x = (dstWidth - w) // 2 if sw > 1 else 0
            y = (dstHeight - h) // 2 if sh > 1 else 0
            self.srcRect = Rect(0, 0, w, h)
            self.destRect = Rect(x, y, w, h)
        else:
            scale = int(scale) # Integer scaling to avoid artifacts
            w, h = int(self.width * scale), int(self.height * scale)
            self.srcRect = Rect(0, 0, self.width, self.height)
            x = (dstWidth - w) // 2
            y = (dstHeight - h) // 2
            self.destRect = Rect(x, y, w, h)

        w, h = int(self.width * scale), int(self.height * scale)
        self.srcRect = Rect(0, 0, self.width, self.height)
        x = (dstWidth - w) // 2
        y = (dstHeight - h) // 2
        self.destRect = Rect(x, y, w, h)
        # Finally, redraw
        self.update()

    def update(self):
        SDL_SetRenderDrawColor(self.renderer, 0, 0, 0, 255)
        SDL_RenderFillRect(self.renderer, None)
        SDL_SetRenderDrawColor(self.renderer, 50, 50, 50, 255)
        r = Rect()
        w = BORDER_THICKNESS
        r.x = self.destRect.x - w
        r.y = self.destRect.y - w
        r.w = self.destRect.w + w + w
        r.h = self.destRect.h + w + w
        SDL_RenderFillRect(self.renderer, r.sdl_rect())
        SDL_RenderCopy(self.renderer, self.texture, self.srcRect.sdl_rect(), self.destRect.sdl_rect())
        SDL_RenderPresent(self.renderer)

    def setTitle(self, title):
        self.title = title
        SDL_SetWindowTitle(self.window, title.encode())

    def run(self):
        parser = argparse.ArgumentParser(description=app_name)
        parser.add_argument(
            '-P', '--localport',
            type=int,
            help='local TCP port',
            default=7780)
        args = parser.parse_args()
        self.server = Server(self)
        self.server.run(args.localport)
        self.setTitle("%s @ %s:%s" % (app_name, self.server.local_ip, self.server.localport))
        self.update()
        event = SDL_Event()
        wait_time = 0.1
        while True:
            try:
                packet = self.packetQueue.get(timeout=wait_time)
                self.processPacket(packet)
                wait_time = 0.01
            except Empty:
                wait_time = 0.1
            except Exception as err:
                debug(str(err))
            while SDL_PollEvent(byref(event)):
                if event.type == SDL_QUIT:
                    self.server.terminate()
                    return
                if event.type == SDL_WINDOWEVENT:
                    if event.window.event in [SDL_WINDOWEVENT_RESIZED, SDL_WINDOWEVENT_SIZE_CHANGED, SDL_WINDOWEVENT_MOVED]:
                        self.windowChanged()
                elif event.type == SDL_MOUSEMOTION:
                    self.mouseEvent(event.motion.state, event.motion.x, event.motion.y)
                elif event.type in [SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP]:
                    self.mouseEvent(SDL_GetMouseState(None, None), event.button.x, event.button.y)
                elif event.type == SDL_USEREVENT and event.user.code == USEREVENT_MOUSETIMER:
                    self.mouseNotify()


    def mouseEvent(self, state, x, y):
        if (self.mouse_state or state) == 0:
            return
        x = (x - self.destRect.x) * self.srcRect.w // self.destRect.w
        y = (y - self.destRect.y) * self.srcRect.h // self.destRect.h
        pos = (x, y)
        if (self.mouse_state != state or self.mouse_pos != pos) and self.srcRect.contains(x, y):
            self.mouse_state = state
            self.mouse_pos = pos
            self.mouseNotify()


    def mouseNotify(self):
        if self.mouse_timer:
            self.mouse_notify = True
            return
        self.mouse_notify = False
        data = struct.pack("I2H", self.mouse_state, self.mouse_pos[0], self.mouse_pos[1])
        self.server.send(data, TouchMagic)
        def callback():
            self.mouse_timer = None
            if self.mouse_notify:
                event = SDL_Event()
                event.type = SDL_USEREVENT
                event.user.code = USEREVENT_MOUSETIMER
                SDL_PushEvent(event)
        self.mouse_timer = Timer(0.1, callback)
        self.mouse_timer.start()


    # Called in TCP thread context
    def packetReceived(self, packet):
        # debug("received %u" % len(packet))
        self.packetQueue.put(packet)
        # debug("queued %u" % len(packet))

    # Called in main thread context
    def processPacket(self, packet):
        # debug("process %u" % len(packet))
        list = DisplayList(packet)
        while not list.done():
            offset = list.offset
            cmd, datalen = list.readHeader()
            # debug("@ %u hdr %s(%u)" % (offset, Code(cmd), datalen))
            if cmd == Code.setColumn:
                self.addr.setColumn(list.readVar(), datalen + 1)
            elif cmd == Code.setRow:
                self.addr.setRow(list.readVar(), datalen + 1)
            elif cmd == Code.writeStart:
                self.addr.reset()
                data = list.read(datalen)
                self.setPixels(data)
            elif cmd == Code.writeData:
                data = list.read(datalen)
                self.setPixels(data)
            elif cmd == Code.writeDataBuffer:
                addr = list.read(4) # Discard pointer to buffer
                data = self.packetQueue.get(timeout=0.5)
                if data is None:
                    debug("Missing WRITE packet")
                else:
                    self.setPixels(data)
            elif cmd == Code.repeat:
                repeats = list.readVar()
                data = list.read(datalen)
                for rep in range(repeats):
                    self.setPixels(data)
            elif cmd == Code.command:
                cmd = list.readByte()
                data = list.read(datalen)
                # debug("cmd %u, %u" % (cmd, datalen))
                if cmd == Command.setSize:
                    w, h = struct.unpack("HH", data)
                    self.resize(w, h)
                elif cmd == Command.copyPixels:
                    src = Rect()
                    src.x, src.y, src.w, src.h, dstx, dsty = struct.unpack("6H", data)
                    self.pixels.copy(src, dstx, dsty)
                    self.updateTexture(Rect(dstx, dsty, src.w, src.h))
                elif cmd == Command.scroll:
                    area = Rect()
                    area.x, area.y, area.w, area.h, shiftx, shifty, wrapx, wrapy, fill = struct.unpack("4H2h2?I", data)
                    self.pixels.scroll(area, shiftx, shifty, wrapx, wrapy, fill)
                    self.updateTexture(area)
                elif cmd == Command.fill:
                    r = Rect()
                    r.x, r.y, r.w, r.h, color = struct.unpack("4HI", data)
                    # debug("Fill (%s), 0x%08x" % (r, color))
                    self.pixels.fill(r, color)
                else:
                    debug("BAD COMMAND: %s" % cmd)
            elif cmd == Code.read or cmd == Code.readStart:
                if cmd == Code.readStart:
                    self.addr.reset()
                list.read(4) # Discard pointer to buffer
                buffer = self.readPixels(datalen // BYTES_PER_PIXEL)
                # debug("READ(%u): %u" % (datalen, len(buffer)))
                self.server.send(buffer)
            elif cmd == Code.callback:
                list.read(4) # Discard pointer to callback
                list.offset = (list.offset + 3) & ~3
                list.read(datalen)
            # debug("addr %s, column %u" % (self.addr, self.column))
        self.pixels.updateTexture(self.texture)
        self.update()

    def updateTexture(self, r = None):
        self.pixels.updateTexture(self.texture, r)

    def readPixels(self, pixelCount):
        len = 0
        buf = bytearray(pixelCount * BYTES_PER_PIXEL)
        for i in range(pixelCount):
            if not self.addr:
                break
            pt = self.addr.pos()
            pix = self.pixels.get(pt[0], pt[1])
            self.addr.step()
            off = i * 3
            buf[off + 0] = pix & 0xff
            buf[off + 1] = (pix >> 8) & 0xff
            buf[off + 2] = (pix >> 16) & 0xff
            len += 3
        return buf

    def setPixel(self, r, g, b):
        x, y = self.addr.pos()
        self.addr.step()
        self.pixels.set(x, y, makeColor(r, g, b))

    def setPixels(self, data):
        for b, g, r in struct.iter_unpack("BBB", data):
            self.setPixel(r, g, b)


if __name__ == '__main__':
    try:
        screen = Screen()
        screen.run()
        debug("%s finished" % app_name)
    except RuntimeError as e:
        debug(e)
        sys.exit(2)
