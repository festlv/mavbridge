import time
from optparse import OptionParser
import sys
from pymavlink import mavutil


class MAVRecv:
    conn = None

    def __init__(self, conn):
        self.conn = conn

    def start(self):
        """Receives packets in a loop, prints out total received packets on Ctrl-C"""
        print "Starting receive loop"
        packets_received = 0
        bytes_received = 0
        start_time = time.time()
        while True:
            try:
                m = self.conn.recv_msg()
                if m:
                    packets_received += 1
                    try:
                        buf = m.pack(self.conn.mav)
                        bytes_received += len(buf)
                    except TypeError:
                        bytes_received += len(m.data)

            except KeyboardInterrupt:
                break

        dt = time.time() - start_time
        print "Total packets received: %d" % packets_received
        print "%.2f messages/second" % (packets_received / dt)
        print "%.2f bytes/second" % (bytes_received / dt)


if __name__ == "__main__":

    usage = "mavrecv.py [options] device"
    parser = OptionParser(usage)

    (opts, args) = parser.parse_args()

    if len(args) < 1:
        print("Usage: %s" % usage)
        sys.exit(1)

    conn = mavutil.mavlink_connection(args[0])
    print "Connected"
    s = MAVRecv(conn)
    s.start()
