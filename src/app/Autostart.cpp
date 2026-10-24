/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "Autostart.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
constexpr auto DesktopFileName = "org.yoink.Yoink.desktop";
constexpr auto DesktopId = "org.yoink.Yoink";

QString configDirectory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (path.isEmpty()) {
        path = QDir::homePath() + QStringLiteral("/.config");
    }
    return path;
}
} // namespace

bool Autostart::isEnabled()
{
    QFile file(desktopFilePath());
    if (!file.exists()) {
        return false;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.compare(QLatin1String("Hidden=true"), Qt::CaseInsensitive) == 0
            || line.compare(QLatin1String("X-GNOME-Autostart-enabled=false"), Qt::CaseInsensitive) == 0) {
            return false;
        }
    }
    return true;
}

bool Autostart::setEnabled(bool enabled, QString *error)
{
    const QString path = desktopFilePath();
    if (!enabled) {
        if (!QFile::exists(path)) {
            return true;
        }
        if (!QFile::remove(path)) {
            if (error) {
                *error = QCoreApplication::translate("Autostart", "Couldn't remove the autostart file.");
            }
            return false;
        }
        return true;
    }

    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        if (error) {
            *error = QCoreApplication::translate("Autostart", "Couldn't create the autostart directory.");
        }
        return false;
    }

    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) {
            *error = QCoreApplication::translate("Autostart", "Couldn't open the autostart file for writing.");
        }
        return false;
    }
    const QByteArray contents = desktopFileContents().toUtf8();
    if (file.write(contents) != contents.size() || !file.commit()) {
        if (error) {
            *error = QCoreApplication::translate("Autostart", "Couldn't write the autostart file.");
        }
        return false;
    }
    return true;
}

QString Autostart::desktopFilePath()
{
    return QDir(configDirectory()).filePath(QStringLiteral("autostart/") + QLatin1String(DesktopFileName));
}

QString Autostart::desktopFileContents()
{
    const QString executable = QCoreApplication::applicationFilePath();
    return QStringLiteral("[Desktop Entry]\n"
                          "Type=Application\n"
                          "Name=Yoink\n"
                          "GenericName=Drag and Drop Shelf\n"
                          "Comment=Keep files, text and links within reach\n"
                          "Exec=%1 --autostart\n"
                          "Icon=%2\n"
                          "Terminal=false\n"
                          "Categories=Utility;\n"
                          "StartupNotify=false\n"
                          "X-GNOME-Autostart-enabled=true\n")
        .arg(quoteExecArgument(executable), QString::fromLatin1(DesktopId));
}

QString Autostart::quoteExecArgument(const QString &argument)
{
    QString escaped = argument;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    escaped.replace(QLatin1Char('`'), QStringLiteral("\\`"));
    escaped.replace(QLatin1Char('$'), QStringLiteral("\\$"));
    return QLatin1Char('"') + escaped + QLatin1Char('"');
}
