#include "usbmanager.h"

#include "utils.h"

#include <QtDebug>

#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QRegularExpressionMatch>
#include <QSet>

QString createPortPath(const int bus, const int port) {
    const auto busStr = QString("%1").arg(bus, 3, 10, QChar('0'));
    const auto portStr = QString("%1").arg(port, 3, 10, QChar('0'));
    return QString("usb:%1,%2").arg(busStr, portStr);
}

const QString& patBus() {
    static const QString pat(R"((.+)\W\W\Wusb:(\d\d\d),(\d\d\d))");
    return pat;
}

const QRegularExpression& reBus() {
    static const QRegularExpression re(patBus());
    return re;
}

UsbManager::UsbManager() {
    refreshDevices();
    listenForEvents();
}

UsbManager::~UsbManager() {
    mDetectProcess.close();
}

void UsbManager::refreshDevices() {
    QProcess proc;

    proc.start("gphoto2", {"--auto-detect"});
    proc.setReadChannel(QProcess::StandardOutput);


    if (!proc.waitForReadyRead()) {
        proc.close();
        proc.setReadChannel(QProcess::StandardError);
        proc.readAll();
        qFatal("Failed to wait for process: ");
    }

    QSet<QPair<int, int>> connectedPorts;
    while (proc.canReadLine()) {
        const QString line = proc.readLine();

        auto match = reBus().match(line);
        if (!match.hasMatch())
            continue;

        const auto name = match.captured(1).trimmed();
        const int bus = match.captured(2).toInt();
        const int port = match.captured(3).toInt();
        connectedPorts.insert({bus, port});

        if (const auto dev = device(bus, port); !dev) {
            // Add device
            auto newDevice = std::make_unique<UsbDevice>(*this, name, bus, port);
            const auto devPtr = newDevice.get();
            mDevices.emplace_back(std::move(newDevice));
            deviceAdded(devPtr);
            listFiles(*devPtr);
        }
    }

    proc.close();

    for (int i = mDevices.size() - 1; i >= 0; i--) {
        auto& oldDev = mDevices[i];
        if (!connectedPorts.contains({oldDev->bus(), oldDev->port()})) {
            // Remove device
            deviceAboutToBeRemoved(oldDev.get());
            mDevices.erase(mDevices.begin() + i);
            deviceRemoved();
        }
    }
}

const std::vector<std::unique_ptr<UsbDevice>>& UsbManager::devices() const {
    return mDevices;
}

UsbDevice* UsbManager::device(const int bus, const int port) const {
    for (const auto& dev: mDevices) {
        if (dev->bus() == bus && dev->port() == port)
            return dev.get();
    }
    return nullptr;
}

int UsbManager::deviceCount() const {
    return mDevices.size();
}


void UsbManager::downloadFiles(UsbDevice& usbDevice) {
    int bus = usbDevice.bus();
    int port = usbDevice.port();
    auto usbFiles = usbDevice.files();
    auto destPath = usbDevice.destFilePath();

    usbDevice.setState(UsbDevice::Copy, "Copying files...");

    QThread* thread = QThread::create([this, bus, port, usbFiles, destPath] {
        const static QRegularExpression reDcim("./DCIM(/.+)");
        const auto portPath = createPortPath(bus, port);

        const auto dev = device(bus, port);

        const auto copyStartTime = QDateTime::currentSecsSinceEpoch();

        int copiedFiles = 0;
        int copiedKbs = 0;
        int totalKbs = 0;
        for (const auto& f: usbFiles) totalKbs += f.kbSize();

        for (int i = 0, len = usbFiles.size(); i < len; i++) {
            const auto& usbFile = usbFiles.at(i);
            if (dev->state() == UsbDevice::Cancel)
                break;

            // Notify GUI
            QString eta;
            if (copiedKbs) {
                const auto elapsedTime = QDateTime::currentSecsSinceEpoch() - copyStartTime;
                const auto kbps = copiedKbs / static_cast<float>(elapsedTime);
                const auto kbsLeft = totalKbs - copiedKbs;
                const auto secondsRemaining = kbsLeft / kbps;
                auto fmtTime = QDateTime::fromTime_t(secondsRemaining).toUTC().toString("hh:mm:ss");
                eta = QString("ETA %1").arg(fmtTime);
            }

            utils::invokeOnMainThread([this, dev, i, len, eta] {
                auto msg = QString("Copying %1 / %2").arg(i + 1).arg(len);
                if (!eta.isEmpty())
                    msg = QString("%1 (%2)").arg(msg, eta);
                dev->setState(UsbDevice::Copy, msg);
            });

            // Construct a nice path to dump to
            auto camSlug = utils::qSlugify(dev->name());
            auto outDirPath = destPath + '/' + camSlug;

            // Make sure dest dir exists
            if (QDir outDir(outDirPath); !outDir.exists())
                outDir.mkpath(".");

            auto idxStr = QString::number(usbFile.gPhotoIndex());

            const auto proc = new QProcess();
            proc->setWorkingDirectory(outDirPath);
            proc->start("gphoto2", {"--get-file=" + idxStr, "-q", "--port=" + portPath});
            proc->setReadChannel(QProcess::StandardOutput);
            proc->waitForFinished();
            proc->close();
            proc->deleteLater();

            copiedKbs += usbFile.kbSize();
            copiedFiles++;
        }

        const auto secondsElapsed = QDateTime::currentSecsSinceEpoch() - copyStartTime;
        const auto timeTaken = QDateTime::fromTime_t(secondsElapsed).toUTC().toString("hh:mm:ss");

        utils::invokeOnMainThread([this, dev, copiedFiles, timeTaken] {
            const auto msg = QString("Done! Copied %1 files. Took %2").arg(copiedFiles).arg(timeTaken);
            dev->setState(UsbDevice::Done, msg);
        });
    });

    connect(thread, &QThread::finished, [thread] {
        thread->deleteLater();
    });

    thread->start(QThread::LowPriority);
}

void UsbManager::cancelDownload(UsbDevice& dev) {
    dev.setState(UsbDevice::Cancel);
}

void UsbManager::listenForEvents() {
    connect(&mDetectProcess, &QProcess::readyReadStandardOutput, [this] {
        static const QSet<QString> interestingActions{"bind", "unbind"};
        static const QRegularExpression reEvent(R"(KERNEL\[[^\]]+\]\W(\w+)\W+(/.+)(\(\w+)\))");
        static const QRegExp reSplit("[\r\n]");
        const QString out = mDetectProcess.readAllStandardOutput();
        for (const auto& s: out.split(reSplit, Qt::SkipEmptyParts)) {
            auto match = reEvent.match(s);
            if (!match.hasMatch())
                continue;
            const auto action = match.captured(1);
            if (interestingActions.contains(action)) {
                refreshDevices();
            }
        }
    });

    connect(&mDetectProcess, &QProcess::readyReadStandardError, [this] {
        static const QRegExp re("[\r\n]");
        const QString out = mDetectProcess.readAllStandardError();
        for (const auto& s: out.split(re, Qt::SkipEmptyParts)) {
            qInfo() << "[STDERR]" << s;
        }
    });

    mDetectProcess.start("udevadm", {"monitor", "--kernel", "--subsystem-match=usb/usb_device"});
}

void UsbManager::listFiles(UsbDevice& dev) {
    int bus = dev.bus();
    int port = dev.port();

    dev.setState(UsbDevice::Init, "Listing files...");

    QThread* thread = QThread::create([this, bus, port] {
        const auto portPath = createPortPath(bus, port);

        const auto proc = new QProcess();
        proc->start("gphoto2", {"--list-files", "--port=" + portPath});
        proc->setReadChannel(QProcess::StandardOutput);

        if (!proc->waitForReadyRead()) {
            utils::invokeOnMainThread([this, bus, port] {
                if (const auto d = device(bus, port))
                    d->setState(UsbDevice::Error, "Busy");
            });
            proc->deleteLater();
            return;
        }

        static const QSet<QString> imageExtensions{"png", "jpg", "jpeg", "gif", "3fr", "ari", "arw", "bay", "braw",
                                                   "cri", "crw", "cap",  "dcs", "dng", "erf", "fff", "gpr", "jxs",
                                                   "mef", "mdc", "mos",  "mrw", "nef", "orf", "pef", "pxn", "R3D",
                                                   "raf", "raw", "raw",  "rwz", "srw", "tco", "x3f"};
        static const QSet<QString> videoExtensions{
                "webm", "mkv", "flv", "vob", "ogv", "ogg", "rrc", "gifv", "mng", "mov",  "avi", "qt",  "wmv",
                "yuv",  "rm",  "asf", "amv", "mp4", "m4p", "m4v", "mpg",  "mp2", "mpeg", "mpe", "mpv", "m4v",
                "svi",  "3gp", "3g2", "mxf", "roq", "nsv", "flv", "f4v",  "f4p", "f4a",  "f4b", "mod"};
        static const QSet<QString> allowedExtensions = imageExtensions | videoExtensions;
        static const QRegularExpression reFolder("There are \\d+ files in folder '([^']+)'");
        static const QRegularExpression reFile(R"(#(\d+)\W+([\w|.]+)\W+(\w+)\W+(\d+)\W+KB\W+(.+))");

        QString currentFolder;
        QVector<UsbFile> paths;
        while (proc->canReadLine()) {
            const auto line = proc->readLine().trimmed();

            if (auto folderMatch = reFolder.match(line); folderMatch.hasMatch()) {
                currentFolder = folderMatch.captured(1);
                continue;
            }

            if (auto fileMatch = reFile.match(line); fileMatch.hasMatch()) {
                QString fileName = fileMatch.captured(2);

                if (auto suffix = QFileInfo(fileName).suffix().toLower(); !allowedExtensions.contains(suffix))
                    continue;

                const int gPhotoIndex = fileMatch.captured(1).toInt();
                const QString filePath = currentFolder + fileMatch.captured(2);
                const QString flags = fileMatch.captured(3);
                const int kbSize = fileMatch.captured(4).toInt();
                const QString mediaType = fileMatch.captured(5);
                const int timeStamp = fileMatch.captured(6).toInt();

                paths.append({gPhotoIndex, filePath, flags, kbSize, mediaType, timeStamp});
            }
        }

        utils::invokeOnMainThread([this, bus, port, paths] {
            const auto d = device(bus, port);
            if (!d)
                return;
            d->setFiles(paths);

            if (d->state() == UsbDevice::Idle || d->state() == UsbDevice::Init) {
                d->setState(UsbDevice::Idle, QString("Files on device: %1").arg(paths.size()));
            }
        });

        proc->close();
        proc->deleteLater();
    });

    connect(thread, &QThread::finished, [thread] {
        thread->deleteLater();
    });

    thread->start(QThread::LowPriority);
}
