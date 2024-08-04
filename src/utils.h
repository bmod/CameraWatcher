#pragma once

#include <QPair>
#include <QString>
#include <functional>
#include <utility>

namespace CamWatcher {
    struct ProcOutput {
        ProcOutput(QString out, QString err) : out(std::move(out)), err(std::move(err)) {}
        QString out;
        QString err;

        bool hasError() const { return !err.isEmpty(); }
    };


    void invokeOnMainThread(std::function<void()> func);
    QString qSlugify(const QString& text);

    ProcOutput runCmd(QStringList cmd, const QString& cwd = {});

    QStringList splitLines(const QString& text);

}// namespace CamWatcher
