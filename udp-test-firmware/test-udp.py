import socket
import time
import struct


def receive_server(ip, port):
    """
    Listens on UDP socket, waiting for data.
    Data is assumed to have uint64_t counter in the beginning.
    Doesn't care about the rest of packet.

    When killed, outputs the following:
        pkts/s
        bytes/s
        number of packets received / lost
        percentage of packets lost / received
    """

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((ip, port))

    packets_received = 0
    packets_lost = 0
    first_packet_time = 0
    prev_packet_seq = 0
    bytes_received = 0

    try:
        while True:
            data, addr = sock.recvfrom(1024)  # max packet size 1KiB
            fmt = "<q%ds" % 120

            (seq_num, payload) = struct.unpack(fmt, data)

            if packets_received == 0:
                #this is the first one
                first_packet_time = time.time()
                prev_packet_seq = seq_num
            else:
                packets_between = seq_num - prev_packet_seq
                if packets_between > 1:
                    packets_lost += packets_between - 1

            prev_packet_seq = seq_num
            packets_received += 1
            bytes_received += len(data)



    except KeyboardInterrupt:
        print "Packets received: %d" % packets_received
        print "Packets lost: %d" % packets_lost
        print "Bytes received: %d" % bytes_received
        print "Prev packet seq: %ld" % prev_packet_seq
        print "Packet loss: %.6f%%" % (float(packets_lost) / float(packets_received + packets_lost) * 100.0)
        dt = time.time() - first_packet_time
        print "Speed: %.1f pkts/s, %.1f bytes/s" % (
            packets_received / dt, bytes_received / dt)

if __name__ == "__main__":
    receive_server("0.0.0.0", 1234)
