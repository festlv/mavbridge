import time
from optparse import OptionParser
import sys
from pymavlink import mavutil
import math

class Spoofer:
    conn = None
    bitrate = None
    _packet_size  = None

    def __init__(self, _conn, _datarate, _packet_size):
        self.conn = _conn
        self.datarate = _datarate
        self.packet_size = _packet_size
        self.system_start = time.time()

    def timestamp_ms(self):
            return int((time.time() - self.system_start) * 1000)

    def start(self):

        print "Sending MAVLink data. Ctrl-C to stop..."

        # In one second:
        #
        # * sends heartbeat message to keep GCS happy
        # * sends attitude message with slowly varying parameters, to see if
        #   connection is alive
        # * sends DEBUG_VECT messages to keep bitrate at requested rate

        roll = 0
        pitch = 0
        yaw = (- math.pi)

        while True:
            try:
                loop_start = time.time()

                bytes_sent = 0
                bytes_to_send = self.datarate
                msg_heartbeat = self.conn.mav.heartbeat_encode(1, 1, 0, 0, 1).pack(self.conn.mav)
                bytes_sent += len(msg_heartbeat)
                self.conn.write(msg_heartbeat)

                msg_attitude = self.conn.mav.attitude_encode(
                    self.timestamp_ms(),
                    roll, pitch,
                    yaw, 0, 0, 0).pack(self.conn.mav)
                yaw += (math.pi / 180)
                if yaw > math.pi:
                    yaw = -math.pi
                bytes_sent += len(msg_attitude)
                self.conn.write(msg_attitude)

                # calculate how much data to send
                bytes_to_send -= bytes_sent

                msg_memory_vect = self.conn.mav.memory_vect_encode(
                    255, 0, 0,
                    [120] * 32).pack(self.conn.mav)

                num_packets = bytes_to_send / len(msg_memory_vect)
                time_remaining = (loop_start + 1) - time.time()

                if time_remaining > 0:
                    packet_interval = time_remaining / num_packets
                    if packet_interval > 0:
                        time.sleep(packet_interval)
                    for i in xrange(num_packets):
                        self.conn.write(msg_memory_vect)
                        bytes_sent += len(msg_memory_vect)
                        if packet_interval > 0:
                            time.sleep(packet_interval)

                loop_dt = time.time() - loop_start

                print("Sent %d bytes/sec" % (bytes_sent / loop_dt))
                #sleep for the rest of second
                if loop_dt < 1:
                    time.sleep(1 - loop_dt)
                bytes_sent = 0
            except KeyboardInterrupt:
                break



if __name__ == '__main__':
    usage = "mavspoof.py [options] port"
    parser = OptionParser(usage)

    parser.add_option("--baud", default=115200, type=int, help="Baud rate")
    parser.add_option("--datarate", default=1024, type=int, help="Data rate (bytes/sec on the wire)")
    parser.add_option("--packet-size", default=50, type=int, help="Packet size in bytes")

    (opts, args) = parser.parse_args()

    if len(args) < 1:
        print("Usage: %s" % usage)
        sys.exit(1)

    conn = mavutil.mavlink_connection(args[0], baud=opts.baud)

    s = Spoofer(conn, opts.datarate, opts.packet_size)
    s.start()
