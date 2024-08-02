#include <QApplication>

#include "camerawindow.h"
#include "usbmanager.h"

#include <libudev.h>


int main(int argc, char* argv[])
{
    QApplication::setApplicationName("CameraWatcher");
    QApplication::setOrganizationName("CoreSmith");

    QApplication a(argc, argv);

    UsbManager usbManager;

    CameraWindow win(usbManager);
    win.show();

    return QApplication::exec();
}
