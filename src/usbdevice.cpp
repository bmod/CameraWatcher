#include "usbdevice.h"
#include "utils.h"

#include <QFileInfo>
#include <QMetaEnum>
#include <QtDebug>

using namespace CamWatcher;

UsbFile::UsbFile(QString filePath, const int kbSize)
    : mFilePath(std::move(filePath)),  mKbSize(kbSize){}


QString UsbFile::filePath() const {
    return mFilePath;
}

int UsbFile::kbSize() const {
    return mKbSize;
}

UsbDevice::UsbDevice(UsbManager& usbManager, const QString& name, const int bus, const int port)
    : mUsbManager(usbManager), mName(name), mBus(bus), mPort(port), mSettingsKey(name), mState(Idle) {}

void UsbDevice::setState(const State state, const StateParm& parm) {
    invokeOnMainThread([this, state, parm] {
        if (mState == state && mStateParm == parm)
            return;

        forceState(state, parm);
    });
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
    const auto msg = QString("[STATE(%1)] %2 (%3)").arg(name(), QMetaEnum::fromType<State>().valueToKey(state), parm.toString());
    qDebug() << msg;

    mState = state;
    mStateParm = parm;
    stateChanged(mState, mStateParm);
}
