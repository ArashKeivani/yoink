/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "MimeImport.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QMimeData>
#include <QSet>

namespace
{
bool isSupportedRemoteScheme(const QUrl &url)
{
    const QString scheme = url.scheme().toLower();
    return scheme == QLatin1String("http") || scheme == QLatin1String("https") || scheme == QLatin1String("ftp") || scheme == QLatin1String("ftps")
        || scheme == QLatin1String("mailto");
}

QString summaryFor(const MimeImport::Plan &plan)
{
    if (plan.kind == MimeImport::Plan::Kind::Image) {
        return QCoreApplication::translate("MimeImport", "image");
    }
    if (plan.kind == MimeImport::Plan::Kind::Text) {
        return plan.textIsUrl ? QCoreApplication::translate("MimeImport", "link") : QCoreApplication::translate("MimeImport", "text");
    }

    int files = 0;
    int folders = 0;
    for (const QString &path : plan.localPaths) {
        const QFileInfo info(path);
        if (info.isDir()) {
            ++folders;
        } else {
            ++files;
        }
    }

    QStringList parts;
    if (files > 0) {
        parts.append(files == 1 ? QCoreApplication::translate("MimeImport", "%n file", nullptr, files)
                                : QCoreApplication::translate("MimeImport", "%n files", nullptr, files));
    }
    if (folders > 0) {
        parts.append(folders == 1 ? QCoreApplication::translate("MimeImport", "%n folder", nullptr, folders)
                                  : QCoreApplication::translate("MimeImport", "%n folders", nullptr, folders));
    }
    if (!plan.remoteUrls.isEmpty()) {
        const int links = int(plan.remoteUrls.size());
        parts.append(links == 1 ? QCoreApplication::translate("MimeImport", "%n link", nullptr, links)
                                : QCoreApplication::translate("MimeImport", "%n links", nullptr, links));
    }
    return parts.join(QStringLiteral(" · "));
}
} // namespace

MimeImport::Plan MimeImport::plan(const QMimeData *mimeData)
{
    Plan result;
    if (!mimeData) {
        return result;
    }

    QSet<QString> seenPaths;
    QSet<QString> seenUrls;
    const QList<QUrl> urls = mimeData->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            const QString path = url.toLocalFile();
            if (path.isEmpty()) {
                continue;
            }
            const QString canonical = QFileInfo(path).absoluteFilePath();
            if (!QFileInfo::exists(canonical) || seenPaths.contains(canonical)) {
                continue;
            }
            seenPaths.insert(canonical);
            result.localPaths.append(canonical);
        } else if (url.isValid() && isSupportedRemoteScheme(url)) {
            const QString normalized = url.adjusted(QUrl::NormalizePathSegments).toString(QUrl::FullyEncoded);
            if (normalized.isEmpty() || seenUrls.contains(normalized)) {
                continue;
            }
            seenUrls.insert(normalized);
            result.remoteUrls.append(url);
        }
    }

    const bool hasImage = mimeData->hasImage() && !mimeData->imageData().value<QImage>().isNull();

    if (!result.localPaths.isEmpty() || !result.remoteUrls.isEmpty()) {
        if (result.localPaths.isEmpty() && hasImage) {
            result.kind = Plan::Kind::Image;
            result.image = qvariant_cast<QImage>(mimeData->imageData());
            result.remoteUrls.clear();
            result.count = 1;
        } else {
            result.kind = Plan::Kind::Files;
            result.count = result.localPaths.size() + result.remoteUrls.size();
        }
        result.summary = summaryFor(result);
        return result;
    }

    if (hasImage) {
        result.kind = Plan::Kind::Image;
        result.image = qvariant_cast<QImage>(mimeData->imageData());
        result.count = 1;
        result.summary = summaryFor(result);
        return result;
    }

    if (mimeData->hasText()) {
        const QString text = mimeData->text();
        if (text.isEmpty()) {
            return result;
        }

        const QString trimmed = text.trimmed();
        const bool singleLine = !trimmed.contains(QLatin1Char('\n')) && !trimmed.contains(QLatin1Char('\r'));
        if (singleLine && !trimmed.contains(QLatin1Char(' '))) {
            const QUrl candidate(trimmed);
            if (candidate.isValid() && isSupportedRemoteScheme(candidate) && (!candidate.host().isEmpty() || candidate.scheme() == QLatin1String("mailto"))) {
                result.kind = Plan::Kind::Text;
                result.text = text;
                result.textIsUrl = true;
                result.textUrl = candidate;
                result.count = 1;
                result.summary = summaryFor(result);
                return result;
            }
        }

        result.kind = Plan::Kind::Text;
        result.text = text;
        result.count = 1;
        result.summary = summaryFor(result);
    }

    return result;
}
