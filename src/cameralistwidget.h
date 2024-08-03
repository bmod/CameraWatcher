#pragma once

#include "camerawidget.h"
#include "usbdevice.h"
#include "usbmanager.h"

namespace CamWatcher {
    class CameraListWidget final : public QWidget {
        Q_OBJECT
    public:
        explicit CameraListWidget(UsbManager& usb);
    Q_SIGNALS:
        void listResized();

    private:
        void onDeviceAdded(UsbDevice* dev);
        void onDeviceRemoved();
        void onDeviceAboutToBeRemoved(UsbDevice* dev);

        UsbManager& mUsb;
        QVBoxLayout mLayout;
        QMap<UsbDevice*, CameraWidget*> mWidgets;
    };
}