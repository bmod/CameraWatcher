import json
import os
import re
import subprocess

patIdLine = re.compile(r'Bus\W+(\d+)\W+Device\W(\d+):\W+ID\W+([0-9|a-f]+:[0-9|a-f]+)\W(.+)')
patLabel = re.compile(r'(.+):')
patByte = re.compile(r'b\\p{Lu}(\w+)\W+(\d+)\W+?([^\n]*)')
patId = re.compile(r'id\\p{Lu}(\w+)\W+(0x[0-9a-f]+)\W+?([^\n]*)')
patBcd = re.compile(r'bcd\\p{Lu}(\w+)\W+(\d+(:.\d+)?)\W+?([^\n]*)')
patInt = re.compile(r'id\\p{Lu}(\w+)\W+(0x[0-9a-f]+)\W+?([^\n]*)')


# TODO: There's more...


def _readLsbUsbOutput():
    with open(f'{os.path.dirname(__file__)}/lsusboutput.txt', 'r') as fp:
        data = fp.read()

    devices = []

    curr = None
    for line in data.splitlines():
        ln = line.rstrip()
        if not ln:
            continue

        indent = (len(ln) - len(line.lstrip())) / 2

        ln = ln.strip()

        value = None

        matchIdLine = patIdLine.match(ln)
        if matchIdLine:
            curr = {
                'bus': int(matchIdLine.group(1)),
                'device': int(matchIdLine.group(2)),
                'uid': matchIdLine.group(3),
                'vendor': matchIdLine.group(4),
            }
            devices.append(curr)
            continue

        matchLabel = patLabel.match(ln)
        if matchLabel:
            labelName = matchLabel.group(1)
            # change parent
            child = {}
            curr[labelName] = child
            curr = child
            continue
        #
        # matchInt = patByte.match(ln)
        # if matchInt:
        #     value = int(matchInt.group(1))
        #     curr.setdefault()
        #     continue
        #
        # matchString = patId.match(ln)
        # if matchString:
        #     value = int(matchString.group(1))

    print(json.dumps(devices, indent=2))


def testPyUsb():
    import usb.core
    import usb.util
    for dev in usb.core.find(find_all=True):
        product = None
        try:
            serial = dev.serial_number
            manufacturer = dev.manufacturer
            product = dev.product
        except ValueError as e:
            pass

        if product:
            print(f'Product: {product}, Vendor: {manufacturer}, Serial: {serial}')


def anotherTest():
    import pyudev
    import psutil

    context = pyudev.Context()

    removable = [device for device in context.list_devices(subsystem='block', DEVTYPE='disk') if
                 device.attributes.asstring('removable') == "1"]
    for device in removable:
        partitions = [device.device_node for device in
                      context.list_devices(subsystem='block', DEVTYPE='partition', parent=device)]
        print("All removable partitions: {}".format(", ".join(partitions)))
        print("Mounted removable partitions:")
        for p in psutil.disk_partitions():
            if p.device in partitions:
                print("  {}: {}".format(p.device, p.mountpoint))


def connectedDevices():
    proc = subprocess.Popen(['gphoto2', '--auto-detect', '--parsable'], encoding='utf-8',
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if err:
        raise Exception(err)

    pat = re.compile(r'(.+)usb:(\d+),(\d+)')
    for ln in out.splitlines():
        match = pat.match(ln)
        if not match:
            continue
        deviceName = match.group(1).strip()
        bus = int(match.group(2))
        port = int(match.group(3))

        print(f'Device: {deviceName}, bus: {bus}, port: {port}')



if __name__ == '__main__':
    test3()
    # anotherTest()
    # testPyUsb()
    # _readLsbUsbOutput()
