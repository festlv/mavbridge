import subprocess

IFACE = "wifi"


class Nmcli:
    exe = None

    def __init__(self, nmcli_executable="/usr/bin/nmcli"):
        self.exe = nmcli_executable

    def force_rescan(self):
        return subprocess.call([self.exe, "device", IFACE, "rescan"]) == 0

    def list_networks(self):
        """
        Returns a list of wireless networks.

        Command:
        nmcli -t  -f SSID dev wifi

        """
        output = subprocess.check_output([self.exe, "-t",
                                          "-f", "SSID", "dev", IFACE])
        return output.splitlines()

    def connect(self, ssid, password=""):
        """

        Connects to specified network using password (if provided)

        """

        cmd = [self.exe, "dev", IFACE, "connect", ssid]
        if password:
            cmd.extend(["password", password])

        return subprocess.call(cmd) == 0

    def wifi_ssid(self):
        """
        Returns currently connected WiFi SSID.

        nmcli -t -f TYPE,CONNECTION dev status
        """

        cmd = [self.exe, "-t", "-f", "TYPE,CONNECTION", "dev", "status"]

        op = subprocess.check_output(cmd)

        for l in op.splitlines():
            if l.startswith(IFACE):
                iface, ssid = l.split(":")
                if ssid != "--":
                    return ssid

        return None

if __name__ == "__main__":
    n = Nmcli()

    n.force_rescan()

    networks = n.list_networks()

    for net in networks:

        print "SSID: %s" % net
        if net.startswith("MavBridge"):
            print "^ connecting"
            n.connect(net)

    print n.wifi_ssid()
