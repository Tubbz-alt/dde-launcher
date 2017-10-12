/*
 * Copyright (C) 2017 ~ 2017 Deepin Technology Co., Ltd.
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

#include "appitemdelegate.h"
#include "global_util/constants.h"
#include "global_util/calculate_util.h"
#include "model/appslistmodel.h"
#include "dbusinterface/dbusvariant/iteminfo.h"

#include <QDebug>
#include <QPixmap>
#include <QVariant>
#include <QApplication>

#include <DSvgRenderer>

DWIDGET_USE_NAMESPACE

QModelIndex AppItemDelegate::CurrentIndex = QModelIndex();

AppItemDelegate::AppItemDelegate(QObject *parent) :
    QAbstractItemDelegate(parent),
    m_calcUtil(CalculateUtil::instance()),
    m_blueDotPixmap(":/skin/images/new_install_indicator.png")
{
    const auto ratio = qApp->devicePixelRatio();
    m_autoStartPixmap = DSvgRenderer::render(":/skin/images/emblem-autostart.svg", QSize(24, 24) * ratio);
    m_autoStartPixmap.setDevicePixelRatio(ratio);
}

void AppItemDelegate::setCurrentIndex(const QModelIndex &index)
{
    if (CurrentIndex == index)
        return;

    const QModelIndex previousIndex = CurrentIndex;
    CurrentIndex = index;

//    emit currentChanged(previousIndex, CurrentIndex);
    emit currentChanged(previousIndex, previousIndex);
    emit currentChanged(CurrentIndex, CurrentIndex);
}

void AppItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.data(AppsListModel::AppItemIsDragingRole).value<bool>() && !(option.features & QStyleOptionViewItem::HasDisplay))
        return;

    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter->setPen(Qt::white);
    painter->setBrush(QBrush(Qt::transparent));

    const int fontPixelSize = index.data(AppsListModel::AppFontSizeRole).value<int>();
    const bool drawBlueDot = index.data(AppsListModel::AppNewInstallRole).toBool();
    const ItemInfo itemInfo = index.data(AppsListModel::AppRawItemInfoRole).value<ItemInfo>();
    const QSize iconSize = index.data(AppsListModel::AppIconSizeRole).toSize();
    const QRect br = itemBoundingRect(option.rect);

    // NOTE(sbw): 多项式拟合，test case:
    // x = [51 69 86 103 121]
    // y = [23 20 10 3   0  ]
//    const int x = iconSize.width();
//    const int margin = -0.000004236988913209739*x*x*x*x+0.0016406743692943455*x*x*x-0.22885856605074573*x*x+13.187308932617098*x-243.2646393941108;

    // NOTE(sbw): 最小二乘拟合，test case:
    /*
     * 173,51       23
     * 173,69       20
     * 173,86       10
     * 173,103      3
     * 173,121      0
     * 202,60       35
     * 202,80       28
     * 202,101      18
     * 202,121      5
     * 202,141      0
     * 168,50       20
     * 168,67       15
     * 168,84       10
     * 168,100      2
     * 168,117      0
    */
    const double x1 = 0.26418192;
    const double x2 = -0.38890932;
    const double result = x1 * br.width() + x2 * iconSize.width();
    const int margin = result > 0 ? result : 0;

//    const int margin = 0;
//    qDebug() << br.width() << iconSize.width() << margin;
    const QRect boundingRect = br.marginsRemoved(QMargins(margin, margin, margin, margin));

    // draw focus background
   if (CurrentIndex == index && !(option.features & QStyleOptionViewItem::HasDisplay))
    {
        const int radius = 10;
        const int margin = 0;
        const QColor brushColor(0, 0, 0, 105);

        painter->setPen(Qt::transparent);
        painter->setBrush(brushColor);
        painter->drawRoundedRect(boundingRect.marginsRemoved(QMargins(margin, margin, margin, margin)),
                                 radius, radius);
    }

    // draw app icon
    const QPixmap iconPix = index.data(AppsListModel::AppIconRole).value<QPixmap>();
    const int iconLeftMargins = (boundingRect.width() - iconSize.width()) / 2;

    int iconTopMargin;
    if (iconSize.height() < boundingRect.height() / 2)
//        iconTopMargin = (boundingRect.height() / 2 - iconSize.height()) / 2;
        iconTopMargin = ((boundingRect.height() * 2 / 3) - iconSize.height()) / 2;
    else
        iconTopMargin = qMin(10, int(boundingRect.height() * 0.1));

    const QRect iconRect = QRect(boundingRect.topLeft() + QPoint(iconLeftMargins, iconTopMargin), iconSize);
    painter->drawPixmap(iconRect, iconPix);

    // draw icon if app is auto startup
    const QPoint autoStartIconPos = iconRect.bottomLeft() - QPoint(0, 24);
    if (index.data(AppsListModel::AppAutoStartRole).toBool())
        painter->drawPixmap(autoStartIconPos, m_autoStartPixmap);

    // draw app name
    QTextOption appNameOption;
    appNameOption.setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    appNameOption.setWrapMode(QTextOption::WordWrap);
    QFont appNamefont(painter->font());
    appNamefont.setPointSize(fontPixelSize);

    const QFontMetrics fm(appNamefont);
    const QRectF appNameRect = itemTextRect(boundingRect, iconRect, drawBlueDot);
//    const QRectF appNameBoundingRect = fm.boundingRect(appNameRect.toRect(), appNameOption.alignment() | wrapFlag, itemInfo.m_name);
    const QString appText = holdTextInRect(fm, itemInfo.m_name, appNameRect.toRect());
//    const QString appText = appNameBoundingRect.width() > appNameRect.width() || appNameBoundingRect.height() > appNameRect.height()
//                                ? fm.elidedText(itemInfo.m_name, Qt::ElideRight, appNameRect.width(), appNameOption.alignment() | wrapFlag)
//                                : itemInfo.m_name;

    painter->setFont(appNamefont);
    painter->setBrush(QBrush(Qt::transparent));
    painter->setPen(QColor(0, 0, 0, 80));
    painter->drawText(appNameRect.adjusted(0.8, 1, 0.8, 1), appText, appNameOption);
    painter->drawText(appNameRect.adjusted(-0.8, 1, -0.8, 1), appText, appNameOption);
    painter->setPen(Qt::white);
    painter->drawText(appNameRect, appText, appNameOption);

    // draw blue dot if needed
    if (drawBlueDot)
    {
        const int marginRight = 2;
        const QRectF textRect = fm.boundingRect(appNameRect.toRect(), Qt::AlignTop | Qt::AlignHCenter | Qt::TextWordWrap, appText);

        const QPointF blueDotPos = textRect.topLeft() + QPoint(-m_blueDotPixmap.width() - marginRight, (fm.height() - m_blueDotPixmap.height()) / 2);
        painter->drawPixmap(blueDotPos, m_blueDotPixmap);
    }
}

QSize AppItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)

    return index.data(AppsListModel::ItemSizeHintRole).toSize();
}

///
/// \brief calculate item background bounding rect, this rect contains all item like icon, text, blue dot, ...
/// \param itemRect the real item rect
/// \return item bounding rect
///
const QRect AppItemDelegate::itemBoundingRect(const QRect &itemRect) const
{
    const int w = itemRect.width();
    const int h = itemRect.height();
    const int sub = qAbs((w - h) / 2);

    if (w == h)
        return itemRect;
    else if (w > h)
        return itemRect - QMargins(sub, 0, sub, 0);
    else
        return itemRect - QMargins(0, sub, 0, sub);
}

///
/// \brief calculate app item text area rect
/// \param boundingRect item bounding rect
/// \param iconRect item icon rect
/// \param extraWidthMargin remove extra margin if need draw blue dot
/// \return app name text bounding rect
///
const QRect AppItemDelegate::itemTextRect(const QRect &boundingRect, const QRect &iconRect, const bool extraWidthMargin) const
{
    const int widthMargin = extraWidthMargin ? 16 : 2;
    const int heightMargin = 2;

    QRect result = boundingRect;

    result.setTop(iconRect.bottom() + 15);

    return result.marginsRemoved(QMargins(widthMargin, heightMargin, widthMargin, heightMargin));
}

const QString AppItemDelegate::holdTextInRect(const QFontMetrics &fm, const QString &text, const QRect &rect) const
{
    const int textFlag = Qt::AlignTop | Qt::AlignHCenter | Qt::TextWordWrap;

    if (rect.contains(fm.boundingRect(rect, textFlag, text)))
        return text;

    QString str(text + "...");

    while (true)
    {
        if (str.size() < 4)
            break;

        QRect boundingRect = fm.boundingRect(rect, textFlag, str);
        if (rect.contains(boundingRect))
            break;

        str.remove(str.size() - 4, 1);
    }

    return str;
}
