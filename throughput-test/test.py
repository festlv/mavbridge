import unittest
import nmcli
import time
import multiprocessing
from send_process import SendProcess, SendResult
from receive_process import ReceiveProcess, ReceiveResult
from pymavlink import mavutil

SERIAL_PORT = "/dev/ttyUSB0,57600"

UDP_IP = "udp:192.168.4.2:14550"
UDP_BCAST_IP = "udp:192.168.4.255:14550"

TCP_IP = "udp:192.168.4.1:5760"


class WifiTestCase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        """
        Need to be connected to MAVBridge-* wireless SSID before tests
        are executed
        """
        nm = nmcli.Nmcli()
        ssid = nm.wifi_ssid()

        if not ssid or not ssid.startswith("MAVBridge-"):
            # force rescan and see if the network is there after some time
            nm.force_rescan()
            time.sleep(5)
            networks = nm.list_networks()
            for n in networks:
                if n.startswith("MAVBridge-"):
                    nm.connect(n)
                    time.sleep(1)
        # verify SSID again
        ssid = nm.wifi_ssid()

        if not ssid or not ssid.startswith("MAVBridge-"):
            raise Exception("Could not connect to MAVBridge, tests not executed")

    def setup_processes(self, datarate, receive_url):

        self.send_queue = multiprocessing.Queue()
        conn = mavutil.mavlink_connection(SERIAL_PORT)
        self.send_process = SendProcess(self.send_queue, conn, datarate)

        self.receive_queue = multiprocessing.Queue()
        receive_conn = mavutil.mavlink_connection(receive_url)
        self.receive_process = ReceiveProcess(self.receive_queue, receive_conn)

    def print_result(self, send_result, receive_result):
        """
        Prints results of throughput / stability testing.
        Example output:
            Sent 570 packets, 24905 bytes
            Received 570 packets, 24905 bytes, 4083.39 bytes/sec
            Packet loss: 0.0%
        """
        print "Sent %d packets, %d bytes" % (
            send_result.packets, send_result.bytes)

        print "Received %d packets, %d bytes, %.2f bytes/sec" % (
            receive_result.valid_packets, receive_result.valid_bytes,
            receive_result.valid_bytes / receive_result.time_seconds)

        print "Packet loss: %.1f%%" % (
            100 - (float(receive_result.valid_packets) / send_result.packets) * 100)


class UDPTestCase(WifiTestCase):

    def _setup_udp_broadcast(self):
        """
        Required before UDP communications.
        Sends 1 second worth of communication
        data and listens on broadcast IP, so that communication starts.
        """
        self.setup_processes(100, UDP_BCAST_IP)
        self.receive_process.start()
        self.send_process.start()
        time.sleep(2)
        self.send_process.stop()
        self.receive_process.stop()


class TestUDPThroughput(UDPTestCase):

    def test_basic_udp(self):
        print "Setting up udp broadcasts"
        self._setup_udp_broadcast()
        print "Setting up test processes"
        self.setup_processes(3000, UDP_IP)

        self.receive_process.start()
        self.send_process.start()

        time.sleep(5)

        self.send_process.stop()

        send_result = self.send_queue.get()
        time.sleep(1)
        self.receive_process.stop()
        receive_result = self.receive_queue.get()

        self.print_result(send_result, receive_result)

        # no malformed packets
        self.assertEqual(
            receive_result.malformed_packets, 0,
            "Malformed packets received")

        self.assertEqual(
            send_result.packets, receive_result.valid_packets,
            "Number of sent and received packets don't match")
