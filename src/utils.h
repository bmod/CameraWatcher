#pragma once

#include <functional>
#include <QString>

namespace CamWatcher {
    void invokeOnMainThread(std::function<void()> func);
    QString qSlugify(const QString& text);
}// namespace utils
