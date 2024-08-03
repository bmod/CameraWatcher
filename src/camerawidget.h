#pragma once

#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QProgressBar>

#include "usbdevice.h"
#include "usbmanager.h"

namespace CamWatcher {

    class CameraWidget final : public QFrame {
        Q_OBJECT
    public:
        explicit CameraWidget(UsbDevice& device);
        UsbDevice& device() const;

    private:
        void onDeviceStateChanged(const UsbDevice::State& state, const StateParm& parm = {});
        void setState(const UsbDevice::State& state, const StateParm& parm = {}) const;
        void resetState();
        QString ensureDestinationPath(bool forcePrompt = false);

        void state_Init(const StateParm& parm);
        void state_Idle(const StateParm& parm);
        void state_VerifyCopy(const StateParm& parm);
        void state_StartCopy(const StateParm& parm);
        void state_Copy(const StateParm& parm);
        void state_Done(const StateParm& parm);
        void state_Error(const StateParm& parm);
        void state_Removed(const StateParm& parm);
        void state_Cancel(const StateParm& parm);

        UsbDevice& mDevice;
        QVBoxLayout mVBoxLayout;
        QLabel mLabel;
        QLabel mDescLabel;
        QHBoxLayout mHBoxLayout;
        QPushButton mLeftButton;
        QPushButton mMiddleButton;
        QPushButton mRightButton;
        QProgressBar mProgressBar;

        QMap<UsbDevice::State, std::function<void(const StateParm&)>> mStateHandlers;
    };
}