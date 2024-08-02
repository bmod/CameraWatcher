#include "camerawindow.h"

#include <QApplication>
#include <QMouseEvent>
#include <QScreen>
#include <QTimer>
#include <QtDebug>

MainLayoutWidget::MainLayoutWidget(UsbManager& usbMan) : mUsbMan(usbMan), mCameraListWidget(usbMan) {
    setObjectName("mainLayout");
    setLayout(&mLayout);
    mLayout.setAlignment(Qt::AlignTop);

    mHeader.setText("HEADER");
    mHeader.setObjectName("header");
    mLayout.addWidget(&mHeader);

    mLayout.addWidget(&mCameraListWidget);

    connect(&mUsbMan, &UsbManager::deviceAdded, this, &MainLayoutWidget::updateLabel);
    connect(&mUsbMan, &UsbManager::deviceRemoved, this, &MainLayoutWidget::updateLabel);

    updateLabel();
}

void MainLayoutWidget::updateLabel() {
    mHeader.setText(QString("Connected cameras: %1").arg(mUsbMan.deviceCount()));
}

CameraWindow::CameraWindow(UsbManager& usbMan) : mMainWidget(usbMan), mUsbMan(usbMan) {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::CustomizeWindowHint);

    setCentralWidget(&mMainWidget);

    const auto& screen = QGuiApplication::screenAt(geometry().center());
    move(screen->geometry().center() - geometry().center());
    restoreGeometry(QSettings().value("winGeo", saveGeometry()).toByteArray());

    loadStyle();

    addAction(&mExitAction);
    mExitAction.setShortcut(Qt::Key_Escape);
    connect(&mExitAction, &QAction::triggered, this, &QMainWindow::close);

    connect(&mUsbMan, &UsbManager::deviceAdded, this, &CameraWindow::onListResized);
    connect(&mUsbMan, &UsbManager::deviceRemoved, this, &CameraWindow::onListResized);

}

void CameraWindow::loadStyle() {
    QFile file(":/style.css");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qFatal("Failed to open stylesheet");
    }
    setStyleSheet(file.readAll());
}

void CameraWindow::onListResized() {
    setVisible(static_cast<bool>(mUsbMan.deviceCount()));
    QApplication::processEvents();
    activateWindow();
    QTimer::singleShot(100, [this] {
        adjustSize();
    });
    adjustSize();
}

void CameraWindow::mousePressEvent(QMouseEvent* event) {
    mCursorOffset = pos() - event->globalPos();
    mDragging = true;
}

void CameraWindow::mouseMoveEvent(QMouseEvent* event) {
    if (mDragging)
        move(event->globalPos() + mCursorOffset);
}

void CameraWindow::mouseReleaseEvent(QMouseEvent* event) {
    mDragging = false;
    // QMainWindow::mouseReleaseEvent(event);
    QSettings s;
    s.setValue("winGeo", saveGeometry());
    s.sync();
}
