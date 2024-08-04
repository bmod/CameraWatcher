#include <QApplication>

#include "camerawindow.h"
#include "usbmanager.h"

#include <QFontDatabase>
#include <libudev.h>


int main(int argc, char* argv[]) {
    QApplication::setApplicationName("CameraWatcher");
    QApplication::setOrganizationName("CoreSmith");


    QApplication a(argc, argv);

    QFontDatabase::addApplicationFont(":/DMMono-Light.ttf");
    QFontDatabase::addApplicationFont(":/DMMono-Medium.ttf");
    QFontDatabase::addApplicationFont(":/DMMono-Regular.ttf");

    CamWatcher::UsbManager usbManager;

    CamWatcher::CameraWindow win(usbManager);
    win.show();

    return QApplication::exec();
}
