#include "camerawidget.h"

#include <QtDebug>

#include <QAction>
#include <QSet>
#include <QUuid>
#include <QDateTime>

using namespace CamWatcher;

CameraWidget::CameraWidget(UsbDevice& device) : mDevice(device) {
    setObjectName("cameraWidget");
    setLayout(&mVBoxLayout);
    {
        mLabel.setText(mDevice.name());
        mVBoxLayout.addWidget(&mLabel);

        mDescLabel.setText("Description Here");
        mDescLabel.setObjectName("descLabel");
        mVBoxLayout.addWidget(&mDescLabel);
        mProgressBar.setTextVisible(false);

        mVBoxLayout.addLayout(&mHBoxLayout);
        {
            mHBoxLayout.addWidget(&mProgressBar);
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
    mProgressBar.setVisible(false);

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
    mDescLabel.setText(parm.toString());
}

void CameraWidget::state_Idle(const StateParm& parm) {

    mDescLabel.setText(QString("File count: %1").arg(mDevice.fileCount()));

    if (device().fileCount() == 0) {
        return;
    }

    mLeftButton.setVisible(true);
    mLeftButton.setText("Copy");
    connect(&mLeftButton, &QPushButton::clicked, [this] {
        bool removeOriginals = false;
        setState(UsbDevice::State::VerifyCopy, removeOriginals);
    });

    mRightButton.setVisible(true);
    mRightButton.setText("Move");
    connect(&mRightButton, &QPushButton::clicked, [this] {
        bool removeOriginals = true;
        setState(UsbDevice::State::VerifyCopy, removeOriginals);
    });
}

void CameraWidget::state_VerifyCopy(const StateParm& parm) {
    bool removeOriginals = parm.toBool();
    auto copyOrMove = removeOriginals ? "move" : "copy";
    auto copyOrMoveC = removeOriginals ? "Move" : "Copy";

    mDescLabel.setText(QString("Where to %1 to?").arg(copyOrMove));

    const auto path = ensureDestinationPath();

    mDescLabel.setText(QString("%1 here?\n%2").arg(copyOrMoveC).arg(path));

    mLeftButton.setVisible(true);
    mLeftButton.setText("Yes");
    connect(&mLeftButton, &QPushButton::clicked, [this, removeOriginals] {
        mDevice.usbManager().downloadFiles(mDevice, removeOriginals);
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
    auto stats = parm.value<CopyStats>();
    auto copyingOrMoving = stats.removeOriginals ? "Moving" : "Copying";

    auto msg = QString("%1 file %2/%3").arg(copyingOrMoving).arg(stats.copiedFiles + 1).arg(stats.totalFiles);
    if (stats.copiedKbs > 0) {
        const auto kbsLeft = stats.totalKbs - stats.copiedKbs;
        const auto secondsRemaining = kbsLeft / stats.kbps;
        auto fmtTime = QDateTime::fromTime_t(secondsRemaining).toUTC().toString("hh:mm:ss");
        auto eta = QString(" (ETA %1)").arg(fmtTime);
        msg += eta;

        mProgressBar.setRange(0, stats.totalKbs);
        mProgressBar.setValue(stats.copiedKbs);
    } else {
        mProgressBar.setRange(0, 0);
        mProgressBar.setValue(0);
    }
    mDescLabel.setText(msg);

    mProgressBar.setVisible(true);

    mLeftButton.setText("Cancel");
    mLeftButton.setVisible(true);
    mProgressBar.setVisible(true);
    connect(&mLeftButton, &QPushButton::clicked, [this] {
        mDevice.usbManager().cancelDownload(mDevice);
    });
}

void CameraWidget::state_Done(const StateParm& parm) {
    mDescLabel.setText(parm.toString());
    mLeftButton.setVisible(true);
    mLeftButton.setText("Okay");
    connect(&mLeftButton, &QPushButton::clicked, [this] {
        setState(UsbDevice::Idle);
    });
}

void CameraWidget::state_Error(const StateParm& parm) {
    mDescLabel.setText(parm.toString());
}

void CameraWidget::state_Removed(const StateParm& parm) {}

void CameraWidget::state_Cancel(const StateParm& parm) {
    mDescLabel.setText("Cancelling...");
}
