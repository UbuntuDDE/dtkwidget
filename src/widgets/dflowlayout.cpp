/*
 * Copyright (C) 2015 ~ 2017 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QStyle>
#include <QDebug>

#include "dflowlayout.h"
#include "private/dflowlayout_p.h"

DWIDGET_BEGIN_NAMESPACE

DFlowLayoutPrivate::DFlowLayoutPrivate(DFlowLayout *qq) :
    DObjectPrivate(qq)
{

}

QSize DFlowLayoutPrivate::doLayout(const QRect &rect, bool testOnly) const
{
    D_QC(DFlowLayout);

    if(!testOnly) {
        if(!q->parentWidget() || q->parentWidget()->isHidden())
            return QSize();
    } else if(rect.width() <= 0){
        return QSize();
    }

    int left, top, right, bottom;

    q->getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);

    int x = q->parentWidget()->layoutDirection() == Qt::RightToLeft
            ? effectiveRect.right()
            :effectiveRect.x();
    int y = effectiveRect.y();

    QSize size_hint;

    if(flow == DFlowLayout::Flow::LeftToRight) {
        int maxWidth = 0;
        int lineHeight = 0;

        if(q->parentWidget()->layoutDirection() == Qt::RightToLeft) {
            for (QLayoutItem *item : itemList) {
                int nextX = x - item->sizeHint().width() - horizontalSpacing;

                if (nextX + horizontalSpacing < effectiveRect.x() && lineHeight > 0) {
                    maxWidth = qMax(effectiveRect.right() - x, maxWidth);
                    x = effectiveRect.right();
                    y = y + lineHeight + verticalSpacing;
                    nextX = x - item->sizeHint().width() - horizontalSpacing;
                    lineHeight = 0;
                }

                if (!testOnly) {
                    QRect item_geometry;

                    item_geometry.setSize(item->sizeHint());
                    item_geometry.moveTopRight(QPoint(x, y));
                    item->setGeometry(item_geometry);
                }

                x = nextX;
                lineHeight = qMax(lineHeight, item->sizeHint().height());
            }

            size_hint = QSize(maxWidth, y + lineHeight - rect.y() + bottom);
        } else {
            for (QLayoutItem *item : itemList) {
                int nextX = x + item->sizeHint().width() + horizontalSpacing;

                if (nextX - horizontalSpacing > effectiveRect.right() && lineHeight > 0) {
                    maxWidth = qMax(x, maxWidth);
                    x = effectiveRect.x();
                    y = y + lineHeight + verticalSpacing;
                    nextX = x + item->sizeHint().width() + horizontalSpacing;
                    lineHeight = 0;
                }

                if (!testOnly)
                    item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

                x = nextX;
                lineHeight = qMax(lineHeight, item->sizeHint().height());
            }

            size_hint = QSize(maxWidth, y + lineHeight - rect.y() + bottom);
        }
    } else {
        int maxHeight = 0;
        int lineWidth = 0;

        if(q->parentWidget()->layoutDirection() == Qt::RightToLeft) {
            for (QLayoutItem *item : itemList) {
                int nextY = y + item->sizeHint().height() + verticalSpacing;

                if(nextY - verticalSpacing > effectiveRect.bottom() && lineWidth > 0) {
                    maxHeight = qMax(y, maxHeight);
                    y = effectiveRect.y();
                    x = x - lineWidth - horizontalSpacing;
                    nextY = y + item->sizeHint().height() + verticalSpacing;
                    lineWidth = 0;
                }

                if (!testOnly)
                    item->setGeometry(QRect(QPoint(x - item->sizeHint().width(), y), item->sizeHint()));

                y = nextY;
                lineWidth = qMax(lineWidth, item->sizeHint().width());
            }

            size_hint = QSize(rect.right() - x + lineWidth + right + 1, maxHeight);
        } else {
            for (QLayoutItem *item : itemList) {
                int nextY = y + item->sizeHint().height() + verticalSpacing;

                if(nextY - verticalSpacing > effectiveRect.bottom() && lineWidth > 0) {
                    maxHeight = qMax(y, maxHeight);
                    y = effectiveRect.y();
                    x = x + lineWidth + horizontalSpacing;
                    nextY = y + item->sizeHint().height() + verticalSpacing;
                    lineWidth = 0;
                }

                if (!testOnly)
                    item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

                y = nextY;
                lineWidth = qMax(lineWidth, item->sizeHint().width());
            }

            size_hint = QSize(x + lineWidth - rect.x() + right, maxHeight);
        }
    }

    if(!testOnly) {
        if(sizeHint != size_hint) {
            sizeHint = size_hint;
            Q_EMIT q->sizeHintChanged(sizeHint);
        }
    }

    return size_hint;
}

/*!
    \~english \class DFlowLayout
    \~english \brief The DFlowLayout class provides a flow list layout.

    \~english \image html dflowlayout.png

    \~english The above image is a classic use case of DFlowLayout. Notably the title text label in
    the example is not a part of DFlowLayout.
*/

/*!
    \~chinese \class DFlowLayout
    \~chinese \brief DFlowLayout ???????????????????????????????????????

    \~chinese \image html dflowlayout.png

    \~chinese ????????????????????? DFlowLayout ?????????????????????????????????????????????????????????????????? DFlowLayout ???????????????
*/

DFlowLayout::DFlowLayout(QWidget *parent) :
    QLayout(parent),
    DObject(*new DFlowLayoutPrivate(this))
{

}

DFlowLayout::DFlowLayout() :
    QLayout(),
    DObject(*new DFlowLayoutPrivate(this))
{

}

DFlowLayout::~DFlowLayout()
{
    QLayoutItem *item;

    while ((item = takeAt(0)))
        delete item;
}

/*!
 * \~chinese \brief ???????????????????????? QLayoutItem
 * \~english \brief Insert item to layout
 *
 * \~chinese \param index ??????????????????
 * \~chinese \param item ???????????? QLayoutItem
 *
 * \~english \param index to be insert
 * \~english \param item the QLayoutItem to be insert
 *
 * \sa addItem()
 */
void DFlowLayout::insertItem(int index, QLayoutItem *item)
{
    d_func()->itemList.insert(index, item);

    Q_EMIT countChanged(count());
}

/*!
 * \~chinese \brief ???????????????????????? QWidget
 * \~english \brief Insert widget to layout
 *
 * \~chinese \param index ??????????????????
 * \~chinese \param widget ???????????? QWidget
 *
 * \~english \param index to be insert
 * \~english \param widget the QWidget to be insert
 */
void DFlowLayout::insertWidget(int index, QWidget *widget)
{
    addChildWidget(widget);
    insertItem(index, new QWidgetItemV2(widget));
}

/*!
 * \~chinese \brief ???????????????????????? QLayout ?????????
 * \~english \brief Insert a sub-layout to layout
 *
 * \~chinese \param index ??????????????????
 * \~chinese \param layout ???????????? QLayout
 *
 * \~english \param index to be insert
 * \~english \param layout the QLayout to be insert
 */
void DFlowLayout::insertLayout(int index, QLayout *layout)
{
    addChildLayout(layout);
    insertItem(index, layout);
}

/*!
 * \~chinese \brief ???????????????????????????????????????????????????QSpacerItem???
 * \~english \brief Insert a fixed \a size QSpacerItem to layout
 *
 * \~chinese \param index ??????????????????
 * \~chinese \param size ???????????? QSpacerItem ?????????
 *
 * \~english \param index to be insert
 * \~english \param size of the QSpacerItem to be insert
 */
void DFlowLayout::insertSpacing(int index, int size)
{
    QSpacerItem *b;

    b = new QSpacerItem(size, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

    insertSpacerItem(index, b);
}

/*!
 * \~chinese \brief ???????????????????????????????????????????????????QSpacerItem???
 * \~english \brief Insert a expanding QSpacerItem to layout
 *
 * \~chinese \param index ??????????????????
 * \~chinese \param stretch ?????????????????????
 *
 * \~english \param index to be insert
 * \~english \param stretch Unused argument
 */
void DFlowLayout::insertStretch(int index, int stretch)
{
    Q_UNUSED(stretch);

    QSpacerItem *b;

    b = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

    insertSpacerItem(index, b);
}

/*!
 * \~chinese \brief ????????????????????????????????????QSpacerItem???
 * \~english \brief Insert a fixed \a size QSpacer to layout
 *
 * \~chinese \param index ??????????????????
 * \~chinese \param spacerItem ???????????? QSpacerItem
 *
 * \~english \param index to be insert
 * \~english \param spacerItem to be insert
 *
 * \sa addSpacerItem()
 */
void DFlowLayout::insertSpacerItem(int index, QSpacerItem *spacerItem)
{
    insertItem(index, spacerItem);
}

/*!
 * \~chinese \brief ???????????????????????????????????????????????????QSpacerItem???
 * \~english \brief Insert a fixed \a size QSpacerItem to layout
 *
 * \~chinese \param size ???????????????
 * \~english \param size of the QSpacerItem to be insert
 *
 * \sa insertSpacing()
 */
void DFlowLayout::addSpacing(int size)
{
    insertSpacing(count(), size);
}

/*!
 * \~chinese \brief ???????????????????????????????????????????????????QSpacerItem???
 * \~english \brief Insert a expanding QSpacerItem to layout
 *
 * \~chinese \param stretch ?????????????????????
 * \~english \param stretch Unused argument.
 *
 * \sa insertStretch()
 */
void DFlowLayout::addStretch(int stretch)
{
    insertStretch(count(), stretch);
}

/*!
 * \~chinese \brief ???????????????????????????????????????QSpacerItem???
 * \~english \brief Insert a QSpacerItem to layout
 *
 * \~chinese \param spacerItem ???????????? QSpacerItem
 * \~english \param spacerItem to be insert.
 *
 * \sa insertSpacerItem()
 */
void DFlowLayout::addSpacerItem(QSpacerItem *spacerItem)
{
    insertSpacerItem(count(), spacerItem);
}

/*!
 * \~chinese \brief ???????????????????????? QLayoutItem
 * \~english \brief Insert a QLayoutItem to layout
 *
 * \~chinese \param item ???????????? QLayoutItem
 * \~english \param item to be insert.
 *
 * \sa insertItem()
 */
void DFlowLayout::addItem(QLayoutItem *item)
{
    insertItem(count(), item);
}

/*!
 * \~chinese \brief ???????????????????????????
 * \~english \brief check if layout can have a fixed height
 *
 * \sa heightForWidth()
 */
bool DFlowLayout::hasHeightForWidth() const
{
    D_DC(DFlowLayout);

    return d->flow == DFlowLayout::Flow::LeftToRight;
}

/*!
 * \~chinese \brief ????????????????????????????????????
 * \~english \brief set fixed height
 *
 * \sa heightForWidth()
 */
int DFlowLayout::heightForWidth(int width) const
{
    D_DC(DFlowLayout);

    QWidget *parentWidget = this->parentWidget();

    if(parentWidget && width == parentWidget->width()) {
        return d->sizeHint.height();
    }

    return d->doLayout(QRect(0, 0, width, 0), true).height();
}

/*!
 * \~chinese \brief ????????????????????????
 * \~english \brief item count in layout
 */
int DFlowLayout::count() const
{
    return d_func()->itemList.count();
}

QLayoutItem *DFlowLayout::itemAt(int index) const
{
    return d_func()->itemList.value(index);
}

/*
 * \reimp
 */
QSize DFlowLayout::minimumSize() const
{
    D_DC(DFlowLayout);

    QSize size;

    for (QLayoutItem *item : d->itemList)
        size = size.expandedTo(item->minimumSize());

    size += QSize(2 * margin(), 2 * margin());

    return size;
}

/*
 * \reimp
 */
void DFlowLayout::setGeometry(const QRect &rect)
{
    if(rect == geometry())
        return;

    QLayout::setGeometry(QRect(rect.topLeft(),
                               d_func()->doLayout(rect, false)));
}

/*
 * \reimp
 */
QSize DFlowLayout::sizeHint() const
{
    return d_func()->sizeHint;
}

/*!
 * \~chinese \brief ??????????????? \a index ?????????????????????
 * \~english \brief take the QLayoutItem at the given \a index .
 */
QLayoutItem *DFlowLayout::takeAt(int index)
{
    D_D(DFlowLayout);

    if (index < 0 || index >= d->itemList.count()) {
        return 0;
    }

    QLayoutItem *item = d->itemList.takeAt(index);

    if (QLayout *l = item->layout()) {
        // sanity check in case the user passed something weird to QObject::setParent()
        if (l->parent() == this)
            l->setParent(0);
    }

    Q_EMIT countChanged(count());

    return item;
}

Qt::Orientations DFlowLayout::expandingDirections() const
{
    return Qt::Vertical;
}

/*!
 * \~chinese \brief ???????????????????????????????????????Spacing???
 * \~english \brief get flow spacing in horizontal direction.
 *
 * \sa setHorizontalSpacing()
 */
int DFlowLayout::horizontalSpacing() const
{
    return d_func()->horizontalSpacing;
}

/*!
 * \~chinese \brief ???????????????????????????????????????Spacing???
 * \~english \brief get flow spacing in vertical direction.
 *
 * \sa setVerticalSpacing()
 */
int DFlowLayout::verticalSpacing() const
{
    return d_func()->verticalSpacing;
}

/*!
 * \~chinese \brief ????????????????????????
 * \~english \brief get flow direction.
 *
 * \sa setFlow()
 */
DFlowLayout::Flow DFlowLayout::flow() const
{
    D_DC(DFlowLayout);

    return d->flow;
}

/*!
 * \~chinese \brief ???????????????????????????????????????Spacing???
 * \~english \brief set flow spacing in horizontal direction.
 *
 * \sa setSpacing(), setVerticalSpacing()
 */
void DFlowLayout::setHorizontalSpacing(int horizontalSpacing)
{
    D_D(DFlowLayout);

    if(horizontalSpacing == d->horizontalSpacing)
        return;

    d->horizontalSpacing = horizontalSpacing;

    Q_EMIT horizontalSpacingChanged(horizontalSpacing);

    invalidate();
}

/*!
 * \~chinese \brief ???????????????????????????????????????Spacing???
 * \~english \brief set flow spacing in vertical direction.
 *
 * \sa setSpacing(), setHorizontalSpacing()
 */
void DFlowLayout::setVerticalSpacing(int verticalSpacing)
{
    D_D(DFlowLayout);

    if(verticalSpacing == d->verticalSpacing)
        return;

    d->verticalSpacing = verticalSpacing;

    Q_EMIT verticalSpacingChanged(verticalSpacing);

    invalidate();
}

/*!
 * \~chinese \brief ?????????????????????????????????????????????Spacing???
 * \~english \brief set flow spacing in horizontal and vertical direction.
 *
 * \sa setVerticalSpacing(), setHorizontalSpacing()
 */
void DFlowLayout::setSpacing(int spacing)
{
    setHorizontalSpacing(spacing);
    setVerticalSpacing(spacing);
}

/*!
 * \~chinese \brief ??????????????????
 * \~english \brief set flow direction
 *
 * \sa QListView::Flow, flow()
 */
void DFlowLayout::setFlow(Flow direction)
{
    D_D(DFlowLayout);

    if (d->flow == direction)
        return;

    d->flow = direction;
    Q_EMIT flowChanged(direction);

    invalidate();
}

DWIDGET_END_NAMESPACE
