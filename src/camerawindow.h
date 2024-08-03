#pragma once

#include "cameralistwidget.h"
#include "usbmanager.h"

#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QAction>

namespace CamWatcher {

    class MainLayoutWidget final : public QFrame {
        Q_OBJECT
    public:
        explicit MainLayoutWidget(UsbManager& usbMan);

    private:
        void updateLabel();

        UsbManager& mUsbMan;
        QVBoxLayout mLayout;
        QLabel mHeader;
        CameraListWidget mCameraListWidget;
    };

    class CameraWindow final : public QMainWindow {
    public:
        explicit CameraWindow(UsbManager& usbMan);

    private:
        void loadStyle();
        void onListResized();

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

    private:
        bool mDragging = false;
        UsbManager& mUsbMan;
        QPoint mCursorOffset;
        MainLayoutWidget mMainWidget;

        QAction mExitAction;
    };

}