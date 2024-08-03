#include "utils.h"

#include <QApplication>
#include <QTimer>

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