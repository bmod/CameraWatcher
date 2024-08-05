#pragma once


#include <QSettings>
#include <utility>

namespace CamWatcher {

    using StateParm = QVariant;

    class UsbManager;

    class UsbFile {
    public:
        UsbFile(QString filePath, int kbSize);
        [[nodiscard]] QString filePath() const;
        [[nodiscard]] int kbSize() const;

    private:
        QString mFilePath;
        int mKbSize;
    };

    class UsbDevice final : public QObject {
        Q_OBJECT
    public:
        enum State {
            Init,
            Idle,
            VerifyCopy,
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

}