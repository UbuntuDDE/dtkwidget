/*
 * Copyright (C) 2017 ~ 2017 Deepin Technology Co., Ltd.
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

#include "dtitlebar.h"

#include <QDebug>
#include <QMenu>
#include <QHBoxLayout>
#include <QApplication>
#include <QMouseEvent>
#include <QProcess>

#include <DWindowManagerHelper>
#include <DObjectPrivate>
#include <DPlatformTheme>

#include "dwindowclosebutton.h"
#include "dwindowmaxbutton.h"
#include "dwindowminbutton.h"
#include "dwindowoptionbutton.h"
#include "dtabletwindowoptionbutton.h"
#include "dwindowquitfullbutton.h"
#include "dplatformwindowhandle.h"
#include "daboutdialog.h"
#include "dapplication.h"
#include "private/dapplication_p.h"
#include "dmainwindow.h"
#include "DHorizontalLine"
#include "dimagebutton.h"
#include "dblureffectwidget.h"
#include "dwidgetstype.h"
#include "dlabel.h"

DWIDGET_BEGIN_NAMESPACE

const int DefaultTitlebarHeight = 50;
const int DefaultIconHeight = 32;
const int DefaultIconWidth = 32;

class DTitlebarPrivate : public DTK_CORE_NAMESPACE::DObjectPrivate
{
protected:
    DTitlebarPrivate(DTitlebar *qq);

private:
    void init();
    QWidget *targetWindow();
    // FIXME: get a batter salution
    // hide title will make eventFilter not work, instead set Height to zero
    bool isVisableOnFullscreen();
    void hideOnFullscreen();
    void showOnFullscreen();

    void updateFullscreen();
    void updateButtonsState(Qt::WindowFlags type);
    void updateButtonsFunc();
    void updateCenterArea();

    void handleParentWindowStateChange();
    void handleParentWindowIdChange();
    void _q_toggleWindowState();
    void _q_showMinimized();
    void _q_onTopWindowMotifHintsChanged(quint32 winId);

#ifndef QT_NO_MENU
    void _q_addDefaultMenuItems();
    void _q_helpActionTriggered();
    void _q_aboutActionTriggered();
    void _q_quitActionTriggered();
    void _q_switchThemeActionTriggered(QAction*action);
#endif

    void setIconVisible(bool visible);
    void updateTabOrder();

    QHBoxLayout         *mainLayout;
    QWidget             *leftArea;
    QHBoxLayout         *leftLayout;
    QWidget             *rightArea;
    QHBoxLayout         *rightLayout;
    DLabel              *centerArea;
    QHBoxLayout         *centerLayout;
    DIconButton         *iconLabel;
    QWidget             *buttonArea;
    DWindowMinButton    *minButton;
    DWindowMaxButton    *maxButton;
    DWindowCloseButton  *closeButton;
    DIconButton         *optionButton;
    DWindowQuitFullButton *quitFullButton;
    DLabel              *titleLabel;
    QWidget             *customWidget = nullptr;

    DHorizontalLine     *separatorTop;
    DHorizontalLine     *separator;

    DBlurEffectWidget   *blurWidget = nullptr;

#ifndef QT_NO_MENU
    QMenu               *menu             = Q_NULLPTR;
    QAction             *helpAction       = Q_NULLPTR;
    QAction             *aboutAction      = Q_NULLPTR;
    QAction             *quitAction       = Q_NULLPTR;
    bool                canSwitchTheme    = true;
    QAction             *themeSeparator   = nullptr;
    QMenu               *switchThemeMenu  = nullptr;
    QAction             *autoThemeAction  = nullptr;
    QAction             *lightThemeAction = nullptr;
    QAction             *darkThemeAction  = nullptr;
#endif

    QWindow            *targetWindowHandle = Q_NULLPTR;

    Qt::WindowFlags     disableFlags;
    bool                mousePressed    = false;
    bool                embedMode       = false;
    bool                autoHideOnFullscreen = false;
    bool                fullScreenButtonVisible = true;

    Q_DECLARE_PUBLIC(DTitlebar)
};

DTitlebarPrivate::DTitlebarPrivate(DTitlebar *qq)
    : DObjectPrivate(qq)
    , quitFullButton(nullptr)
{
}

void DTitlebarPrivate::init()
{
    D_Q(DTitlebar);

    mainLayout      = new QHBoxLayout;
    leftArea        = new QWidget;
    leftLayout      = new QHBoxLayout(leftArea);
    rightArea       = new QWidget;
    rightLayout     = new QHBoxLayout;
    centerArea      = new DLabel(q);
    centerLayout    = new QHBoxLayout(centerArea);
    iconLabel       = new DIconButton(q);
    buttonArea      = new QWidget;
    minButton       = new DWindowMinButton;
    maxButton       = new DWindowMaxButton;
    closeButton     = new DWindowCloseButton;

    if (DGuiApplicationHelper::isTabletEnvironment()) {
        optionButton = new DTabletWindowOptionButton;
    } else {
        optionButton = new DWindowOptionButton;
    }

    separatorTop    = new DHorizontalLine(q);
    separator       = new DHorizontalLine(q);
    titleLabel      = centerArea;
    titleLabel->setElideMode(Qt::ElideMiddle);

    minButton->installEventFilter(q);
    maxButton->installEventFilter(q);
    closeButton->installEventFilter(q);
    optionButton->installEventFilter(q);

    optionButton->setObjectName("DTitlebarDWindowOptionButton");
    optionButton->setIconSize(QSize(DefaultTitlebarHeight, DefaultTitlebarHeight));
    optionButton->setAccessibleName("DTitlebarDWindowOptionButton");
    minButton->setObjectName("DTitlebarDWindowMinButton");
    minButton->setIconSize(QSize(DefaultTitlebarHeight, DefaultTitlebarHeight));
    minButton->setAccessibleName("DTitlebarDWindowMinButton");
    maxButton->setObjectName("DTitlebarDWindowMaxButton");
    maxButton->setIconSize(QSize(DefaultTitlebarHeight, DefaultTitlebarHeight));
    maxButton->setAccessibleName("DTitlebarDWindowMaxButton");
    closeButton->setObjectName("DTitlebarDWindowCloseButton");
    closeButton->setAccessibleName("DTitlebarDWindowCloseButton");
    closeButton->setIconSize(QSize(DefaultTitlebarHeight, DefaultTitlebarHeight));


    iconLabel->setIconSize(QSize(DefaultIconWidth, DefaultIconHeight));
    iconLabel->setWindowFlags(Qt::WindowTransparentForInput);
    iconLabel->setFocusPolicy(Qt::NoFocus);
    iconLabel->setAccessibleName("DTitlebarIconLabel");
    iconLabel->setFlat(true);
    // ??????????????????????????????
    iconLabel->hide();

    leftArea->setWindowFlag(Qt::WindowTransparentForInput);
    leftArea->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    leftArea->setAccessibleName("DTitlebarLeftArea");
    leftLayout->setContentsMargins(0, 0, 0, 0);

    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerArea->setText(qApp->applicationName());
    centerArea->setWindowFlags(Qt::WindowTransparentForInput);
    centerArea->setFrameShape(QFrame::NoFrame);
    centerArea->setAutoFillBackground(false);
    centerArea->setBackgroundRole(QPalette::NoRole);
    centerArea->setAlignment(Qt::AlignCenter);
    centerArea->setAccessibleName("DTitlebarCenterArea");

    buttonArea->setWindowFlag(Qt::WindowTransparentForInput);
    buttonArea->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    buttonArea->setAccessibleName("DTitlebarButtonArea");
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonArea);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(0);
    buttonLayout->addWidget(optionButton);
    buttonLayout->addWidget(minButton);
    buttonLayout->addWidget(maxButton);
    if (!DGuiApplicationHelper::isTabletEnvironment()) {
        quitFullButton  = new DWindowQuitFullButton;
        quitFullButton->installEventFilter(q);
        quitFullButton->setObjectName("DTitlebarDWindowQuitFullscreenButton");
        quitFullButton->setAccessibleName("DTitlebarDWindowQuitFullscreenButton");
        quitFullButton->setIconSize(QSize(DefaultTitlebarHeight, DefaultTitlebarHeight));
        quitFullButton->hide();
        buttonLayout->addWidget(quitFullButton);
    }
    buttonLayout->addWidget(closeButton);

    rightArea->setWindowFlag(Qt::WindowTransparentForInput);
    rightArea->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    rightArea->setAccessibleName("DTitlebarRightArea");
    rightLayout->setContentsMargins(0, 0, 0, 0);
    auto rightAreaLayout = new QHBoxLayout(rightArea);
    rightAreaLayout->setContentsMargins(0, 0, 0, 0);
    rightAreaLayout->setMargin(0);
    rightAreaLayout->setSpacing(0);
    rightAreaLayout->addLayout(rightLayout);
    rightAreaLayout->addWidget(buttonArea);

    separatorTop->setFixedHeight(1);
    separatorTop->setAccessibleName("DTitlebarTopHorizontalLine");
    separatorTop->hide();
    separatorTop->setWindowFlags(Qt::WindowTransparentForInput);

    separator->setFixedHeight(1);
    separator->setAccessibleName("DTitlebarHorizontalLine");
    separator->hide();
    separator->setWindowFlags(Qt::WindowTransparentForInput);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(leftArea, 0, Qt::AlignLeft);
    mainLayout->addWidget(rightArea, 0, Qt::AlignRight);

    q->setLayout(mainLayout);
    q->setFixedHeight(DefaultTitlebarHeight);
    q->setMinimumHeight(DefaultTitlebarHeight);

    if (!DGuiApplicationHelper::isTabletEnvironment()) {
        q->connect(quitFullButton, &DWindowQuitFullButton::clicked, q, [ = ]() {
            bool isFullscreen = targetWindow()->windowState().testFlag(Qt::WindowFullScreen);
            if (isFullscreen) {
                targetWindow()->showNormal();
            } else {
                targetWindow()->showFullScreen();
            }
        });
    }
    q->connect(optionButton, &DIconButton::clicked, q, &DTitlebar::optionClicked);
    q->connect(DWindowManagerHelper::instance(), SIGNAL(windowMotifWMHintsChanged(quint32)),
               q, SLOT(_q_onTopWindowMotifHintsChanged(quint32)));
    q->connect(DGuiApplicationHelper::instance()->systemTheme(), &DPlatformTheme::iconThemeNameChanged, q, [ = ]() {
        iconLabel->update();
    });

    // ??????????????????????????????????????????
    q->setMenu(new QMenu(q));
    q->setFrameShape(QFrame::NoFrame);
    q->setBackgroundRole(QPalette::Base);
    q->setAutoFillBackground(true);
    q->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // ????????????????????????????????????????????????
    // ????????????????????????????????????????????????????????????focus??????????????????????????????????????????????????????????????????
    q->setFocusPolicy(Qt::StrongFocus);

    // fix wayland ???????????????????????????????????????????????? ??? dtk????????? ?????????????????????
    q->setEmbedMode(!DApplication::isDXcbPlatform());
}

QWidget *DTitlebarPrivate::targetWindow()
{
    D_Q(DTitlebar);
    return q->topLevelWidget()->window();
}

bool DTitlebarPrivate::isVisableOnFullscreen()
{
    D_Q(DTitlebar);
    return !q->property("_restore_height").isValid();
}

void DTitlebarPrivate::hideOnFullscreen()
{
    D_Q(DTitlebar);
    if (q->height() > 0) {
        q->setProperty("_restore_height", q->height());
    }
    q->setFixedHeight(0);
}

void DTitlebarPrivate::showOnFullscreen()
{
    D_Q(DTitlebar);
    if (q->property("_restore_height").isValid()) {
        q->setFixedHeight(q->property("_restore_height").toInt());
        q->setProperty("_restore_height", QVariant());
    }
}

void DTitlebarPrivate::updateFullscreen()
{
    D_Q(DTitlebar);

    if (!autoHideOnFullscreen) {
        return;
    }

    bool isFullscreen = targetWindow()->windowState().testFlag(Qt::WindowFullScreen);
    auto mainWindow = qobject_cast<DMainWindow *>(targetWindow());
    if (!isFullscreen) {
        if (!DGuiApplicationHelper::isTabletEnvironment())
            quitFullButton->hide();
        mainWindow->setMenuWidget(q);
        showOnFullscreen();
    } else {
        // must set to empty
        if (!DGuiApplicationHelper::isTabletEnvironment())
            quitFullButton->show();
        if (mainWindow->menuWidget()) {
            mainWindow->menuWidget()->setParent(nullptr);
            mainWindow->setMenuWidget(Q_NULLPTR);
        }

        q->setParent(mainWindow);
        q->show();
        hideOnFullscreen();
    }
}

void DTitlebarPrivate::updateButtonsState(Qt::WindowFlags type)
{
    D_Q(DTitlebar);
    bool useDXcb = DPlatformWindowHandle::isEnabledDXcb(targetWindow());
    bool isFullscreen = targetWindow()->windowState().testFlag(Qt::WindowFullScreen);

//    bool forceShow = !useDXcb;
    bool forceShow = false;
#ifndef Q_OS_LINUX
    forceShow = false;
#endif

    bool showTitle = (type.testFlag(Qt::WindowTitleHint) || forceShow) && !embedMode;
    if (titleLabel) {
        if (showTitle) {
            titleLabel->setText(q->property("_dtk_title").toString());
        } else {
            q->setProperty("_dtk_title", titleLabel->text());
            titleLabel->clear();
        }
    }

    // Never show in embed/fullscreen
    bool forceHide = (!useDXcb) || embedMode || isFullscreen;

    bool showMin = (type.testFlag(Qt::WindowMinimizeButtonHint) || forceShow) && !forceHide;
    minButton->setVisible(showMin);

    bool allowResize = true;

    if (q->window() && q->window()->windowHandle()) {
        auto functions_hints = DWindowManagerHelper::getMotifFunctions(q->window()->windowHandle());
        allowResize = functions_hints.testFlag(DWindowManagerHelper::FUNC_RESIZE);
    }

    bool showMax = (type.testFlag(Qt::WindowMaximizeButtonHint) || forceShow) && !forceHide && allowResize;
    bool showClose = type.testFlag(Qt::WindowCloseButtonHint) && useDXcb;
    bool showQuit = isFullscreen && useDXcb && fullScreenButtonVisible;
    maxButton->setVisible(showMax);
    closeButton->setVisible(showClose);

    if (!DGuiApplicationHelper::isTabletEnvironment())
        quitFullButton->setVisible(showQuit);
    //    qDebug() << "max:"
    //             << "allowResize" << allowResize
    //             << "useDXcb" << useDXcb
    //             << "forceHide" << forceHide
    //             << "type.testFlag(Qt::WindowMaximizeButtonHint)" << type.testFlag(Qt::WindowMaximizeButtonHint);
}

void DTitlebarPrivate::updateButtonsFunc()
{
    // TASK-18145 (bug-17474) do not setMotifFunctions on wayland
    if (!targetWindowHandle || !qgetenv("WAYLAND_DISPLAY").isEmpty()) {
        return;
    }

    // ?????? disableFlags ?????????????????????????????????????????????????????????/?????????????????????
    // ??????????????????????????????????????????????????????????????????????????? disableFlags ?????????
    // ??????????????????????????????????????????????????????????????????????????? setDisableFlags
    // ??????????????????????????? disableFlags ????????????????????? setDisableFlags
    // ????????????????????????????????????????????????????????????????????????????????????
    DWindowManagerHelper::setMotifFunctions(
        targetWindowHandle,
        DWindowManagerHelper::FUNC_MAXIMIZE,
        !disableFlags.testFlag(Qt::WindowMaximizeButtonHint));
    DWindowManagerHelper::setMotifFunctions(
        targetWindowHandle,
        DWindowManagerHelper::FUNC_MINIMIZE,
        !disableFlags.testFlag(Qt::WindowMinimizeButtonHint));
    DWindowManagerHelper::setMotifFunctions(
        targetWindowHandle,
        DWindowManagerHelper::FUNC_CLOSE,
        !disableFlags.testFlag(Qt::WindowCloseButtonHint));
}

void DTitlebarPrivate::updateCenterArea()
{
    D_QC(DTitlebar);

    if (centerArea->isHidden()) {
        return;
    }

    int padding = qMax(leftArea->width(), rightArea->width());
    QRect rect(0, 0, q->width() - 2 * padding, q->height());
    rect.moveCenter(q->rect().center());
    centerArea->setGeometry(rect);
}

void DTitlebarPrivate::handleParentWindowStateChange()
{
    maxButton->setMaximized(targetWindow()->windowState() == Qt::WindowMaximized);
    updateFullscreen();
    updateButtonsState(targetWindow()->windowFlags());
}

//!
//! \brief DTitlebarPrivate::handleParentWindowIdChange
//! Them WindowStateChnage Event will miss some state changed message,
//! So use windowHandle::windowStateChanged instead
void DTitlebarPrivate::handleParentWindowIdChange()
{
    if (!targetWindowHandle) {
        targetWindowHandle = targetWindow()->windowHandle();

        updateButtonsFunc();
    } else if (targetWindow()->windowHandle() != targetWindowHandle) {
        // Parent change???, show never here
        qWarning() << "targetWindowHandle change" << targetWindowHandle << targetWindow()->windowHandle();
    }
}

void DTitlebarPrivate::_q_toggleWindowState()
{
    QWidget *parentWindow = targetWindow();

    if (!parentWindow || disableFlags.testFlag(Qt::WindowMaximizeButtonHint)) {
        return;
    }

    if (parentWindow->isMaximized()) {
        parentWindow->showNormal();
    } else if (!parentWindow->isFullScreen()
               && (maxButton->isVisible())) {
        parentWindow->showMaximized();
    }
}

void DTitlebarPrivate::_q_showMinimized()
{
    targetWindow()->showMinimized();
}

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
static Qt::WindowFlags &setWindowFlag(Qt::WindowFlags &flags, Qt::WindowType type, bool on)
{
    return on ? (flags |= type) : (flags &= ~int(type));
}
#endif

void DTitlebarPrivate::_q_onTopWindowMotifHintsChanged(quint32 winId)
{
    D_QC(DTitlebar);

    if (!DPlatformWindowHandle::isEnabledDXcb(targetWindow())) {
        q->disconnect(DWindowManagerHelper::instance(), SIGNAL(windowMotifWMHintsChanged(quint32)),
                      q, SLOT(_q_onTopWindowMotifHintsChanged(quint32)));
        return;
    }

    if (winId != q->window()->internalWinId()) {
        return;
    }

    DWindowManagerHelper::MotifDecorations decorations_hints = DWindowManagerHelper::getMotifDecorations(q->window()->windowHandle());
    DWindowManagerHelper::MotifFunctions functions_hints = DWindowManagerHelper::getMotifFunctions(q->window()->windowHandle());

    if (titleLabel) {
        titleLabel->setVisible(decorations_hints.testFlag(DWindowManagerHelper::DECOR_TITLE));
    }

    updateButtonsState(targetWindow()->windowFlags());

    minButton->setEnabled(functions_hints.testFlag(DWindowManagerHelper::FUNC_MINIMIZE));
    maxButton->setEnabled(functions_hints.testFlag(DWindowManagerHelper::FUNC_MAXIMIZE)
                          && functions_hints.testFlag(DWindowManagerHelper::FUNC_RESIZE));
    closeButton->setEnabled(functions_hints.testFlag(DWindowManagerHelper::FUNC_CLOSE));
    // sync button state
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    disableFlags.setFlag(Qt::WindowMinimizeButtonHint, !minButton->isEnabled());
    disableFlags.setFlag(Qt::WindowMaximizeButtonHint, !maxButton->isEnabled());
    disableFlags.setFlag(Qt::WindowCloseButtonHint, !closeButton->isEnabled());
#else
    setWindowFlag(disableFlags, Qt::WindowMinimizeButtonHint, !minButton->isEnabled());
    setWindowFlag(disableFlags, Qt::WindowMaximizeButtonHint, !maxButton->isEnabled());
    setWindowFlag(disableFlags, Qt::WindowCloseButtonHint, !closeButton->isEnabled());
#endif
}

#ifndef QT_NO_MENU

void DTitlebarPrivate::_q_addDefaultMenuItems()
{
    D_Q(DTitlebar);

    // add switch theme sub menu
    if (!switchThemeMenu && DGuiApplicationHelper::testAttribute(DGuiApplicationHelper::Attribute::IsDeepinPlatformTheme)) {
        bool disableDtkSwitchThemeMenu = qEnvironmentVariableIsSet("KLU_DISABLE_MENU_THEME");
        if (!disableDtkSwitchThemeMenu) {
            switchThemeMenu = new QMenu(qApp->translate("TitleBarMenu", "Theme"), menu);
            switchThemeMenu->setAccessibleName("DTitlebarThemeMenu");
            lightThemeAction = switchThemeMenu->addAction(qApp->translate("TitleBarMenu", "Light Theme"));
            darkThemeAction = switchThemeMenu->addAction(qApp->translate("TitleBarMenu", "Dark Theme"));
            autoThemeAction = switchThemeMenu->addAction(qApp->translate("TitleBarMenu", "System Theme"));

            autoThemeAction->setCheckable(true);
            lightThemeAction->setCheckable(true);
            darkThemeAction->setCheckable(true);

            QActionGroup *group = new QActionGroup(switchThemeMenu);
            group->addAction(autoThemeAction);
            group->addAction(lightThemeAction);
            group->addAction(darkThemeAction);

            QObject::connect(group, SIGNAL(triggered(QAction*)),
                             q, SLOT(_q_switchThemeActionTriggered(QAction*)));

            menu->addMenu(switchThemeMenu);
            themeSeparator = menu->addSeparator();

            switchThemeMenu->menuAction()->setVisible(canSwitchTheme);

            themeSeparator->setVisible(DGuiApplicationHelper::isTabletEnvironment() ? false : canSwitchTheme);
        }
    }

    // add help menu item.
    if (!helpAction) {
        helpAction = new QAction(qApp->translate("TitleBarMenu", "Help"), menu);
        QObject::connect(helpAction, SIGNAL(triggered(bool)), q, SLOT(_q_helpActionTriggered()));
        menu->addAction(helpAction);
        helpAction->setVisible(false);
    }

    // add about menu item.
    if (!aboutAction) {
        aboutAction = new QAction(qApp->translate("TitleBarMenu", "About"), menu);
        QObject::connect(aboutAction, SIGNAL(triggered(bool)), q, SLOT(_q_aboutActionTriggered()));
        menu->addAction(aboutAction);
    }

    // add quit menu item.
    if (!quitAction) {
        quitAction = new QAction(qApp->translate("TitleBarMenu", "Exit"), menu);
        QObject::connect(quitAction, SIGNAL(triggered(bool)), q, SLOT(_q_quitActionTriggered()));
        if (!DGuiApplicationHelper::isTabletEnvironment()) {
            menu->addAction(quitAction);
        }
    }
}

void DTitlebarPrivate::_q_helpActionTriggered()
{
    DApplication *dapp = qobject_cast<DApplication *>(qApp);
    if (dapp) {
        dapp->handleHelpAction();
    }
}

void DTitlebarPrivate::_q_aboutActionTriggered()
{
    DApplication *dapp = qobject_cast<DApplication *>(qApp);
    if (dapp) {
        dapp->handleAboutAction();
    }
}

void DTitlebarPrivate::_q_quitActionTriggered()
{
    DApplication *dapp = qobject_cast<DApplication *>(qApp);
    if (dapp) {
        dapp->handleQuitAction();
    }
}

void DTitlebarPrivate::_q_switchThemeActionTriggered(QAction *action)
{
    DGuiApplicationHelper::ColorType type = DGuiApplicationHelper::UnknownType;

    if (action == lightThemeAction) {
        type = DGuiApplicationHelper::LightType;
    } else if (action == darkThemeAction) {
        type = DGuiApplicationHelper::DarkType;
    }

    DGuiApplicationHelper::instance()->setPaletteType(type);
}

void DTitlebarPrivate::setIconVisible(bool visible)
{
    if (iconLabel->isVisible() == visible)
        return;

    if (visible) {
        leftLayout->insertSpacing(0, 10);
        leftLayout->insertWidget(1, iconLabel, 0, Qt::AlignLeading | Qt::AlignVCenter);
        iconLabel->show();
    } else {
        iconLabel->hide();
        // ???????????????????????????????????????
        delete leftLayout->takeAt(0);
        delete leftLayout->takeAt(1);
    }
}

void DTitlebarPrivate::updateTabOrder()
{
    D_Q(DTitlebar);

    QList<QWidget *> orderWidget;
    QList<QHBoxLayout *> orderLayout;
    orderLayout << leftLayout << centerLayout << rightLayout;

    //?????? leftLayout???centerLayout???rightLayout ?????????????????? TabFocus ????????? widget
    for (QHBoxLayout * lyt : orderLayout) {
        if (!lyt) {
            continue;
        }

        for (int i = 0; i < lyt->count(); ++i) {
            QWidget *wdg = lyt->itemAt(i)->widget();
            if (wdg && (wdg->focusPolicy() & Qt::FocusPolicy::TabFocus)) {
                orderWidget.append(wdg);
            }
        }
    }

    if (orderWidget.isEmpty()) {
        return;
    }

    //?????????????????? widget ???????????? taborder
    QWidget::setTabOrder(q, orderWidget.first());
    for (int i = 0; i < orderWidget.count() - 1; ++i) {
        QWidget::setTabOrder(orderWidget.at(i), orderWidget.at(i + 1));
    }
}

#endif

/*!
 * \~english \class DTitlebar
 * \brief The DTitlebar class is an universal title bar on the top of windows.
 * \param parent is the parent widget to be attached on.
 *
 * Usually you don't need to construct a DTitlebar instance by your self, you
 * can get an DTitlebar instance by DMainWindow::titlebar .
 */
/*!
 * \~chinese \class DTitlebar
 * \brief Dtitlebar???Dtk???????????????????????????????????????????????????????????????????????????
 * \param ?????????????????????????????????????????????
 *
 * ???????????????????????????Dtk::Widget::DMainWindow::titlebar()?????????????????????????????????????????????
 * ???????????????????????????????????????
 */


/*!
 *  \~english @brief DTitlebar::DTitlebar create an default widget with icon/title/and buttons
 * @param parent
 */
/*!
 *  \~chinese @brief ????????????DTitlebar???????????????????????????????????????
 */
DTitlebar::DTitlebar(QWidget *parent) :
    QFrame(parent),
    DObject(*new DTitlebarPrivate(this))
{
    if (DApplication::buildDtkVersion() < DTK_VERSION_CHECK(2, 0, 6, 1)) {
        setBackgroundTransparent(true);
    }

    D_D(DTitlebar);
    d->init();

    // ???????????????????????????????????????????????????
    if (parent && parent->window()->windowType() != Qt::Window) {
        d->optionButton->hide();
    }
}

#ifndef QT_NO_MENU
/*!
 * \~english @brief DTitlebar::menu holds the QMenu object attached to this title bar.
 * @return the QMenu object it holds, returns null if there's no one set.
 */
/*!
 * \~chinese @brief ????????????????????????????????????????????????
 * @return ??????????????????????????????????????????????????????????????????????????? Dtk::Widget::DApplication ,
 * ??????????????????????????????????????????????????????
 */
QMenu *DTitlebar::menu() const
{
    D_DC(DTitlebar);

    return d->menu;
}

/*!
 * \~english @brief DTitlebar::setMenu attaches a QMenu object to the title bar.
 * @param menu is the target menu.
 */
/*!
 * \~chinese @brief ??????????????????????????????
 * @param ????????????????????????
 */
void DTitlebar::setMenu(QMenu *menu)
{
    D_D(DTitlebar);

    d->menu = menu;
    if (d->menu) {
        d->menu->setAccessibleName("DTitlebarMainMenu");
        disconnect(this, &DTitlebar::optionClicked, 0, 0);
        connect(this, &DTitlebar::optionClicked, this, &DTitlebar::showMenu);
    }
}

#endif

/*!
 * \~english @brief DTitlebar::customWidget
 * @return the customized widget used in this title bar.
 *
 * One can set customized widget to show some extra widgets on the title bar.
 * \see Dtk::Widget::DTitlebar::setCustomWidget()
 */
/*!
 * \~chinese @brief ?????????????????????????????????
 * @return ???????????????
 *
 * ????????????????????????????????????????????????????????????????????????
 *
 * \see Dtk::Widget::DTitlebar::setCustomWidget()
 */
QWidget *DTitlebar::customWidget() const
{
    D_DC(DTitlebar);

    return d->customWidget;
}

#ifndef QT_NO_MENU
/*!
 * \~english \brief DTitlebar::showMenu pop the menu of application on titlebar.
 */
/*!
 * \~chineses \brief ????????????????????????
 */

void DTitlebar::showMenu()
{
    D_D(DTitlebar);

#ifndef QT_NO_MENU
    // ?????????????????????showmenu????????????, ??????showevent
    d->_q_addDefaultMenuItems();
#endif
     if (d->helpAction)
         d->helpAction->setVisible(DApplicationPrivate::isUserManualExists());

    if (d->menu) {
        // ????????????????????????
        if (d->switchThemeMenu) {
            QAction *action;

            switch (DGuiApplicationHelper::instance()->paletteType()) {
            case DGuiApplicationHelper::LightType:
                action = d->lightThemeAction;
                break;
            case DGuiApplicationHelper::DarkType:
                action = d->darkThemeAction;
                break;
            default:
                action = d->autoThemeAction;
                break;
            }

            action->setChecked(true);
        }

        d->menu->exec(d->optionButton->mapToGlobal(d->optionButton->rect().bottomLeft()));
        d->optionButton->update(); // FIX: bug-25253 sometimes optionButton not udpate after menu exec(but why?)
    }
}
#endif

void DTitlebar::showEvent(QShowEvent *event)
{
    //fix the width issue and process menu
    D_D(DTitlebar);
    d->separatorTop->setFixedWidth(width());
    d->separatorTop->move(0, 0);
    d->separator->setFixedWidth(width());
    d->separator->move(0, height() - d->separator->height());

    QWidget::showEvent(event);

    if (DPlatformWindowHandle::isEnabledDXcb(window())) {
        d->_q_onTopWindowMotifHintsChanged(
            static_cast<quint32>(window()->internalWinId()));
    }

    d->updateCenterArea();
}

void DTitlebar::mousePressEvent(QMouseEvent *event)
{
    D_D(DTitlebar);
    d->mousePressed = (event->button() == Qt::LeftButton);

    if (event->button() == Qt::RightButton) {
        DWindowManagerHelper::popupSystemWindowMenu(window()->windowHandle());
        return;
    }

#ifdef DTK_TITLE_DRAG_WINDOW
    Q_EMIT mousePosPressed(event->buttons(), event->globalPos());
#endif
    Q_EMIT mousePressed(event->buttons());
}

void DTitlebar::mouseReleaseEvent(QMouseEvent *event)
{
    D_D(DTitlebar);
    if (event->button() == Qt::LeftButton) {
        d->mousePressed = false;
    }
}

bool DTitlebar::eventFilter(QObject *obj, QEvent *event)
{
    D_D(DTitlebar);

    if (event->type() == QEvent::MouseButtonPress &&
            static_cast<QMouseEvent *>(event)->button() == Qt::RightButton &&
            (obj ==d->minButton || obj == d->maxButton ||
            obj == d->closeButton || obj == d->optionButton ||
            obj == d->quitFullButton))
    {
        event->accept(); // button on titlebar should not show kwin menu
        return true;
    }

    if (obj == d->targetWindow()) {
        switch (event->type()) {
        case QEvent::ShowToParent:
            d->handleParentWindowIdChange();
            d->updateButtonsState(d->targetWindow()->windowFlags());
            break;
        case QEvent::Resize:
            if (d->autoHideOnFullscreen) {
                setFixedWidth(d->targetWindow()->width());
            }
            break;
        case QEvent::HoverMove: {
            auto mouseEvent = reinterpret_cast<QMouseEvent *>(event);
            bool isFullscreen = d->targetWindow()->windowState().testFlag(Qt::WindowFullScreen);
            if (isFullscreen && d->autoHideOnFullscreen) {
                if (mouseEvent->pos().y() > height() && d->isVisableOnFullscreen()) {
                    d->hideOnFullscreen();
                }
                if (mouseEvent->pos().y() < 2) {
                    d->showOnFullscreen();
                }
            }
            break;
        }
        case QEvent::WindowStateChange: {
            d->handleParentWindowStateChange();
            break;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

bool DTitlebar::event(QEvent *e)
{
    if (e->type() == QEvent::LayoutRequest) {
        D_D(DTitlebar);

        d->updateCenterArea();
    }

    return QFrame::event(e);
}

void DTitlebar::resizeEvent(QResizeEvent *event)
{
    //override QWidget::resizeEvent to fix button and separator pos.
    D_D(DTitlebar);

    d->separatorTop->setFixedWidth(event->size().width());
    d->separator->setFixedWidth(event->size().width());
    d->updateCenterArea();

    if (d->blurWidget) {
        d->blurWidget->resize(event->size());
    }

    return QWidget::resizeEvent(event);
}

/*!
 * \~english @brief DTitlebar::setCustomWidget is an overloaded function.
 * @param w is the widget to be used as the customize widget shown in the title
 * bar.
 * @param fixCenterPos indicates whether it should automatically move the
 * customize widget to the horizontal center of the title bar or not.
 */
/*!
 * \~chinese @brief ????????????????????????????????????
 * @param w ????????????????????????
 * @param fixCenterPos ????????????????????????????????????????????????????????????????????????
 */
void DTitlebar::setCustomWidget(QWidget *w, bool fixCenterPos)
{
    D_D(DTitlebar);

    if (w == d->customWidget) {
        return;
    }

    if (d->customWidget) {
        d->mainLayout->removeWidget(d->customWidget);
        d->customWidget->hide();
        d->customWidget->deleteLater();
    }

    d->customWidget = w;

    if (w) {
        w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    } else {
        d->centerArea->show();
        d->titleLabel = d->centerArea;

        return;
    }

    if (fixCenterPos) {
        for (int i = 0; i < d->centerLayout->count(); ++i) {
            delete d->centerLayout->itemAt(i);
        }

        addWidget(w, Qt::Alignment());
        d->centerArea->show();
        d->titleLabel = d->centerArea;
    } else {
        d->mainLayout->insertWidget(1, w);
        d->titleLabel = nullptr;
        d->centerArea->hide();
    }
}

void DTitlebar::addWidget(QWidget *w, Qt::Alignment alignment)
{
    D_D(DTitlebar);

    if (alignment & Qt::AlignLeft) {
        d->leftLayout->addWidget(w, 0, alignment & ~Qt::AlignLeft);
    } else if (alignment & Qt::AlignRight) {
        d->rightLayout->addWidget(w, 0, alignment & ~Qt::AlignRight);
    } else {
        d->centerLayout->addWidget(w, 0, alignment);
        d->centerArea->clear();
        d->titleLabel = nullptr;
    }

    updateGeometry();
    d->updateTabOrder();
}

void DTitlebar::removeWidget(QWidget *w)
{
    D_D(DTitlebar);

    d->leftLayout->removeWidget(w);
    d->centerLayout->removeWidget(w);
    d->rightLayout->removeWidget(w);

    if (d->centerLayout->isEmpty()) {
        d->titleLabel = d->centerArea;
        d->titleLabel->setText(d->targetWindowHandle->title());
        setProperty("_dtk_title", d->titleLabel->text());
    }

    updateGeometry();
    d->updateTabOrder();
}

/*!
 * \~english @brief DTitlebar::setFixedHeight change the height of the title bar to
 * another value.
 * @param h is the target height.
 */
/*!
 * \~chinese @brief ?????????????????????????????????????????? 50???
 * @param h ?????????????????????
 */
void DTitlebar::setFixedHeight(int h)
{
    QWidget::setFixedHeight(h);
}

/*!
 * \~english @brief DTitlebar::setBackgroundTransparent set the title background transparent
 * @param transparent is the targeting value.
 */
/*!
 * \~chinese @brief ?????????????????????????????????????????????????????????????????????????????????????????????
 * @param transparent ????????????
 */
void DTitlebar::setBackgroundTransparent(bool transparent)
{
    setAutoFillBackground(!transparent);

    if (transparent)
        setBackgroundRole(QPalette::NoRole);
    else
        setBackgroundRole(QPalette::Base);
}

/*!
 * \~english @brief DTitlebar::setSeparatorVisible sets the bottom separator of the title
 * bar and the window contents to be visible or not.
 * @param visible is the targeting value.
 */
/*!
 * \~chinese @brief ??????????????????????????????????????????????????????????????????
 * @param ????????????
 */
void DTitlebar::setSeparatorVisible(bool visible)
{
    D_D(DTitlebar);
    if (visible) {
        d->separator->show();
        d->separator->raise();
    } else {
        d->separator->hide();
    }
}

/*!
 * \~english @brief DTitlebar::setTitle sets the title to be shown on the title bar.
 * @param title is the text to be used as the window title.
 */
/*!
 * \~chinese @brief ????????????????????????
 * @param ???????????????
 */
void DTitlebar::setTitle(const QString &title)
{
    D_D(DTitlebar);
    if (d->titleLabel && !d->embedMode) {
        d->titleLabel->setText(title);
    } else if (parentWidget()) {
        parentWidget()->setWindowTitle(title);
    }
    setProperty("_dtk_title", title);
}

/*!
 * \~english @brief DTitlebar::setIcon sets the icon to be shown on the title bar.
 * @param icon is to be used as the window icon.
 */
/*!
 * \~chinese @brief ?????????????????????
 * @param ??????????????????
 */
void DTitlebar::setIcon(const QIcon &icon)
{
    D_D(DTitlebar);
    if (!d->embedMode) {
        d->iconLabel->setIcon(icon);
        d->setIconVisible(!icon.isNull());
    } else if (parentWidget()) {
        d->setIconVisible(false);
        parentWidget()->setWindowIcon(icon);
    }
}

void DTitlebar::toggleWindowState()
{
    D_D(DTitlebar);

    d->_q_toggleWindowState();
}

void DTitlebar::setBlurBackground(bool blurBackground)
{
    D_D(DTitlebar);

    if (static_cast<bool>(d->blurWidget) == blurBackground)
        return;

    if (d->blurWidget) {
        d->blurWidget->hide();
        d->blurWidget->deleteLater();
        d->blurWidget = nullptr;
    } else {
        d->blurWidget = new DBlurEffectWidget(this);
        d->blurWidget->lower();
        d->blurWidget->resize(size());
        d->blurWidget->setMaskColor(DBlurEffectWidget::AutoColor);
        d->blurWidget->setRadius(30);
        d->blurWidget->show();
    }

    setAutoFillBackground(!blurBackground);
}

/*!
 * \~english @brief DTitlebar::buttonAreaWidth returns the width of the area that all the
 * window buttons occupies.
 */
/*!
 * \~chinese @brief ??????????????????????????????????????????????????????????????????
 */
int DTitlebar::buttonAreaWidth() const
{
    D_DC(DTitlebar);
    return d->buttonArea->width();
}

/*!
 * \~english @brief DTitlebar::separatorVisible returns the visibility of the bottom
 * separator of the titlebar.
 */
/*!
 * \~chinese @brief ????????????????????????
 */
bool DTitlebar::separatorVisible() const
{
    D_DC(DTitlebar);
    return d->separator->isVisible();
}

/*!
 * \~english @brief DTitlebar::autoHideOnFullscreen returns if titlebar show on fullscreen mode.
 * separator of the titlebar.
 */
/*!
 * \~chinese @brief ??????????????????????????????????????????
 */
bool DTitlebar::autoHideOnFullscreen() const
{
    D_DC(DTitlebar);
    return d->autoHideOnFullscreen;
}

/*!
 * \~english \brief DTitlebar::setAutoHideOnFullscreen set if titlebar show when window is fullscreen state.
 * \param autohide
 */
/*!
 * \~chinese \brief ??????????????????????????????????????????????????????
 * \param ??????????????????
 */
void DTitlebar::setAutoHideOnFullscreen(bool autohide)
{
    D_D(DTitlebar);
    d->autoHideOnFullscreen = autohide;
}

void DTitlebar::setVisible(bool visible)
{
    D_D(DTitlebar);

    if (visible == isVisible()) {
        return;
    }

    QWidget::setVisible(visible);

    if (visible) {
        if (!d->targetWindow()) {
            return;
        }
        d->targetWindow()->installEventFilter(this);

        connect(d->maxButton, SIGNAL(clicked()), this, SLOT(_q_toggleWindowState()), Qt::UniqueConnection);
        connect(this, SIGNAL(doubleClicked()), this, SLOT(_q_toggleWindowState()), Qt::UniqueConnection);
        connect(d->minButton, SIGNAL(clicked()), this, SLOT(_q_showMinimized()), Qt::UniqueConnection);
        connect(d->closeButton, &DWindowCloseButton::clicked, d->targetWindow(), &QWidget::close, Qt::UniqueConnection);

        d->updateButtonsState(d->targetWindow()->windowFlags());
    } else {
        if (!d->targetWindow()) {
            return;
        }
        d->targetWindow()->removeEventFilter(this);
    }
}


/*!
 * \~english @brief DTitlebar::setEmbedMode set a titlebar is in parent;
 */
/*!
 * \~chinese @brief ????????????????????????????????????????????????????????????????????????dxcb?????????
 */
void DTitlebar::setEmbedMode(bool visible)
{
    D_D(DTitlebar);
    d->embedMode = visible;
    d->separatorTop->setVisible(visible);
    d->updateButtonsState(windowFlags());
}

/*!
 * \~chinese \brief DTitlebar::menuIsVisible
 * \~chinese \return true ???????????? false???????????????
 */
bool DTitlebar::menuIsVisible() const
{
    D_DC(DTitlebar);
    return !d->optionButton->isVisible();
}

/*!
 * \~chinese \brief DTitlebar::setMenuVisible ????????????????????????
 * \~chinese \param visible true ???????????? false???????????????
 */
void DTitlebar::setMenuVisible(bool visible)
{
    D_D(DTitlebar);
    d->optionButton->setVisible(visible);
}

/*!
 * \~chinese \brief DTitlebar::menuIsDisabled
 * \~chinese \return true ??????????????? false ?????????????????????
 */
bool DTitlebar::menuIsDisabled() const
{
    D_DC(DTitlebar);
    return !d->optionButton->isEnabled();
}

/*!
 * \~chinese \brief DTitlebar::setMenuDisabled ???????????????????????????
 * \~chinese \param disabled true ??????????????? false?????????????????????
 */
void DTitlebar::setMenuDisabled(bool disabled)
{
    D_D(DTitlebar);
    d->optionButton->setDisabled(disabled);
}

/*!
 * \~chinese \brief DTitlebar::quitMenuIsDisabled
 * \~chinese \return true ????????????????????? false???????????????????????????
 */
bool DTitlebar::quitMenuIsDisabled() const
{
    D_DC(DTitlebar);

    return d->quitAction && !d->quitAction->isEnabled();
}

/*!
 * \~chinese \brief DTitlebar::setQuitMenuDisabled ?????????????????????????????????
 * \~chinese \param disabled true ????????????????????? false???????????????????????????
 */
void DTitlebar::setQuitMenuDisabled(bool disabled)
{
    D_D(DTitlebar);

    if (!d->quitAction) {
        d->_q_addDefaultMenuItems();
    }

    d->quitAction->setEnabled(!disabled);
}

/*!
 * \~chinese \brief DTitlebar::setQuitMenuVisible ?????????????????????????????????
 * \~chinese \param visible true ?????????????????? false?????????????????????
 */
void DTitlebar::setQuitMenuVisible(bool visible)
{
    D_D(DTitlebar);

    if (!d->quitAction) {
        d->_q_addDefaultMenuItems();
    }

    d->quitAction->setVisible(visible);
}

/*!
 * \~chinese \brief DTitlebar::switchThemeMenuIsVisible
 * \~chinese \return true ???????????????????????? false???????????????????????????
 */
bool DTitlebar::switchThemeMenuIsVisible() const
{
    D_DC(DTitlebar);

    return d->switchThemeMenu;
}

/*!
 * \~chinese \brief DTitlebar::setSwitchThemeMenuVisible ????????????????????????????????????
 * \~chinese \param visible true ???????????????????????? false???????????????????????????
 */
void DTitlebar::setSwitchThemeMenuVisible(bool visible)
{
    D_D(DTitlebar);

    if (visible == d->canSwitchTheme) {
        return;
    }

    d->canSwitchTheme = visible;

    if (d->switchThemeMenu) {
        d->switchThemeMenu->menuAction()->setVisible(visible);
        d->themeSeparator->setVisible(visible);
    }
}

/*!
 * \~english \brief DTitlebar::setDisableFlags will disable button match flags.
 * \param flags
 */
/*!
 * \~chinese \brief ??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
 * \param flags ?????????????????????????????????
 */
void DTitlebar::setDisableFlags(Qt::WindowFlags flags)
{
    D_D(DTitlebar);
    d->disableFlags = flags;
    d->updateButtonsFunc();
}

/*!
 * \~english \brief DTitlebar::disableFlags return which button is disabled.
 */
/*!
 * \~chinese \brief ????????????????????????????????????
 */
Qt::WindowFlags DTitlebar::disableFlags() const
{
    D_DC(DTitlebar);
    return d->disableFlags;
}

QSize DTitlebar::sizeHint() const
{
    D_DC(DTitlebar);

    if (d->centerArea->isHidden()) {
        return QFrame::sizeHint();
    }

    int padding = qMax(d->leftArea->sizeHint().width(), d->rightArea->sizeHint().width());
    int width = d->centerArea->sizeHint().width() + 2 * d->mainLayout->spacing() + 2 * padding;

    return QSize(width, DefaultTitlebarHeight);
}

QSize DTitlebar::minimumSizeHint() const
{
    return sizeHint();
}

bool DTitlebar::blurBackground() const
{
    D_DC(DTitlebar);
    return d->blurWidget;
}

void DTitlebar::setFullScreenButtonVisible(bool visible)
{
    D_D(DTitlebar);
    d->fullScreenButtonVisible = visible;
}

void DTitlebar::mouseMoveEvent(QMouseEvent *event)
{
    D_D(DTitlebar);

    Qt::MouseButton button = event->buttons() & Qt::LeftButton ? Qt::LeftButton : Qt::NoButton;
    if (event->buttons() == Qt::LeftButton /*&& d->mousePressed*/) {
        if (!d->mousePressed) {
            return;
        }
        Q_EMIT mouseMoving(button);
    }

#ifdef DTK_TITLE_DRAG_WINDOW
    D_D(DTitlebar);
    if (d->mousePressed) {
        Q_EMIT mousePosMoving(button, event->globalPos());
    }
#endif
    QWidget::mouseMoveEvent(event);
}

void DTitlebar::mouseDoubleClickEvent(QMouseEvent *event)
{
    D_D(DTitlebar);

    if (event->buttons() == Qt::LeftButton) {
        d->mousePressed = false;
        Q_EMIT doubleClicked();
    }
}

DWIDGET_END_NAMESPACE

#include "moc_dtitlebar.cpp"
