/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QImage>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

class QMimeData;

namespace MimeImport
{
struct Plan {
    enum class Kind {
        None,
        Files,
        Image,
        Text,
    };

    Kind kind = Kind::None;
    QStringList localPaths;
    QList<QUrl> remoteUrls;
    QString text;
    bool textIsUrl = false;
    QUrl textUrl;
    QImage image;
    int count = 0;
    QString summary;

    [[nodiscard]] bool isSupported() const
    {
        return count > 0;
    }
};

[[nodiscard]] Plan plan(const QMimeData *mimeData);
} // namespace MimeImport
