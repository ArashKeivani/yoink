/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QQuickImageProvider>

class YoinkImageProvider : public QQuickImageProvider
{
public:
    YoinkImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    static QImage renderIcon(const QString &key, int edge);
    static QImage renderPreview(const QString &path, int edge);
};
