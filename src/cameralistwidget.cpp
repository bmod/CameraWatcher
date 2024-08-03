#include "cameralistwidget.h"

using namespace CamWatcher;

CameraListWidget::CameraListWidget(UsbManager& usb) : mUsb(usb) {
    setLayout(&mLayout);
    mLayout.setContentsMargins({});

    connect(&mUsb, &UsbManager::deviceAdded, this, &CameraListWidget::onDeviceAdded);
    connect(&mUsb, &UsbManager::deviceAboutToBeRemoved, this, &CameraListWidget::onDeviceAboutToBeRemoved);

    connect(&mUsb, &UsbManager::deviceAdded, this, &CameraListWidget::listResized);
    connect(&mUsb, &UsbManager::deviceRemoved, this, &CameraListWidget::listResized);

    for (auto& dev: mUsb.devices()) {
        onDeviceAdded(dev.get());
    }
}

void CameraListWidget::onDeviceAdded(UsbDevice* dev) {
    const auto w = new CameraWidget(*dev);
    mLayout.addWidget(w);
    mWidgets.insert(dev, w);
    update();
}

void CameraListWidget::onDeviceAboutToBeRemoved(UsbDevice* dev) {
    mWidgets.take(dev)->deleteLater();
}