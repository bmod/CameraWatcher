#pragma once
#include "usbdevice.h"

#include <QProcess>
#include <memory>

#include <QRegularExpression>
#include <QThread>

namespace CamWatcher {

    struct CopyStats {
        bool removeOriginals;
        int totalKbs;
        int copiedKbs;
        int totalFiles;
        int copiedFiles;
        int kbps;
    };

    class UsbManager final : public QObject {
        Q_OBJECT
    public:
        UsbManager();
        ~UsbManager() override;

        void refreshDevices();
        [[nodiscard]] const std::vector<std::unique_ptr<UsbDevice>>& devices() const;
        [[nodiscard]] UsbDevice* device(int bus, int port) const;
        [[nodiscard]] int deviceCount() const;
        void listFiles(UsbDevice& dev);
        void downloadFiles(UsbDevice& usbDevice, bool removeOriginals);
        void cancelDownload(UsbDevice& dev);

    Q_SIGNALS:
        void deviceAdded(UsbDevice* dev);
        void deviceRemoved();
        void deviceAboutToBeRemoved(UsbDevice* dev);

    private:
        void listenForEvents();

        std::vector<std::unique_ptr<UsbDevice>> mDevices;
        QProcess mDetectProcess;
    };

}

Q_DECLARE_METATYPE(CamWatcher::CopyStats)
