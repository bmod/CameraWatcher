#pragma once


#include <QSettings>
#include <utility>


using StateParm = QString;

class UsbManager;

class UsbFile {
public:
    UsbFile(int gPhotoIndex, QString filePath, QString flags, int kbSize, QString mediaType, int timeStamp);
    [[nodiscard]] int gPhotoIndex() const;
    [[nodiscard]] QString filePath() const;
    [[nodiscard]] QString flags() const;
    [[nodiscard]] int kbSize() const;
    [[nodiscard]] QString mediaType() const;
    [[nodiscard]] int timeStamp() const;

private:
    int mGPhotoIndex;
    QString mFilePath;
    QString mFlags;
    int mKbSize;
    QString mMediaType;
    int mTimeStamp;
};

class UsbDevice final : public QObject {
    Q_OBJECT
public:
    enum State {
        Init,
        Idle,
        VerifyCopy,
        StartCopy,
        Copy,
        Done,
        Error,
        Cancel,
        Removed,
    };
    Q_ENUM(State)

    UsbDevice(UsbManager& usbMan, const QString& name, int bus, int port);

    [[nodiscard]] const State& state() const;
    void setState(State state, const StateParm& parm = {});
    void resetState();
    [[nodiscard]] const QString& name() const;
    [[nodiscard]] int bus() const;
    [[nodiscard]] int port() const;
    [[nodiscard]] const StateParm& stateParm() const;
    [[nodiscard]] const QVector<UsbFile>& files() const;
    void setFiles(const QVector<UsbFile>& filePaths);
    [[nodiscard]] QVector<UsbFile> copyableUsbFiles() const;
    [[nodiscard]] int fileCount() const;
    [[nodiscard]] QString destFilePath() const;
    void setDestFilePath(const QString& path) const;
    UsbManager& usbManager() const;

Q_SIGNALS:
    void stateChanged(State state, StateParm parm);

private:
    void forceState(const State state, const StateParm& parm);

    UsbManager& mUsbManager;
    QString mName;
    int mBus;
    int mPort;

    QString mSettingsKey;

    QVector<UsbFile> mFiles;
    State mState;
    StateParm mStateParm;
};