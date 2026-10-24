/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "YoinkImageProvider.h"

#include <QFileIconProvider>
#include <QFileInfo>
#include <QIcon>
#include <QImageReader>
#include <QPainter>
#include <QPainterPath>
#include <QUrl>

#include <cmath>

YoinkImageProvider::YoinkImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage YoinkImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const int edge = qBound(16, requestedSize.width() > 0 ? requestedSize.width() : (requestedSize.height() > 0 ? requestedSize.height() : 48), 256);

    if (id.startsWith(QLatin1String("icon/"))) {
        const QString key = QUrl::fromPercentEncoding(id.mid(5).toUtf8());
        QImage image = renderIcon(key, edge);
        if (size) {
            *size = image.size();
        }
        return image;
    }

    if (id.startsWith(QLatin1String("preview/"))) {
        const QString path = QUrl::fromPercentEncoding(id.mid(8).toUtf8());
        QImage image = renderPreview(path, edge);
        if (size) {
            *size = image.size();
        }
        return image;
    }

    if (size) {
        *size = QSize();
    }
    return {};
}

QImage YoinkImageProvider::renderIcon(const QString &key, int edge)
{
    QIcon icon;
    const int separator = key.indexOf(QLatin1Char(':'));
    const QString type = separator > 0 ? key.left(separator) : QString();
    const QString value = separator > 0 ? key.mid(separator + 1) : key;

    if (type == QLatin1String("theme")) {
        icon = QIcon::fromTheme(value);
    } else if (type == QLatin1String("file")) {
        QFileIconProvider provider;
        icon = provider.icon(QFileInfo(value));
    }
    if (icon.isNull()) {
        icon = QIcon::fromTheme(QStringLiteral("text-x-generic"));
    }

    const QPixmap pixmap = icon.pixmap(QSize(edge, edge), 1.0);
    return pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

QImage YoinkImageProvider::renderPreview(const QString &imagePath, int edge)
{
    QImageReader reader(imagePath);
    reader.setAutoTransform(true);
    QSize original = reader.size();
    if (!original.isValid()) {
        const QImage image = reader.read();
        if (image.isNull()) {
            return {};
        }
        original = image.size();
    }
    if (original.width() <= 0 || original.height() <= 0) {
        return {};
    }
    if (qint64(original.width()) * original.height() > 64'000'000) {
        return {};
    }

    const qreal scale = qMax(qreal(edge) / original.width(), qreal(edge) / original.height());
    const QSize targetSize(qMax(edge, int(std::ceil(original.width() * scale))), qMax(edge, int(std::ceil(original.height() * scale))));
    reader.setScaledSize(targetSize);

    const QImage image = reader.read();
    if (image.isNull() || image.width() < edge || image.height() < edge) {
        return {};
    }

    const QImage square = image.copy((image.width() - edge) / 2, (image.height() - edge) / 2, edge, edge);
    QImage rounded(edge, edge, QImage::Format_ARGB32_Premultiplied);
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QPainterPath roundedPath;
    roundedPath.addRoundedRect(QRectF(0, 0, edge, edge), edge * 0.22, edge * 0.22);
    painter.setClipPath(roundedPath);
    painter.drawImage(0, 0, square);
    painter.end();
    return rounded;
}
