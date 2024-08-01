import logging
import os
import sys

from PySide6.QtCore import *
from PySide6.QtGui import *
from PySide6.QtWidgets import *

from tests.camerawatcher import usb

logging.basicConfig(level=logging.DEBUG)
LOG = logging.getLogger(__name__)

FILEPATH_STYLE = f'{os.path.dirname(__file__)}/style.css'
IMAGE_EXTENSIONS = {'png', 'jpg', 'jpeg', 'gif', '3fr',
                    'ari', 'arw', 'bay', 'braw', 'cri', 'crw', 'cap', 'dcs', 'dng', 'erf', 'fff', 'gpr', 'jxs', 'mef',
                    'mdc', 'mos', 'mrw', 'nef', 'orf', 'pef', 'pxn', 'R3D', 'raf', 'raw', 'raw', 'rwz', 'srw', 'tco',
                    'x3f'}
VIDEO_EXTENSIONS = {'webm', 'mkv', 'flv', 'vob', 'ogv', 'ogg', 'rrc', 'gifv', 'mng', 'mov', 'avi', 'qt', 'wmv', 'yuv',
                      'rm', 'asf', 'amv', 'mp4', 'm4p', 'm4v', 'mpg', 'mp2', 'mpeg', 'mpe', 'mpv', 'm4v', 'svi', '3gp',
                      '3g2', 'mxf', 'roq', 'nsv', 'flv', 'f4v', 'f4p', 'f4a', 'f4b', 'mod'}
ALLOWED_EXTENSIONS = IMAGE_EXTENSIONS | VIDEO_EXTENSIONS

class CameraWidget(QFrame):

    def __init__(self, device: usb.UsbDevice):
        super().__init__()
        self.setObjectName('cameraWidget')
        self._device = device  # type: usb.UsbDevice
        self._vbox = QVBoxLayout()
        self.setLayout(self._vbox)

        self._label = QLabel(self._device.name())
        self._vbox.addWidget(self._label)

        self._descLabel = QLabel(f'Retrieving info...')
        self._descLabel.setObjectName('descLabel')
        self._vbox.addWidget(self._descLabel)

        self._hbox = QHBoxLayout()
        self._vbox.addLayout(self._hbox)

        self._btLeft = QPushButton()
        self._btLeft.clicked.connect(lambda: ...)  # to avoid a disconnect warning later
        self._hbox.addWidget(self._btLeft)

        self._btRight = QPushButton()
        self._btRight.clicked.connect(lambda: ...)  # to avoid a disconnect warning later
        self._hbox.addWidget(self._btRight)

        self._stateHandlers = {
            usb.DevState.Idle: self._state_idle,
            usb.DevState.Init: self._state_init,
            usb.DevState.VerifyCopy: self._state_verifyCopy,
            usb.DevState.StartCopy: self._state_startCopy,
            usb.DevState.Copy: self._state_copy,
            usb.DevState.Done: self._state_done,
            usb.DevState.Error: self._state_error,
            usb.DevState.Removed: self._state_removed,
        }

        self._onDeviceStateChanged(device.state(), device.stateParm())
        self._device.stateChanged.connect(self._onDeviceStateChanged)

    def _state_idle(self, msg):
        num = len(list(self.copyableFileIndexes()))
        if num:
            self._descLabel.setText(f'Found {num} files')
            self._btLeft.setText('Copy')
            self._btLeft.clicked.connect(lambda: self._setState(usb.DevState.VerifyCopy))
            self._btRight.setText('Move')
            self._btRight.clicked.connect(lambda: self._setState(usb.DevState.VerifyMOve))
        else:
            self._descLabel.setText('No files found')
            self._btLeft.setVisible(False)
            self._btRight.setVisible(False)

    def _state_init(self, parms: list = None):
        self._descLabel.setText('Initializing...')
        self._btLeft.setVisible(False)
        self._btRight.setVisible(False)

    def _state_verifyCopy(self, parm):
        self._descLabel.setText(f'Where to copy to?')

        path = self._ensureDestinationPath()  # may prompt user

        self._descLabel.setText(f'Copy to: "{path}"?')

        self._btLeft.setText(f'Start')
        self._btLeft.clicked.connect(lambda: self._setState(usb.DevState.StartCopy))

        self._btRight.setText(f'Cancel')
        self._btRight.clicked.connect(lambda: self._setState(usb.DevState.Idle))

    def _state_startCopy(self, parm):
        indexes = list(self.copyableFileIndexes())
        usb.copyFiles(self._device, indexes)

    def _state_copy(self, parm):
        self._descLabel.setText(parm)

        self._btLeft.setText(f'...')
        self._btLeft.setVisible(False)

        self._btRight.setText(f'Stop')
        self._btRight.clicked.connect(lambda: self._setState(usb.DevState.Idle))

    def _state_done(self, parm):
        self._descLabel.setText(parm)
        self._btLeft.setVisible(False)
        self._btRight.setVisible(False)

    def _state_error(self, parms: list = None):
        self._descLabel.setText(f'Error: {parms}')
        self._btLeft.setVisible(False)
        self._btRight.setVisible(False)

    def _state_removed(self, parms: list = None):
        self._descLabel.setText(f'Removed')

    def _onDeviceStateChanged(self, state: usb.DevState, parm=None):
        self._btLeft.setVisible(True)
        self._btRight.setVisible(True)

        self._btLeft.clicked.disconnect()
        self._btRight.clicked.disconnect()

        self._label.setText(f'{self._device.name()}')

        handler = self._stateHandlers[state]
        handler(parm)

    def _setState(self, state: usb.DevState):
        self._device.setState(state)

    def _ensureDestinationPath(self):
        filePath = self._device.destinationFilePath()
        if not filePath or not os.path.exists(filePath):
            filePath = QFileDialog.getExistingDirectory(self, 'Select Destination Directory', filePath)
            self._device.setDestinationFilePath(filePath)
        return filePath

    def device(self) -> usb.UsbDevice:
        return self._device

    def copyableFileIndexes(self):
        for i, f in enumerate(self._device.filePaths()):
            baseName = os.path.basename(f)
            if not '.' in baseName:
                continue

            ext = os.path.splitext(baseName)[1][1:].lower()
            if ext not in ALLOWED_EXTENSIONS:
                continue

            yield i + 1 # gphoto2 indexing starts at 1


class CameraListWidget(QWidget):
    listResized = Signal()

    def __init__(self):
        super().__init__()
        self.setLayout(QVBoxLayout())
        self.layout().setSpacing(30)
        self.layout().setContentsMargins(0, 0, 0, 0)

        self._widgets = {}

        self._usb = usb.usbMan()
        self._usb.deviceAdded.connect(self._onDeviceAdded)
        self._usb.deviceRemoved.connect(self._onDeviceRemoved)

        self._usb.deviceAdded.connect(self.listResized)
        self._usb.deviceRemoved.connect(self.listResized)

        for dev in self._usb.devices():
            self._onDeviceAdded(dev)

    def _onDeviceAdded(self, device):
        w = CameraWidget(device)
        self.layout().addWidget(w)
        self.update()
        self._widgets[device] = w

    def _onDeviceRemoved(self, device):
        w = self._widgets[device]
        del self._widgets[device]
        w.deleteLater()


class MainLayoutWidget(QFrame):
    """Just a container to keep the list and title separate"""

    listResized = Signal()

    def __init__(self):
        super().__init__()
        self.setObjectName('mainLayout')
        self.setLayout(QVBoxLayout())
        self._nameButton = QLabel()
        self._nameButton.setObjectName('header')
        self.layout().addWidget(self._nameButton)
        self._camWidget = CameraListWidget()
        self.layout().addWidget(self._camWidget)

        self._usb = usb.usbMan()
        self._usb.deviceAdded.connect(self._updateLabel)
        self._usb.deviceUpdated.connect(self._updateLabel)
        self._usb.deviceRemoved.connect(self._updateLabel)

        self._updateLabel()

    def _updateLabel(self):
        self._labelText = "Connected Cameras"
        txt = 'Connected Camera' + ('s' if len(usb.usbMan().devices()) > 1 else '')
        self._nameButton.setText(txt)
        self.listResized.emit()


class CameraWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setAttribute(Qt.WA_TranslucentBackground)
        self.setWindowFlags(Qt.CustomizeWindowHint)
        self._cursorOffset = None

        self._mainWidget = MainLayoutWidget()
        self._mainWidget.listResized.connect(self._onListResized)

        self.setCentralWidget(self._mainWidget)
        screen = QGuiApplication.screenAt(self.geometry().center())
        self.move(screen.geometry().center() - self.geometry().center())

        self._styleFile = FILEPATH_STYLE
        assert self._styleFile, f'File not found: {self._styleFile}'
        self._styleFileWatch = QFileSystemWatcher(self)
        self._styleFileWatch.addPath(self._styleFile)
        self._styleFileWatch.fileChanged.connect(self._onStyleFileChanged)
        self._onStyleFileChanged()

    def _onListResized(self):
        # Somehow, this magic cocktail actually packs the window, leave it be
        QApplication.processEvents()
        self.activateWindow()
        QTimer.singleShot(10, self.adjustSize)
        self.adjustSize()

    def _onStyleFileChanged(self, *_):
        try:
            with open(self._styleFile, 'r') as fp:
                styleSheet = fp.read()
            self.setStyleSheet(styleSheet)
        except Exception as e:
            LOG.error(f'Error updating stylesheet: {e}')

    def mousePressEvent(self, event):
        self._cursorOffset = self.pos() - event.globalPosition().toPoint()

    def mouseMoveEvent(self, event):
        if self._cursorOffset is not None:
            pos = event.globalPosition().toPoint()
            self.move(pos + self._cursorOffset)

    def mouseReleaseEvent(self, event):
        self._cursorOffset = None
        super().mouseReleaseEvent(event)


APP = None
win = None


def main():
    global APP
    global win

    QApplication.setApplicationName('CamWatcher')
    QApplication.setOrganizationName('CoreSmith')

    APP = QApplication(sys.argv)

    QFontDatabase.addApplicationFont(f'{os.path.dirname(__file__)}/DMMono-Light.ttf')
    QFontDatabase.addApplicationFont(f'{os.path.dirname(__file__)}/DMMono-Regular.ttf')
    QFontDatabase.addApplicationFont(f'{os.path.dirname(__file__)}/DMMono-Medium.ttf')

    win = CameraWindow()
    win.show()

    APP.exec()

    usb.usbMan().close()


if __name__ == '__main__':
    targetPath = '~/Documents/CamWatcher_Import'
    main()
