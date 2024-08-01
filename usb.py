import dataclasses
import enum
import logging
import re
import subprocess
import threading
import time

from PySide6.QtCore import *
from PySide6.QtGui import *
from PySide6.QtWidgets import *

LOG = logging.getLogger(__name__)

EVT_REMOVE = 'remove'
EVT_UPDATE = 'update'
EVT_ADDED = 'update'

PAT_BUS = r'usb\W(\d+-\d+):\W'
PAT_DEVICE_ADD = PAT_BUS + 'New USB device found'
PAT_DEVICE_REMOVE = PAT_BUS + 'USB disconnect'

_USB_MANAGER = None


class DevState(enum.Enum):
    Idle = 'Idle'
    Init = 'Initializing'
    VerifyCopy = 'VerifyCopy'
    VerifyMOve = 'VerifyMove'
    Copy = 'Copy'
    StartCopy = 'StartCopy'
    Move = 'Moving'
    Done = 'Done'
    Error = 'Error'
    Removed = 'Removed'



class UsbDevice(QObject):
    stateChanged = Signal(object, object)

    def __init__(self, name: str, bus: int = -1, port: int = -1):
        super().__init__()
        self._name = name
        self._bus = bus
        self._port = port
        self._filePaths = []
        self._state = DevState.Idle
        self._stateParm = None

    def setState(self, state, parm=None):
        # ignore = {DevState.STATE_COPYING, DevState.STATE_MOVING, DevState.STATE_IDLE}
        # if (state not in ignore) and (self._state == state):
        #     return
        self._state = state
        self._stateParm = parm
        LOG.info(f'Dev state "{self._state}" ({self._name})')
        self.stateChanged.emit(self._state, parm)

    def state(self):
        return self._state

    def stateParm(self):
        return self._stateParm

    def name(self):
        return self._name

    def bus(self):
        return self._bus

    def port(self):
        return self._port

    def portPath(self):
        return f'usb:{self.bus():03},{self.port():03}'

    def destinationFilePath(self):
        s = QSettings()
        return s.value('destinationPath', None)

    def setDestinationFilePath(self, filePath):
        s = QSettings()
        s.setValue('destinationPath', filePath)
        s.sync()

    def filePaths(self):
        return self._filePaths

    def setFilePaths(self, filePaths):
        self._filePaths = filePaths

    def fileCount(self):
        return len(self.filePaths())

    def __eq__(self, other):
        return self.portPath() == other.portPath()

    def __hash__(self):
        return hash(self.portPath())

    def __repr__(self):
        return str(self.__dict__)


def runSync(cmd, cwd=None):
    cmdStr = ' '.join(cmd)
    LOG.info(f'Running: {cmdStr} (cwd: {cwd})')
    proc = subprocess.Popen(cmd, cwd=cwd, encoding='utf-8', stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if err:
        raise Exception(err)

    return out.splitlines()


def runGen(cmd, cwd=None):
    cmdStr = ' '.join(cmd)
    print(f'Running: {cmdStr}')
    proc = subprocess.Popen(cmd, cwd=cwd, encoding='utf-8', stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def _readPipe(pipe):
        ln = proc.stdout.readLine()
        if ln == b'' and proc.poll() is not None:
            return
        if ln:
            yield ln.decode('utf-8')

    while True:
        outLn = _readPipe(proc.stdout)
        errLn = _readPipe(proc.stderr)

        if (not outLn) and (not errLn):
            return

        yield outLn, errLn

        output = proc.stdout.readLine()
        if output == b'' and proc.poll() is not None:
            return
        if output:
            yield output.decode('utf-8')


def connectedDevices():
    pat = re.compile(r'(.+)usb:(\d+),(\d+)')

    for ln in runSync(['gphoto2', '--auto-detect', '--parsable']):
        match = pat.match(ln)
        if not match:
            continue
        deviceName = match.group(1).strip()
        bus = int(match.group(2))
        port = int(match.group(3))

        yield UsbDevice(deviceName, bus, port)


def listFiles(device, callback):
    device.setState(DevState.Init)
    threading.Thread(target=listFiles_thread, args=(device, callback)).start()


def listFiles_thread(dev, callback):
    try:
        # time.sleep(2)  # TODO: Remove this fake delay
        lines = runSync(['gphoto2', '--list-files', '-q', '--port', dev.portPath()])
        callback(dev, lines)
    except Exception as e:
        LOG.warning(e)


def copyFiles(device, fileIndexes):
    threading.Thread(target=copyFiles_thread, args=[device, fileIndexes]).start()


def copyFiles_thread(dev: UsbDevice, fileIndexes):
    total = len(fileIndexes)
    succeeded = []
    failed = []
    for i, idx in enumerate(fileIndexes):
        dev.setState(DevState.Copy, f'Copying {i + 1} / {total}')
        try:
            lines = runSync(['gphoto2', f'--get-file={idx}', '--force-overwrite', f'--port={dev.portPath()}'], cwd=dev.destinationFilePath())
            succeeded.append(idx)
        except Exception as e:
            LOG.warning(e)
            failed.append(idx)

    if failed:
        dev.setState(DevState.Error, f'{len(failed)} files failed to copy..')
    else:
        dev.setState(DevState.Done, f'{len(succeeded)} files copied successfully')

def usbMan():
    global _USB_MANAGER
    if not _USB_MANAGER:
        _USB_MANAGER = _UsbManager(QApplication.instance())
    return _USB_MANAGER


class _UsbManager(QObject):
    deviceAdded = Signal(object)
    deviceUpdated = Signal(object)
    deviceRemoved = Signal(object)

    _messageReceived = Signal(str)  # Route messages to main thread

    def __init__(self, parent):
        super().__init__(parent)
        self._devices = []

        self._processingMessages = True
        self._messageReceived.connect(self._onMessageReceived, Qt.ConnectionType.QueuedConnection)
        self.refresh()
        self._listenForEvents()

    def close(self):
        self._processingMessages = False

    def _listenForEvents(self):
        self._processingMessages = True
        self._thread = threading.Thread(target=self._processMessagesThreaded, daemon=True)
        self._thread.start()

    def devices(self):
        return self._devices

    def refresh(self):
        connected = list(connectedDevices())
        for dev in connected:
            if dev not in self._devices:
                self._addDevice(dev)

        for oldDev in self._devices:
            if oldDev not in connected:
                self._removeDevice(oldDev)

    def _addDevice(self, device):
        self._devices.append(device)
        self.deviceAdded.emit(device)
        device.setState(DevState.Idle, 'Discovering files..')
        listFiles(device, self._onFilesRetrieved)

    def _onFilesRetrieved(self, dev, files):
        dev.setFilePaths(files)
        dev.setState(DevState.Idle, f'Found {len(files)} files')
        self.deviceUpdated.emit(dev)

    def _removeDevice(self, dev):
        self._devices.remove(dev)
        self.deviceRemoved.emit(dev)

    def _processMessagesThreaded(self):
        proc = subprocess.Popen(['journalctl', '--follow'],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        for line in iter(proc.stdout.readline, ''):
            if self._processingMessages:
                line = line.decode('utf-8').rstrip()
                self._messageReceived.emit(line)

    def _onMessageReceived(self, line):
        if self._isUsbEvent(line):
            self.refresh()

    def _isUsbEvent(self, ln):
        if re.search(PAT_DEVICE_ADD, ln):
            return EVT_ADDED
        if re.search(PAT_DEVICE_REMOVE, ln):
            return EVT_REMOVE
