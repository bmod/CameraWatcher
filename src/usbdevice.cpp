#include "usbdevice.h"

#include <QFileInfo>
#include <QMetaEnum>
#include <QtDebug>

UsbFile::UsbFile(const int gPhotoIndex, QString filePath, QString flags, const int kbSize, QString mediaType,
                 const int timeStamp)
    : mGPhotoIndex(gPhotoIndex), mFilePath(std::move(filePath)), mFlags(std::move(flags)), mKbSize(kbSize),
      mMediaType(std::move(mediaType)), mTimeStamp(timeStamp) {}

int UsbFile::gPhotoIndex() const {
    return mGPhotoIndex;
}
QString UsbFile::filePath() const {
    return mFilePath;
}
QString UsbFile::flags() const {
    return mFlags;
}
int UsbFile::kbSize() const {
    return mKbSize;
}
QString UsbFile::mediaType() const {
    return mMediaType;
}
int UsbFile::timeStamp() const {
    return mTimeStamp;
}

UsbDevice::UsbDevice(UsbManager& usbManager, const QString& name, const int bus, const int port)
    : mUsbManager(usbManager), mName(name), mBus(bus), mPort(port), mSettingsKey(name), mState(Idle) {}

void UsbDevice::setState(const State state, const StateParm& parm) {
    if (mState == state && mStateParm == parm)
        return;

    forceState(state, parm);
}

void UsbDevice::resetState() {
    forceState(mState, mStateParm);
}

const QString& UsbDevice::name() const {
    return mName;
}

int UsbDevice::bus() const {
    return mBus;
}

int UsbDevice::port() const {
    return mPort;
}

const UsbDevice::State& UsbDevice::state() const {
    return mState;
}

const StateParm& UsbDevice::stateParm() const {
    return mStateParm;
}

const QVector<UsbFile>& UsbDevice::files() const {
    return mFiles;
}

void UsbDevice::setFiles(const QVector<UsbFile>& filePaths) {
    mFiles = filePaths;
}

int UsbDevice::fileCount() const {
    return files().size();
}

QString UsbDevice::destFilePath() const {
    QSettings s;
    s.beginGroup(mSettingsKey);
    const auto value = s.value("destPath").toString();
    s.endGroup();
    return value;
}

void UsbDevice::setDestFilePath(const QString& path) const {
    QSettings s;
    s.beginGroup(mSettingsKey);
    s.setValue("destPath", path);
    s.endGroup();
    s.sync();
}

UsbManager& UsbDevice::usbManager() const {
    return mUsbManager;
}

void UsbDevice::forceState(const State state, const StateParm& parm) {
    const auto msg = QString("[STATE(%1)] %2 (%3)").arg(name(), QMetaEnum::fromType<State>().valueToKey(state), parm);
    qDebug() << msg;

    mState = state;
    mStateParm = parm;
    stateChanged(mState, mStateParm);
}
