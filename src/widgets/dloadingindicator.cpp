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

#include <QGraphicsProxyWidget>
#include <QLabel>

#include "dloadingindicator.h"
#include "private/dloadingindicator_p.h"
#include "dthememanager.h"

DWIDGET_BEGIN_NAMESPACE

DLoadingIndicatorPrivate::DLoadingIndicatorPrivate(DLoadingIndicator *qq) :
    DObjectPrivate(qq)
{

}

void DLoadingIndicatorPrivate::init()
{
    D_Q(DLoadingIndicator);

    q->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    q->setScene(new QGraphicsScene(q));
    q->setRenderHint(QPainter::SmoothPixmapTransform);
    q->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    q->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    q->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    q->viewport()->setAccessibleName("DLoadingIndicatorViewport");

    rotateAni.setDuration(1000);
    rotateAni.setEasingCurve(QEasingCurve::OutInQuad);

    rotateAni.setLoopCount(-1);
    rotateAni.setStartValue(QVariant(qreal(0.0)));
    rotateAni.setEndValue(QVariant(qreal(360.0)));

    q->connect(&rotateAni, SIGNAL(valueChanged(QVariant)), q, SLOT(setRotate(QVariant)));
}

void DLoadingIndicatorPrivate::setLoadingItem(QGraphicsItem *item)
{
    D_QC(DLoadingIndicator);

    QSizeF itemSize = item->boundingRect().size();

    item->setPos((q->width()-itemSize.width())/2,
                 (q->height()-itemSize.height())/2);
    item->setTransformOriginPoint(itemSize.width()/2, itemSize.height()/2);

    q->scene()->clear();
    q->scene()->addItem(item);
}

/*!
 * \~english \class DLoadingIndicator
 * \~english \brief The DLoadingIndicator class provides a widget that showing loading animation.
 * \~english
 * \~english DLoadingIndicator renders and rotates the source set by
 * \~english DLoadingIndicator::setWidgetSource or DLoadingIndicator::setImageSource,
 * \~english the rotation animation is runing in a given duration and at a given easing curve,
 * \~english those two properties can be accessed by DLoadingIndicator::aniDuration and
 * \~english DLoadingIndicator::aniEasingType .
 *
 *
 * \~chinese \class DLoadingIndicator
 * \~chinese \brief DLoadingIndicator ???????????????????????????????????????????????????????????????
 * \~chinese
 * \~chinese ????????????????????? DLoadingIndicator::imageSource ????????????
 * \~chinese DLoadingIndicator::widgetSource ????????????????????????????????????????????????
 * \~chinese ?????? start ??? stop ???????????????????????????????????????????????????????????????????????????
 * \~chinese ?????????????????????????????????????????????
 * \~chinese DLoadingIndicator::aniDuration ??? DLoadingIndicator::aniEasingType
 * \~chinese ?????????????????????????????????
 * \~chinese
 * \~chinese \note DLoadingIndicator ?????? Qt Graphics View Framework ?????????
 */


/*!
 * \~english \enum DLoadingIndicator::RotationDirection
 * \~english \brief The RotationDirection enum contains the possible rotation
 * \~english directions of the DLoadingIndicator widget
 * \~english \var DLoadingIndicator::RotationDirection DLoadingIndicator::Clockwise
 * \~english the animation will rotate clockwise.
 * \~english \var DLoadingIndicator::RotationDirection DLoadingIndicator::Counterclockwise
 * \~english the animation will rotate counterclockwise.
 *
 *
 * \~chinese \enum DLoadingIndicator::RotationDirection
 * \~chinese \brief DLoadingIndicator::RotationDirection ????????????????????????????????????????????????????????????
 * \~chinese \var DLoadingIndicator::RotationDirection DLoadingIndicator::Clockwise
 * \~chinese ???????????????
 * \~chinese \var DLoadingIndicator::RotationDirection DLoadingIndicator::Counterclockwise
 * \~chinese ???????????????
 */



/*!
 * \~english \brief DLoadingIndicator::DLoadingIndicator constructs an instance of DLoadingIndicator.
 * \~english \param parent is passed to QGraphicsView constructor.
 *
 *
 * \~chinese \brief DLoadingIndicator::DLoadingIndicator ????????? DLoadingIndicator
 * \~chinese ??????????????????
 * \~chinese \param parent ??????????????????????????????
 */
DLoadingIndicator::DLoadingIndicator(QWidget *parent) :
    QGraphicsView(parent),
    DObject(*new DLoadingIndicatorPrivate(this))
{
    d_func()->init();
}

DLoadingIndicator::~DLoadingIndicator()
{
    D_DC(DLoadingIndicator);

    if(d->widgetSource)
        d->widgetSource->deleteLater();
}

/*!
 * \~english \property DLoadingIndicator::backgroundColor
 * \~english \brief This property holds the background color of this widget.
 *
 *
 * \~chinese \property DLoadingIndicator::backgroundColor
 * \~chinese \brief DLoadingIndicator::backgroundColor ?????????????????? DLoadingIndicator
 * \~chinese ?????????????????????
 */
QColor DLoadingIndicator::backgroundColor() const
{
    return scene()->backgroundBrush().color();
}

void DLoadingIndicator::setRotate(QVariant angle)
{
    if(!scene()->items().isEmpty())
        scene()->items().first()->setRotation(angle.toReal());
}

void DLoadingIndicator::setWidgetSource(QWidget *widgetSource)
{
    D_D(DLoadingIndicator);

    if(d->widgetSource)
        d->widgetSource->deleteLater();

    d->widgetSource = widgetSource;

    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget;

    proxy->setWidget(widgetSource);

    d->setLoadingItem(proxy);
}

void DLoadingIndicator::setImageSource(const QPixmap &imageSource)
{
    D_D(DLoadingIndicator);

    QGraphicsPixmapItem * item = new QGraphicsPixmapItem(imageSource);

    if(d->smooth)
        item->setTransformationMode(Qt::SmoothTransformation);

    d->setLoadingItem(item);
}

void DLoadingIndicator::setAniEasingType(QEasingCurve::Type aniEasingType)
{
    setAniEasingCurve(aniEasingType);
}

void DLoadingIndicator::setSmooth(bool smooth)
{
    D_D(DLoadingIndicator);

    if(d->smooth == smooth)
        return;

    d->smooth = smooth;

    QGraphicsPixmapItem * item = nullptr;

    if(!scene()->items().isEmpty())
        item = dynamic_cast<QGraphicsPixmapItem*>(scene()->items().first());

    if(smooth){
        setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
        if(item)
            item->setTransformationMode(Qt::SmoothTransformation);
    }else{
        setRenderHint(QPainter::SmoothPixmapTransform, false);
        setRenderHint(QPainter::Antialiasing, false);
        if(item)
            item->setTransformationMode(Qt::FastTransformation);
    }
}

void DLoadingIndicator::setDirection(DLoadingIndicator::RotationDirection direction)
{
    D_D(DLoadingIndicator);

    if (d->direction == direction)
        return;

    d->direction = direction;

    if(direction == Clockwise) {
        d->rotateAni.setStartValue(QVariant(qreal(0.0)));
        d->rotateAni.setEndValue(QVariant(qreal(360.0)));
    } else {
        d->rotateAni.setStartValue(QVariant(qreal(0.0)));
        d->rotateAni.setEndValue(QVariant(qreal(-360.0)));
    }

    Q_EMIT directionChanged(direction);
}

void DLoadingIndicator::resizeEvent(QResizeEvent *e)
{
    QGraphicsView::resizeEvent(e);

    setSceneRect(QRectF(rect()));

    for(QGraphicsItem *item : items()) {
        QSizeF itemSize = item->boundingRect().size();

        item->setPos((width()-itemSize.width())/2,
                                 (height()-itemSize.height())/2);
    }
}

void DLoadingIndicator::setLoading(bool flag)
{
    if (flag == true){
        start();
    } else {
        stop();
    }
}

void DLoadingIndicator::setAniDuration(int msecs)
{
    D_D(DLoadingIndicator);

    d->rotateAni.setDuration(msecs);
}

/*!
 * \~english \brief DLoadingIndicator::setAniEasingCurve sets an easing curve on the loading animation.
 * \~english \param easing
 *
 * \~english This property defines the easing curve of the animation.
 * \~english By default, a linear easing curve is used, resulting in linear interpolation.
 * \~english Other curves are provided, for instance, QEasingCurve::InCirc,
 * \~english which provides a circular entry curve.
 * \~english Another example is QEasingCurve::InOutElastic,
 * \~english which provides an elastic effect on the values of the interpolated variant.
 *
 *
 * \~chinese \brief DLoadingIndicator::setAniEasingCurve ??????
 * \~chinese DLoadingIndicator::setAniEasingType ?????????????????????????????????
 * \~chinese QEasingCurve::Type ????????????????????????????????????????????????????????? QEasingCurve ???
 * \~chinese ??????????????????????????????
 */
void DLoadingIndicator::setAniEasingCurve(const QEasingCurve & easing)
{
    D_D(DLoadingIndicator);

    d->rotateAni.setEasingCurve(easing);
}

void DLoadingIndicator::setBackgroundColor(const QColor &color)
{
    scene()->setBackgroundBrush(color);
}

/*!
 * \~english \property DLoadingIndicator::loading
 * \~english \brief This property holds the animation state.
 *
 * \~english It returns true if the animation is running, false otherwise, you can also
 * \~english set value on this property to run or stop the animation.
 *
 *
 * \~chinese \property DLoadingIndicator::loading
 * \~chinese \brief DLoadingIndicator::loading ???????????????????????????????????????
 * \~chinese
 * \~chinese ????????? true ??????????????? false ???????????????
 *
 *
 * \sa start(), stop()
 */
bool DLoadingIndicator::loading() const
{
    D_DC(DLoadingIndicator);

    return d->rotateAni.state() == QVariantAnimation::Running;
}

/*!
 * \~english \property DLoadingIndicator::widgetSource
 * \~english \brief This property holds the widget to be rendered as the content of the
 * \~english loading indicator.
 *
 *
 * \~chinese \property DLoadingIndicator::widgetSource
 * \~chinese \brief DLoadingIndicator::widgetSource ???????????????????????????????????????????????????
 *
 * \sa imageSource
 */
QWidget *DLoadingIndicator::widgetSource() const
{
    D_DC(DLoadingIndicator);

    return d->widgetSource;
}

/*!
 * \~english \property DLoadingIndicator::imageSource
 * \~english \brief This property holds the image to be rendered as the content of the
 * \~english loading indicator.
 *
 *
 * \~chinese \property DLoadingIndicator::imageSource
 * \~chinese \brief DLoadingIndicator::imageSource ???????????????????????????????????????????????????
 *
 * \sa widgetSource
 */
QPixmap DLoadingIndicator::imageSource() const
{
    QGraphicsPixmapItem * item = nullptr;

    if(!scene()->items().isEmpty())
        item = dynamic_cast<QGraphicsPixmapItem*>(scene()->items().first());

    return item ? item->pixmap() : QPixmap();
}

/*!
 * \~english \property DLoadingIndicator::aniDuration
 * \~english \brief This property holds the duration of the loading animation.
 *
 *
 * \~chinese \property DLoadingIndicator::aniDuration
 * \~chinese \brief DLoadingIndicator::aniDuration ??????????????????????????????????????????
 */
int DLoadingIndicator::aniDuration() const
{
    D_DC(DLoadingIndicator);

    return d->rotateAni.duration();
}

/*!
 * \~english \property DLoadingIndicator::aniEasingType
 * \~english \brief This property holds the easing type of the easing curve used by the
 * \~english loading animation.
 *
 *
 * \~chinese \property DLoadingIndicator::aniEasingType
 * \~chinese \brief DLoadingIndicator::aniEasingType ????????????????????????????????????????????????
 *
 * \sa setAniEasingCurve
 */
QEasingCurve::Type DLoadingIndicator::aniEasingType() const
{
    D_DC(DLoadingIndicator);

    return d->rotateAni.easingCurve().type();
}

QSize DLoadingIndicator::sizeHint() const
{
    return scene()->sceneRect().size().toSize();
}

/*!
 * \~english \property DLoadingIndicator::smooth
 * \~english \brief This property holds whether the rendered content are antialiased or
 * \~english smoothly filtered.
 *
 * \~english Smooth filtering gives better visual quality, but it may be slower on
 * \~english some hardware.
 *
 *
 * \~chinese \property DLoadingIndicator::smooth
 * \~chinese \brief DLoadingIndicator::smooth ???????????????????????????????????????????????????
 * \~chinese
 * \~chinese ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
 * \~chinese ???????????????????????????
 */
bool DLoadingIndicator::smooth() const
{
    D_DC(DLoadingIndicator);

    return d->smooth;
}

/*!
 * \~english \property DLoadingIndicator::direction
 * \~english \brief This property holds the direction used while doing the rotation animation.
 *
 *
 * \~chinese \property DLoadingIndicator::direction
 * \~chinese \brief DLoadingIndicator::direction ??????????????????????????????????????????
 * \~chinese
 * \~chinese ???????????????????????????
 * \~chinese
 *
 * \sa DLoadingIndicator::RotationDirection
 */

/*!
 * \~chinese \copydoc DLoadingIndicator::direction
 */
DLoadingIndicator::RotationDirection DLoadingIndicator::direction() const
{
    D_DC(DLoadingIndicator);

    return d->direction;
}

/*!
 * \~english \property DLoadingIndicator::rotate
 * \~english \brief This property holds the current rotation of the content.
 *
 * \~english This property is usually used to correct the rotation of the content after
 * \~english calling DLoadingIndicator::stop to stop the animation.
 *
 *
 * \~chinese \property DLoadingIndicator::rotate
 * \~chinese \brief DLoadingIndicator::rotate ??????????????????????????????????????????????????????
 *
 *
 * \~chinese \sa QGraphicsItem::rotation
 */
qreal DLoadingIndicator::rotate() const
{
    if(!scene()->items().isEmpty())
        return scene()->items().first()->rotation();

    return 0;
}

/*!
 * \~english \brief DLoadingIndicator::start starts the loading animation.
 *
 *
 * \~chinese \brief DLoadingIndicator::start ?????????????????????
 */
void DLoadingIndicator::start()
{
    D_D(DLoadingIndicator);

    d->rotateAni.start();
}

/*!
 * \~english \brief DLoadingIndicator::stop stops the loading animation.
 *
 *
 * \~chinese \brief DLoadingIndicator::stop ?????????????????????
 */
void DLoadingIndicator::stop()
{
    D_D(DLoadingIndicator);

    d->rotateAni.stop();
}


DWIDGET_END_NAMESPACE
