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

#include "backgroundmanager.h"
#include "../global_util/constants.h"

using namespace com::deepin;

static const QString DefaultWallpaper = "/usr/share/backgrounds/default_background.jpg";
static const QString DefaultWallpaper2 = "/usr/share/wallpapers/deepin/desktop.bmp";

static QString getLocalFile(const QString &file)
{
    const QUrl url(file);
    return url.isLocalFile() ? url.toLocalFile() : url.url();
}

BackgroundManager::BackgroundManager(QObject *parent)
    : QObject(parent)
    , m_currentWorkspace(-1)
    , m_wmInter(new wm("com.deepin.wm", "/com/deepin/wm", QDBusConnection::sessionBus(), this))
    , m_imageEffectInter(new ImageEffectInter("com.deepin.daemon.ImageEffect", "/com/deepin/daemon/ImageEffect", QDBusConnection::systemBus(), this))
    , m_imageblur(new ImageEffeblur("com.deepin.daemon.ImageEffect", "/com/deepin/daemon/ImageBlur", QDBusConnection::systemBus(), this))
    , m_appearanceInter(new AppearanceInter("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", QDBusConnection::sessionBus(), this))
    , m_timerUpdateBlurbg(new QTimer)
{
    m_appearanceInter->setSync(false, false);

    connect(m_wmInter, &__wm::WorkspaceSwitched, this, &BackgroundManager::updateBackgrounds);
    connect(m_wmInter, &__wm::WorkspaceBackgroundChanged, this, &BackgroundManager::updateBackgrounds);
    connect(m_appearanceInter, &AppearanceInter::Changed, this, [ = ](const QString & type, const QString &) {
        if (type == "background") {
            updateBackgrounds();
        }
    });

    m_timerUpdateBlurbg->setSingleShot(false);
    connect(m_timerUpdateBlurbg, &QTimer::timeout, this, &BackgroundManager::updateBlurBackgrounds);
    //QTimer::singleShot(0, this, &BackgroundManager::updateBackgrounds);
    updateBackgrounds();
}

void BackgroundManager::updateBackgrounds()
{
    if (!m_timerUpdateBlurbg->isActive()) {
        m_timerUpdateBlurbg->start(0);
    }
}

void BackgroundManager::updateBlurBackgrounds()
{
    m_timerUpdateBlurbg->setInterval(2000);
    QString path = getLocalFile(m_wmInter->GetCurrentWorkspaceBackground());
    QString filePath = QFile::exists(path) ? path : DefaultWallpaper;

    auto call = m_imageblur->Get(filePath);
    call.waitForFinished();
    if (call.isError()) {
        qDebug() << "call com.deepin.daemon.ImageBlur Get Error : " << call.error();
        m_timerUpdateBlurbg->stop();
        return;
    }
    if (m_imageblur->Get(filePath) != "") {
        m_blurBackground = m_imageEffectInter->Get("", m_imageblur->Get(filePath));
    } else {
        connect(m_imageblur, &ImageEffeblur::BlurDone, this, &BackgroundManager::backgroudBlurDone);
        qDebug() << "Wait for ImageEffeblur BlurDone signal";
        m_timerUpdateBlurbg->stop();
        return;
    }

    m_background = m_imageEffectInter->Get("", filePath);

    if (m_background != "") {
        emit currentWorkspaceBackgroundChanged(m_background);
    }

    if (m_blurBackground != "") {
        emit currentWorkspaceBlurBackgroundChanged(m_blurBackground);
    }

    if (m_blurBackground != "" && m_background != "") {
        m_timerUpdateBlurbg->stop();
        qDebug() << "updateBlurBackgrounds Done: " << m_background << "," << m_blurBackground;
    }
}

void BackgroundManager::backgroudBlurDone(const QString &inputPath, const QString &outputPath, bool isSucess)
{
    qDebug() << "in:" << inputPath << "out:" << outputPath << ":" << isSucess;
    m_background =m_imageEffectInter->Get("", inputPath);
    if (m_background != "") {
        emit currentWorkspaceBackgroundChanged(m_background);
    }

    m_blurBackground = m_imageEffectInter->Get("", outputPath);
    if (m_blurBackground != "") {
        emit currentWorkspaceBlurBackgroundChanged(m_blurBackground);
    }
}
