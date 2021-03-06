/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
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
#include "darrowlinedrawer.h"
#include "dheaderline.h"
#include "diconbutton.h"
#include "private/ddrawer_p.h"

#include <QResizeEvent>

DWIDGET_BEGIN_NAMESPACE

namespace HeaderLine {
    class ArrowHeaderLine : public DHeaderLine
    {
        Q_OBJECT
    public:
        ArrowHeaderLine(QWidget *parent = nullptr);
        void setExpand(bool value);

    Q_SIGNALS:
        void mousePress();

    protected:
        void mousePressEvent(QMouseEvent *) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void changeEvent(QEvent *e) override;

    private:
        void reverseArrowDirection();
        bool m_isExpanded = false;
        DIconButton *m_arrowButton = nullptr;
    };

    ArrowHeaderLine::ArrowHeaderLine(QWidget *parent) :
        DHeaderLine(parent)
    {
        m_arrowButton = new DIconButton(DStyle::SP_ReduceElement, this);
        m_arrowButton->setFlat(true);
        m_arrowButton->setAccessibleName("ArrowHeaderArrowButton");
        setExpand(false);
        connect(m_arrowButton, &DIconButton::clicked, this, &ArrowHeaderLine::mousePress);
        setContent(m_arrowButton);
        setFixedHeight(EXPAND_HEADER_HEIGHT);
    }

    void ArrowHeaderLine::setExpand(bool value)
    {
        if (value) {
            m_arrowButton->setIcon(DStyle::SP_ExpandElement);
        } else {
            m_arrowButton->setIcon(DStyle::SP_ReduceElement);
        }
        m_isExpanded = value;
    }

    void ArrowHeaderLine::mousePressEvent(QMouseEvent *)
    {
        Q_EMIT mousePress();
    }

    void ArrowHeaderLine::mouseMoveEvent(QMouseEvent *event)
    {
        //???????????????????????????
        event->accept();
    }

    void ArrowHeaderLine::changeEvent(QEvent *e)
    {
        if (e->type() == QEvent::FontChange)
            setFixedHeight(qMax(EXPAND_HEADER_HEIGHT, this->fontMetrics().height()));

        return DHeaderLine::changeEvent(e);
    }

    void ArrowHeaderLine::reverseArrowDirection()
    {
        setExpand(!m_isExpanded);
    }
}
using namespace HeaderLine;

class DArrowLineDrawerPrivate : public DDrawerPrivate
{
public:
    D_DECLARE_PUBLIC(DArrowLineDrawer)
    explicit DArrowLineDrawerPrivate(DDrawer *qq)
        : DDrawerPrivate(qq) {

    }

    ArrowHeaderLine *headerLine = nullptr;
};

/**
 * \~chinese \class DArrowLineDrawer
 * \~chinese \brief ?????????????????????????????????
 * \~chinese
 * \~chinese DArrowLineDrawer ????????? DDrawer ??????????????? ArrowHeaderLine (?????????????????????????????????)???????????????????????????????????????????????? DDrawer ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????? DDrawer ??????
 * \~chinese \sa  DDrawer
 */

/**
 * \~chinese \brief ???????????? DArrowLineDrawer ??????
 * \~chinese
 * \~chinese \param parent ?????????????????????
 */
DArrowLineDrawer::DArrowLineDrawer(QWidget *parent)
    : DDrawer(*new DArrowLineDrawerPrivate(this), parent)
{
    D_D(DArrowLineDrawer);
    d->headerLine = new ArrowHeaderLine(this);
    d->headerLine->setExpand(expand());
    d->headerLine->setAccessibleName("DArrowLineDrawerHeaderLine");
    connect(d->headerLine, &ArrowHeaderLine::mousePress, [=]{
        setExpand(!expand());
    });
    setHeader(d->headerLine);
}

/**
 * \~chinese \brief ??????????????????????????????
 * \~chinese
 * \~chinese \param title ????????????
 */
void DArrowLineDrawer::setTitle(const QString &title)
{
    D_D(DArrowLineDrawer);
    d->headerLine->setTitle(title);
}

/**
 * \~chinese \brief ???????????????????????????????????????
 * \~chinese
 * \~chinese \param value ??? true ???????????????????????????
 */
void DArrowLineDrawer::setExpand(bool value)
{
    D_D(DArrowLineDrawer);
    //Header's arrow direction change here
    d->headerLine->setExpand(value);
    DDrawer::setExpand(value);
}

/**
 * \~chinese \brief ??????????????????
 * \~chinese \return ????????????
 * \~chinese \sa ArrowHeaderLine DHeaderLine DBaseLine
 */
DBaseLine *DArrowLineDrawer::headerLine()
{
    D_D(DArrowLineDrawer);
    return d->headerLine;
}

void DArrowLineDrawer::setHeader(QWidget *header)
{
    DDrawer::setHeader(header);
}

void DArrowLineDrawer::resizeEvent(QResizeEvent *e)
{
    D_D(DArrowLineDrawer);
    d->headerLine->setFixedWidth(e->size().width());

    DDrawer::resizeEvent(e);
}

DWIDGET_END_NAMESPACE

#include "darrowlinedrawer.moc"
