/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KConfigWatcher>
#include <KSharedConfig>

#include <QColor>
#include <QObject>
#include <QString>

class Theme : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QColor surface READ surface NOTIFY changed)
    Q_PROPERTY(QColor header READ header NOTIFY changed)
    Q_PROPERTY(QColor menu READ menu NOTIFY changed)
    Q_PROPERTY(QColor menuText READ menuText NOTIFY changed)
    Q_PROPERTY(QColor text READ text NOTIFY changed)
    Q_PROPERTY(QColor textMuted READ textMuted NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor accentText READ accentText NOTIFY changed)
    Q_PROPERTY(QColor accentSoft READ accentSoft NOTIFY changed)
    Q_PROPERTY(QColor border READ border NOTIFY changed)
    Q_PROPERTY(QColor divider READ divider NOTIFY changed)
    Q_PROPERTY(QColor hover READ hover NOTIFY changed)
    Q_PROPERTY(QColor pressed READ pressed NOTIFY changed)
    Q_PROPERTY(QColor danger READ danger NOTIFY changed)
    Q_PROPERTY(QColor dangerSoft READ dangerSoft NOTIFY changed)
    Q_PROPERTY(QColor warning READ warning NOTIFY changed)
    Q_PROPERTY(QColor positive READ positive NOTIFY changed)
    Q_PROPERTY(QColor shadow READ shadow NOTIFY changed)
    Q_PROPERTY(QColor scrollbar READ scrollbar NOTIFY changed)
    Q_PROPERTY(QColor scrim READ scrim NOTIFY changed)
    Q_PROPERTY(bool dark READ isDark NOTIFY changed)
    Q_PROPERTY(qreal motionFactor READ motionFactor NOTIFY changed)
    Q_PROPERTY(bool reducedMotion READ reducedMotion NOTIFY changed)
    Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY changed)
    Q_PROPERTY(qreal bodyPointSize READ bodyPointSize NOTIFY changed)
    Q_PROPERTY(qreal smallPointSize READ smallPointSize NOTIFY changed)
    Q_PROPERTY(qreal titlePointSize READ titlePointSize NOTIFY changed)

public:
    explicit Theme(QObject *parent = nullptr);
    ~Theme() override;

    [[nodiscard]] QColor surface() const;
    [[nodiscard]] QColor header() const;
    [[nodiscard]] QColor menu() const;
    [[nodiscard]] QColor menuText() const;
    [[nodiscard]] QColor text() const;
    [[nodiscard]] QColor textMuted() const;
    [[nodiscard]] QColor accent() const;
    [[nodiscard]] QColor accentText() const;
    [[nodiscard]] QColor accentSoft() const;
    [[nodiscard]] QColor border() const;
    [[nodiscard]] QColor divider() const;
    [[nodiscard]] QColor hover() const;
    [[nodiscard]] QColor pressed() const;
    [[nodiscard]] QColor danger() const;
    [[nodiscard]] QColor dangerSoft() const;
    [[nodiscard]] QColor warning() const;
    [[nodiscard]] QColor positive() const;
    [[nodiscard]] QColor shadow() const;
    [[nodiscard]] QColor scrollbar() const;
    [[nodiscard]] QColor scrim() const;
    [[nodiscard]] bool isDark() const;
    [[nodiscard]] qreal motionFactor() const;
    [[nodiscard]] bool reducedMotion() const;
    [[nodiscard]] QString fontFamily() const;
    [[nodiscard]] qreal bodyPointSize() const;
    [[nodiscard]] qreal smallPointSize() const;
    [[nodiscard]] qreal titlePointSize() const;

    Q_INVOKABLE [[nodiscard]] int duration(int milliseconds) const;

Q_SIGNALS:
    void changed();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void reload();
    void scheduleReload();

    KSharedConfig::Ptr m_config;
    KConfigWatcher::Ptr m_watcher;

    QColor m_surface;
    QColor m_header;
    QColor m_menu;
    QColor m_menuText;
    QColor m_text;
    QColor m_textMuted;
    QColor m_accent;
    QColor m_accentText;
    QColor m_accentSoft;
    QColor m_border;
    QColor m_divider;
    QColor m_hover;
    QColor m_pressed;
    QColor m_danger;
    QColor m_dangerSoft;
    QColor m_warning;
    QColor m_positive;
    QColor m_shadow;
    QColor m_scrollbar;
    QColor m_scrim;
    bool m_dark = true;
    qreal m_motionFactor = 1.0;
    bool m_reducedMotion = false;
    QString m_fontFamily;
    qreal m_bodyPointSize = 10.0;
    qreal m_smallPointSize = 9.0;
    qreal m_titlePointSize = 12.0;
    bool m_reloadPending = false;
};
