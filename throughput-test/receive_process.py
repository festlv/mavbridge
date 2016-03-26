import multiprocessing
import time
import collections
import Queue

MSG_STOP = 0x00

ReceiveResult = collections.namedtuple(
    'ReceiveResult',
    ['valid_packets', 'malformed_packets',
     'valid_bytes', 'time_seconds'])


class ReceiveProcess(multiprocessing.Process):
    """
    Receiving process. Sample usage:
        conn = mavutil.mavlink_connection("udp:192.168.4.2:14550")
        q = multiprocessing.Queue()
        receive_process = ReceiveProcess(q, conn)
        receive_process.start()
        # after some time

        receive_process.stop()
        # there should be a summary result available in queue
        result = q.get()
    """

    # queue to post results to caller
    queue = None

    # mavutil.mavlink_connection to listen for packets
    conn = None

    def __init__(self, queue, mavlink_connection):
        super(ReceiveProcess, self).__init__()
        self.queue = queue
        self.conn = mavlink_connection

    def stop(self):
        """ Gracefully stop the subprocess """

        self.queue.put(MSG_STOP)
        self.join()

    def run(self):
        valid_packets_received = 0
        malformed_packets_received = 0

        valid_bytes_received = 0
        start_time = time.time()

        while True:
            # exits upon receiving MSG_STOP
            try:
                if self.queue.get(block=False) == MSG_STOP:
                    break
            except Queue.Empty:
                pass

            m = self.conn.recv_msg()
            if m:
                try:
                    buf = m.pack(self.conn.mav)

                    valid_packets_received += 1
                    valid_bytes_received += len(buf)

                except TypeError:
                    malformed_packets_received += 1

                # if this was a heartbeat, send it back to keep connection alive
                try:
                    if m.id == 0:
                        self.conn.write(buf)
                except AttributeError:
                    pass

        time_delta = time.time() - start_time

        # communicate result back via queue

        self.conn.close()
        self.queue.put(ReceiveResult(
            valid_packets=valid_packets_received,
            malformed_packets=malformed_packets_received,
            valid_bytes=valid_bytes_received,
            time_seconds=time_delta))

if __name__ == "__main__":

    from pymavlink import mavutil

    conn = mavutil.mavlink_connection("udp:192.168.4.2:14550")
    q = multiprocessing.Queue()
    receive_process = ReceiveProcess(q, conn)
    receive_process.start()
    time.sleep(5)

    receive_process.stop()
    # there should be a summary result available in queue
    result = q.get()
    print result
