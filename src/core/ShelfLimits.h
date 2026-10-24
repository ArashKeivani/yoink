/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QtGlobal>

namespace ShelfLimits
{
inline constexpr int MaxItems = 60;
inline constexpr qsizetype MaxTextLength = 256 * 1024;
inline constexpr qint64 MaxImagePixels = 20'000'000;
inline constexpr qint64 MaxImageBytes = 6 * 1024 * 1024;
inline constexpr qint64 MaxDataFileBytes = 64 * 1024 * 1024;
} // namespace ShelfLimits
