#include "camerawidget.h"

#include <QtDebug>

#include <QAction>
#include <QSet>
#include <QUuid>

CameraWidget::CameraWidget(UsbDevice& device) : mDevice(device) {
    setObjectName("cameraWidget");
    setLayout(&mVBoxLayout);
    {
        mLabel.setText(mDevice.name());
        mVBoxLayout.addWidget(&mLabel);

        mDescLabel.setText("Description Here");
        mDescLabel.setObjectName("descLabel");
        mVBoxLayout.addWidget(&mDescLabel);

        mVBoxLayout.addLayout(&mHBoxLayout);
        {
            mHBoxLayout.addWidget(&mLeftButton);
            mHBoxLayout.addWidget(&mMiddleButton);
            mHBoxLayout.addWidget(&mRightButton);
        }
    }

    mStateHandlers = {
            {UsbDevice::State::Init, std::bind(&CameraWidget::state_Init, this, std::placeholders::_1)},
            {UsbDevice::State::Idle, std::bind(&CameraWidget::state_Idle, this, std::placeholders::_1)},
            {UsbDevice::State::VerifyCopy, std::bind(&CameraWidget::state_VerifyCopy, this, std::placeholders::_1)},
            {UsbDevice::State::StartCopy, std::bind(&CameraWidget::state_StartCopy, this, std::placeholders::_1)},
            {UsbDevice::State::Copy, std::bind(&CameraWidget::state_Copy, this, std::placeholders::_1)},
            {UsbDevice::State::Done, std::bind(&CameraWidget::state_Done, this, std::placeholders::_1)},
            {UsbDevice::State::Error, std::bind(&CameraWidget::state_Error, this, std::placeholders::_1)},
            {UsbDevice::State::Removed, std::bind(&CameraWidget::state_Removed, this, std::placeholders::_1)},
            {UsbDevice::State::Cancel, std::bind(&CameraWidget::state_Cancel, this, std::placeholders::_1)},
    };

    connect(&mDevice, &UsbDevice::stateChanged, this, &CameraWidget::onDeviceStateChanged);
    onDeviceStateChanged(UsbDevice::State::Init);
}

UsbDevice& CameraWidget::device() const {
    return mDevice;
}

void CameraWidget::onDeviceStateChanged(const UsbDevice::State& state, const StateParm& parm) {
    mLeftButton.setVisible(false);
    mMiddleButton.setVisible(false);
    mRightButton.setVisible(false);

    mLeftButton.disconnect();
    mMiddleButton.disconnect();
    mRightButton.disconnect();

    mLabel.setText(mDevice.name());

    const auto& handler = mStateHandlers[state];
    handler(parm);
}

void CameraWidget::setState(const UsbDevice::State& state, const StateParm& parm) const {
    mDevice.setState(state, parm);
}

void CameraWidget::resetState() {
    mDevice.resetState();
}

QString CameraWidget::ensureDestinationPath(bool forcePrompt) {
    auto filePath = mDevice.destFilePath();
    if (filePath.isEmpty() || !QFileInfo::exists(filePath) || forcePrompt) {

        if (filePath.isEmpty())
            filePath = QDir::homePath();

        filePath = QFileDialog::getExistingDirectory(this, "Select Destination Directory", filePath);

        if (filePath.isEmpty())
            filePath = QDir::homePath();

        mDevice.setDestFilePath(filePath);
    }
    return filePath;
}



void CameraWidget::state_Init(const StateParm& parm) {
    mDescLabel.setText(parm);
    mLeftButton.setVisible(false);
    mRightButton.setVisible(false);
}

void CameraWidget::state_Idle(const StateParm& parm) {

    mDescLabel.setText(QString("File count: %1").arg(mDevice.fileCount()));

    if (device().fileCount() == 0) {
        mLeftButton.setVisible(false);
        mRightButton.setVisible(false);
        return;
    }

    mLeftButton.setVisible(true);
    mLeftButton.setText("Copy");
    connect(&mLeftButton, &QPushButton::clicked, [this] {
        setState(UsbDevice::State::VerifyCopy);
    });
    mRightButton.setVisible(false);
}

void CameraWidget::state_VerifyCopy(const StateParm& parm) {
    mDescLabel.setText("Where to copy to?");

    const auto path = ensureDestinationPath();

    mDescLabel.setText("Copy here? <b>" + path + "</b>");

    mLeftButton.setVisible(true);
    mLeftButton.setText("Yes");
    connect(&mLeftButton, &QPushButton::clicked, [this] {
        mDevice.usbManager().downloadFiles(mDevice);
    });

    mMiddleButton.setVisible(true);
    mMiddleButton.setText("No");
    connect(&mMiddleButton, &QPushButton::clicked, [this] {
        mDevice.setDestFilePath({});
        resetState();
    });

    mRightButton.setVisible(true);
    mRightButton.setText("Cancel");
    connect(&mRightButton, &QPushButton::clicked, [this] { setState(UsbDevice::Idle); });
}

void CameraWidget::state_StartCopy(const StateParm& parm) {}

void CameraWidget::state_Copy(const StateParm& parm) {
    mDescLabel.setText(parm);
    mLeftButton.setText("Cancel");
    mLeftButton.setVisible(true);
    connect(&mLeftButton, &QPushButton::clicked, [this] {
        mDevice.usbManager().cancelDownload(mDevice);
    });
}

void CameraWidget::state_Done(const StateParm& parm) {
    mDescLabel.setText(parm);
    mLeftButton.setVisible(true);
    mLeftButton.setText("Okay");
    connect(&mLeftButton, &QPushButton::clicked, [this] {
        setState(UsbDevice::Idle);
    });
}

void CameraWidget::state_Error(const StateParm& parm) {
    mDescLabel.setText(parm);
}

void CameraWidget::state_Removed(const StateParm& parm) {}

void CameraWidget::state_Cancel(const StateParm& parm) {
    mDescLabel.setText("Cancelling...");
}