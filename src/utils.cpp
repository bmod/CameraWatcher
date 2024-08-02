#include "utils.h"

#include <QTimer>
#include <QApplication>

#include "slugify.hpp"

void utils::invokeOnMainThread(std::function<void()> func) {
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

QString utils::qSlugify(const QString& text) {
    const auto str = text.toStdString();
    const auto slug = slugify(str);
    return QString::fromStdString(slug);
}