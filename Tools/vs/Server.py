import threading, socket, struct
from Util import debug

PacketMagic = 0x3facbe5a

class Server:
    def __init__(self, screen):
        self.screen = screen
        self.terminated = False

    def run(self, localport):
        self.localport = localport
        # Determine primary IP address
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                # doesn't even have to be reachable
                s.connect(('10.255.255.255', 1))
                self.local_ip = s.getsockname()[0]
        except Exception:
            self.local_ip = '127.0.0.1'

        self.thread = threading.Thread(target=self.thread_routine, daemon=True)
        self.thread.start()

    def terminate(self):
        self.terminated = True
        self.srv.close()

    def send(self, data):
        if self.client_socket is not None:
            hdr = struct.pack("2I", PacketMagic, len(data))
            self.client_socket.send(hdr)
            self.client_socket.send(data)

    def thread_routine(self):
        local_ip = self.local_ip
        localport = self.localport

        self.srv = srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((self.local_ip, self.localport))
        srv.listen(1)
        while not self.terminated:
            debug('Waiting for connection on %s:%d...' % (str(local_ip), localport))
            client_socket, addr = srv.accept()
            debug('Connected by %s' % str(addr))
            # More quickly detect bad clients who quit without closing the
            # connection: After 1 second of idle, start sending TCP keep-alive
            # packets every 1 second. If 3 consecutive keep-alive packets
            # fail, assume the client is gone and close the connection.
            try:
                client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, 1)
                client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 1)
                client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, 3)
                client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            except AttributeError:
                pass # XXX not available on windows
            client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

            self.client_socket = client_socket
            try:
                while not self.terminated:
                    try:
                        hdr = client_socket.recv(8, socket.MSG_WAITALL)
                        if not hdr:
                            break
                        if len(hdr) != 8:
                            debug("Header too small %s" % hdr)
                            continue
                        magic, datalen = struct.unpack("II", hdr)
                        if magic != PacketMagic:
                            debug("Bad Magic 0x%08x" % magic)
                            continue
                        data = client_socket.recv(datalen, socket.MSG_WAITALL)
                        self.screen.packetReceived(data)
                    except socket.error as msg:
                        debug('ERROR: %s' % msg)
                        # probably got disconnected
                        break
            except socket.error as msg:
                debug('ERROR: %s' % msg)
            finally:
                debug('Disconnected')
                client_socket.close()
                self.client_socket = None

