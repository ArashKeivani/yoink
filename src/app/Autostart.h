/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QString>

class Autostart
{
public:
    static bool isEnabled();
    static bool setEnabled(bool enabled, QString *error = nullptr);
    static QString desktopFilePath();
    static QString desktopFileContents();

private:
    static QString quoteExecArgument(const QString &argument);
};
