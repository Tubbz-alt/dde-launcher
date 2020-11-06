/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BACKGROUNDMANAGER_H
#define BACKGROUNDMANAGER_H

#include <QObject>

#include <com_deepin_wm.h>
#include <com_deepin_daemon_imageeffect.h>
#include <com_deepin_daemon_imageblur.h>
#include <com_deepin_daemon_appearance.h>

using ImageEffeblur = com::deepin::daemon::ImageBlur;
using ImageEffectInter = com::deepin::daemon::ImageEffect;
using AppearanceInter = com::deepin::daemon::Appearance;

class BackgroundManager : public QObject
{
    Q_OBJECT
public:
    explicit BackgroundManager(QObject *parent = 0);

signals:
    void currentWorkspaceBackgroundChanged(const QString &background);
    void currentWorkspaceBlurBackgroundChanged(const QString &background);

private slots:
    void updateBackgrounds();
    void updateBlurBackgrounds();
    void backgroudBlurDone(const QString &inputPath, const QString &outputPath, bool isSucess);
private:
    int m_currentWorkspace;
    mutable QString m_blurBackground;
    mutable QString m_background;

    com::deepin::wm *m_wmInter;
    ImageEffectInter *m_imageEffectInter;
    ImageEffeblur      *m_imageblur;
    AppearanceInter *m_appearanceInter;
    QTimer *m_timerUpdateBlurbg;
};

#endif // BACKGROUNDMANAGER_H
