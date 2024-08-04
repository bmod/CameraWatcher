#include "utils.h"

#include <QtDebug>
#include <QApplication>
#include <QProcess>
#include <QTimer>
#include <QString>

#include "slugify.hpp"

void CamWatcher::invokeOnMainThread(std::function<void()> func) {
    // any thread
    const auto timer = new QTimer();
    timer->moveToThread(qApp->thread());
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [=]() {
        // main thread
        func();
        timer->deleteLater();
    });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));
}

QString CamWatcher::qSlugify(const QString& text) {
    const auto str = text.toStdString();
    const auto slug = slugify(str);
    return QString::fromStdString(slug);
}

CamWatcher::ProcOutput CamWatcher::runCmd(QStringList cmd, const QString& cwd) {
    const auto proc = new QProcess();
    proc->setWorkingDirectory(cwd);
    qInfo() << "Run Cmd:" << cmd.join(' ');
    auto exe = cmd.takeFirst();
    proc->start(exe, cmd);
    proc->waitForFinished();

    QString err;
    if (proc->exitStatus() != QProcess::NormalExit) {
        err = proc->readAllStandardError();
    }

    auto out = proc->readAllStandardOutput();
    proc->close();
    proc->deleteLater();
    return {out, err};
}

QStringList CamWatcher::splitLines(const QString& text) {
    return text.split(QRegExp("[\r\n]"),Qt::SkipEmptyParts);
}
