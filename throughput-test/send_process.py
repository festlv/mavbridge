import multiprocessing
import time
import collections
import math
import Queue

SendResult = collections.namedtuple(
    'SendResult',
    ['packets', 'bytes', 'time_seconds'])


class SendProcess(multiprocessing.Process):
    """
    Sends semi-random MAVLink messages at requested data rate.

    Sample usage:
        conn = mavutil.mavlink_connection("/dev/ttyUSB0,115200")
        q = multiprocessing.Queue()
        send_process = SendProcess(q, conn, requested_data_rate)
        send_process.start()

        # after some time

        send_process.stop()

        # there will be a SendResult in queue
        result = q.get()
    """

    # queue to post results to caller
    queue = None
    # mavutil.mavlink_connection to listen for packets
    conn = None
    # requested datarate, bytes per second
    datarate = None

    MSG_STOP = 0x00

    # time.time() when started sending
    start_time = time.time()

    # total bytes/packets sent counters
    total_bytes_sent = 0
    total_packets_sent = 0

    # attitude simulation
    yaw = 0
    pitch = 0
    roll = 0

    def __init__(self, queue, connection, datarate):
        super(SendProcess, self).__init__()
        self.queue = queue
        self.conn = connection
        self.datarate = datarate

    def stop(self):
        """ Gracefully stop the subprocess """
        self.queue.put(self.MSG_STOP)
        self.join()

    def _timestamp_ms(self):
        return int((time.time() - self.start_time) * 1000)

    def _heartbeat_send(self):
        msg_heartbeat = self.conn.mav.heartbeat_encode(
            1, 1, 0, 0, 1).pack(self.conn.mav)
        self.conn.write(msg_heartbeat)

        self.total_packets_sent += 1
        self.total_bytes_sent += len(msg_heartbeat)
        return len(msg_heartbeat)

    def _attitude_send(self):
        """
        Sends time-varying attitude (to see if data changes).
        """
        msg_attitude = self.conn.mav.attitude_encode(
            self._timestamp_ms(),
            self.roll, self.pitch,
            self.yaw, 0, 0, 0).pack(self.conn.mav)
        self.yaw += (math.pi / 180)
        if self.yaw > math.pi:
            self.yaw = -math.pi

        self.conn.write(msg_attitude)

        self.total_packets_sent += 1
        self.total_bytes_sent += len(msg_attitude)

        return len(msg_attitude)

    def _memory_vect_send(self, msg):
        msg_memory_vect = msg
        self.conn.write(msg_memory_vect)
        self.total_bytes_sent += len(msg_memory_vect)
        self.total_packets_sent += 1

        return len(msg_memory_vect)

    def run(self):
        send_start_time = time.time()

        while True:
            # exits upon receiving MSG_STOP
            try:
                if self.queue.get(block=False) == self.MSG_STOP:
                    break
            except Queue.Empty:
                pass

            bytes_in = self.conn.port.inWaiting()

            if bytes_in > 0:
                print(self.conn.port.read(bytes_in))

            loop_start = time.time()
            bytes_to_send = self.datarate

            bytes_to_send -= self._heartbeat_send()
            bytes_to_send -= self._attitude_send()

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
                        self._memory_vect_send(msg_memory_vect)
                        if packet_interval > 0:
                            time.sleep(packet_interval)

            loop_dt = time.time() - loop_start

            # sleep for the rest of second
            if loop_dt < 1 and loop_dt > 0:
                time.sleep(1 - loop_dt)

        self.conn.close()
        self.queue.put(SendResult(
            packets=self.total_packets_sent,
            bytes=self.total_bytes_sent,
            time_seconds=time.time() - send_start_time))


if __name__ == "__main__":

    from pymavlink import mavutil

    conn = mavutil.mavlink_connection("/dev/ttyUSB0,115200")
    q = multiprocessing.Queue()
    p = SendProcess(q, conn, 1000)
    p.start()
    time.sleep(5)

    p.stop()
    # there should be a summary result available in queue
    result = q.get()

    print result
