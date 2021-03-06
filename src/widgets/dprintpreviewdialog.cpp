#include "dprintpreviewdialog.h"

#include "private/dprintpreviewdialog_p.h"
#include "dframe.h"
#include "diconbutton.h"
#include "dlabel.h"
#include "dlineedit.h"
#include "dfloatingbutton.h"
#include "dswitchbutton.h"
#include "dsuggestbutton.h"
#include "dcommandlinkbutton.h"
#include "dspinbox.h"
#include "dbackgroundgroup.h"
#include "dfiledialog.h"
#include "dpalettehelper.h"
#include "dstyleoption.h"
#include "dapplication.h"
#include "dfilechooseredit.h"
#include "dslider.h"
#include "dtoolbutton.h"

#include <DScrollArea>
#include <DScrollBar>
#include <DPlatformWindowHandle>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QtPrintSupport/QPrinterInfo>
#include <QPrinter>
#include <QDebug>
#include <QTime>
#include <QFontDatabase>
#include <QDesktopServices>
#include <QRegExp>
#include <QRegExpValidator>
#include <QTimer>
#include <QKeyEvent>
#include <private/qprint_p.h>
#include <private/qcups_p.h>
#include <private/qprintdevice_p.h>
#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

#define NORMAL_LEFT_RIGHT 31.8
#define NORMAL_MODERATE_TOP_BOTTRM 25.4
#define MODERATE_LEFT_RIGHT 19.1

#define EightK_Weight 270
#define EightK_Height 390
#define SixteenK_Weight 195
#define SixteenK_Height 270

#define WIDTH_NORMAL 422
#define HEIGHT_NORMAL 48
#define SPACER_HEIGHT_HIDE 0
#define SPACER_HEIGHT_SHOW 370

#define PAGERANGE_ALL 0
#define PAGERANGE_CURRENT 1
#define PAGERANGE_SELECT 2
#define FIRST_PAGE 1

#define ACTUAL_SIZE 1
#define SCALE 2

#define PICKCOLOR_RADIUS 8
#define PICKCOLOR_WIDTH 314
#define PICKCOLOR_HEIGNT 375

#define WATERLAYOUT_CENTER 0
#define WATERLAYOUT_TILED 1
#define WATERFONT_SIZE 65

DWIDGET_BEGIN_NAMESPACE

void setwidgetfont(QWidget *widget, DFontSizeManager::SizeType type = DFontSizeManager::T5)
{
    QFont font = widget->font();
    font.setBold(true);
    widget->setFont(font);
    DFontSizeManager::instance()->bind(widget, type);
}

static void _d_setSpinboxDefaultValue(QHash<QWidget *, QString> valueCaches, DSpinBox *spinBox)
{
    if (valueCaches.contains(spinBox->lineEdit()) && (valueCaches.value(spinBox->lineEdit()).isEmpty() || valueCaches.value(spinBox->lineEdit()) == "??")) {
        QVariant defaultVariant = spinBox->property("_d_printPreview_spinboxDefalutValue");
        if (defaultVariant.isValid())
            spinBox->setValue(defaultVariant.toInt());
    }
}

DPrintPreviewDialogPrivate::DPrintPreviewDialogPrivate(DPrintPreviewDialog *qq)
    : DDialogPrivate(qq)
{
}

void DPrintPreviewDialogPrivate::startup()
{
    Q_Q(DPrintPreviewDialog);

    this->printer = new DPrinter;

    if (qApp)
        qApp->installEventFilter(q);
    initui();
    initdata();
    initconnections();
}

void DPrintPreviewDialogPrivate::initui()
{
    Q_Q(DPrintPreviewDialog);
    DWidget *mainWidget = new DWidget(q);
    mainWidget->setObjectName("mainwidget");

    DWidget *titleWidget = new DWidget(q);
    titleWidget->setObjectName("titlewidget");

    mainWidget->setAutoFillBackground(true);

    DPalette pa = DPaletteHelper::instance()->palette(titleWidget);
    pa.setBrush(DPalette::Background, pa.base());
    DPaletteHelper::instance()->setPalette(titleWidget, pa);
    titleWidget->setAutoFillBackground(true);

    QHBoxLayout *mainlayout = new QHBoxLayout();
    mainlayout->setContentsMargins(QMargins(0, 0, 0, 0));
    mainlayout->setSpacing(0);

    QVBoxLayout *pleftlayout = new QVBoxLayout;
    initleft(pleftlayout);
    QVBoxLayout *prightlayout = new QVBoxLayout;
    initright(prightlayout);

    DWidget *leftWidget = new DWidget;
    DWidget *rightWidget = new DWidget;
    leftWidget->setObjectName("leftWidget");
    rightWidget->setObjectName("rightWidget");
    mainlayout->addWidget(leftWidget);
    mainlayout->addWidget(rightWidget);
    leftWidget->setLayout(pleftlayout);
    rightWidget->setLayout(prightlayout);
    back = new DBackgroundGroup(mainlayout);
    back->setObjectName("backGround");
    back->setItemSpacing(2);
    q->addSpacing(5);
    q->addContent(back);

    colorWidget = new DFloatingWidget(q);
    colorWidget->setFixedSize(PICKCOLOR_WIDTH, PICKCOLOR_HEIGNT);
    pickColorWidget = new DPrintPickColorWidget(colorWidget);
    colorWidget->setWidget(pickColorWidget);
    colorWidget->hide();
}

void DPrintPreviewDialogPrivate::initleft(QVBoxLayout *layout)
{
    Q_Q(DPrintPreviewDialog);
    pview = new DPrintPreviewWidget(this->printer);
    pview->setLineWidth(0);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->addWidget(pview);

    QHBoxLayout *pbottomlayout = new QHBoxLayout;
    pbottomlayout->setContentsMargins(0, 10, 0, 0);
    layout->addLayout(pbottomlayout);
    firstBtn = new DIconButton(DStyle::SP_ArrowPrev);
    prevPageBtn = new DIconButton(DStyle::SP_ArrowLeft);
    firstBtn->setIcon(QIcon::fromTheme("printer_original"));
    firstBtn->setEnabled(false);
    prevPageBtn->setEnabled(false);
    jumpPageEdit = new DSpinBox;
    jumpPageEdit->setMaximumWidth(105);
    jumpPageEdit->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);
    jumpPageEdit->installEventFilter(q);
    DLabel *spaceLabel = new DLabel("/");
    totalPageLabel = new DLabel;
    originTotalPageLabel = new DLabel;
    originTotalPageLabel->setEnabled(false);
    nextPageBtn = new DIconButton(DStyle::SP_ArrowRight);
    lastBtn = new DIconButton(DStyle::SP_ArrowNext);
    lastBtn->setIcon(QIcon::fromTheme("printer_final"));
    pbottomlayout->addStretch();
    pbottomlayout->addWidget(firstBtn);
    pbottomlayout->addSpacing(10);
    pbottomlayout->addWidget(prevPageBtn);
    pbottomlayout->addSpacing(55);
    pbottomlayout->addWidget(jumpPageEdit);
    pbottomlayout->addWidget(spaceLabel);
    pbottomlayout->addWidget(totalPageLabel);
    pbottomlayout->addWidget(originTotalPageLabel);
    pbottomlayout->addSpacing(55);
    pbottomlayout->addWidget(nextPageBtn);
    pbottomlayout->addSpacing(10);
    pbottomlayout->addWidget(lastBtn);
    pbottomlayout->addStretch();

    QRegExp reg("^([1-9][0-9]*)");
    QRegExpValidator *val = new QRegExpValidator(reg, jumpPageEdit);
    jumpPageEdit->lineEdit()->setValidator(val);

    DPalette m_pa = DPaletteHelper::instance()->palette(pview);
    m_pa.setBrush(DPalette::Base, m_pa.itemBackground());
    DPaletteHelper::instance()->setPalette(pview, m_pa);
}

void DPrintPreviewDialogPrivate::initright(QVBoxLayout *layout)
{
    //top
    QVBoxLayout *ptoplayout = new QVBoxLayout;
    ptoplayout->setContentsMargins(0, 0, 0, 0);
    DWidget *ptopwidget = new DWidget;
    ptopwidget->setMinimumWidth(WIDTH_NORMAL);
    ptopwidget->setLayout(ptoplayout);

    basicsettingwdg = new DWidget;
    advancesettingwdg = new DWidget;
    if (Q_LIKELY(QLocale::system().language() != QLocale::Tibetan)) {
        basicsettingwdg->setFixedHeight(415);
    } else {
        basicsettingwdg->setFixedHeight(445);
    }
    scrollarea = new DScrollArea;
    scrollarea->setWidget(ptopwidget);
    scrollarea->setWidgetResizable(true);
    scrollarea->setFrameShape(QFrame::NoFrame);
    scrollarea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    advancesettingwdg->hide();

    advanceBtn = new DPushButton(qApp->translate("DPrintPreviewDialogPrivate", "Advanced"));
    advanceBtn->setLayoutDirection(Qt::RightToLeft);
    advanceBtn->setIcon(QIcon::fromTheme("printer_dropdown"));
    advanceBtn->setIconSize(QSize(12, 12));
    DPalette pa = advanceBtn->palette();
    pa.setColor(DPalette::ButtonText, pa.link().color());
    advanceBtn->setPalette(pa);
    advanceBtn->setFlat(true);

    QHBoxLayout *advancelayout = new QHBoxLayout;
    advancelayout->addStretch();
    advancelayout->addWidget(advanceBtn);
    advancelayout->addStretch();
    ptoplayout->addWidget(basicsettingwdg);
    ptoplayout->addLayout(advancelayout);
    ptoplayout->addStretch();
    ptoplayout->addWidget(advancesettingwdg);

    initbasicui();
    initadvanceui();
    //bottom
    QHBoxLayout *pbottomlayout = new QHBoxLayout;
    pbottomlayout->setContentsMargins(0, 10, 0, 10);
    cancelBtn = new DPushButton(qApp->translate("DPrintPreviewDialogPrivate", "Cancel", "button"));
    printBtn = new DSuggestButton(qApp->translate("DPrintPreviewDialogPrivate", "Print", "button"));

    cancelBtn->setFixedSize(170, 36);
    printBtn->setFixedSize(170, 36);
    pbottomlayout->addWidget(cancelBtn);
    pbottomlayout->addWidget(printBtn);

    layout->addWidget(scrollarea);
    layout->addLayout(pbottomlayout);
}

void DPrintPreviewDialogPrivate::initbasicui()
{
    Q_Q(DPrintPreviewDialog);
    QVBoxLayout *layout = new QVBoxLayout(basicsettingwdg);
    layout->setSpacing(10);
    DLabel *basicLabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Basic"), basicsettingwdg);
    setwidgetfont(basicLabel);
    QHBoxLayout *basictitlelayout = new QHBoxLayout;
    layout->addLayout(basictitlelayout);
    basictitlelayout->setContentsMargins(10, 0, 0, 0);
    basictitlelayout->addWidget(basicLabel);
    basictitlelayout->setAlignment(basicLabel, Qt::AlignLeft | Qt::AlignBottom);

    //???????????????
    DFrame *printerFrame = new DFrame(basicsettingwdg);
    layout->addWidget(printerFrame);
    printerFrame->setMinimumSize(WIDTH_NORMAL, HEIGHT_NORMAL);
    setfrmaeback(printerFrame);
    QHBoxLayout *printerlayout = new QHBoxLayout(printerFrame);
    printerlayout->setContentsMargins(10, 0, 10, 0);
    DLabel *printerlabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Printer"), printerFrame);
    printDeviceCombo = new DComboBox(basicsettingwdg);
    printerlayout->addWidget(printerlabel, 4);
    printerlayout->addWidget(printDeviceCombo, 9);
    printerlayout->setAlignment(printDeviceCombo, Qt::AlignVCenter);

    //????????????
    DFrame *copycountFrame = new DFrame(basicsettingwdg);
    copycountFrame->setObjectName("copucountframe");
    layout->addWidget(copycountFrame);
    copycountFrame->setMinimumSize(WIDTH_NORMAL, HEIGHT_NORMAL);
    setfrmaeback(copycountFrame);
    QHBoxLayout *copycountlayout = new QHBoxLayout(copycountFrame);
    copycountlayout->setContentsMargins(10, 0, 10, 0);
    DLabel *copycountlabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Copies"), copycountFrame);
    copycountspinbox = new DSpinBox(copycountFrame);
    copycountspinbox->setEnabledEmbedStyle(true);
    copycountspinbox->setRange(1, 999);
    copycountspinbox->installEventFilter(q);
    copycountlayout->addWidget(copycountlabel, 4);
    copycountlayout->addWidget(copycountspinbox, 9);

    QRegExp re("^[1-9][0-9][0-9]$");
    QRegExpValidator *va = new QRegExpValidator(re, copycountspinbox);
    copycountspinbox->lineEdit()->setValidator(va);

    //????????????
    DFrame *pageFrame = new DFrame(basicsettingwdg);
    pageFrame->setObjectName("pageFrame");
    layout->addWidget(pageFrame);
    pageFrame->setMinimumSize(WIDTH_NORMAL, 94);
    setfrmaeback(pageFrame);
    QVBoxLayout *pagelayout = new QVBoxLayout(pageFrame);
    pagelayout->setContentsMargins(10, 5, 10, 5);
    DLabel *pagerangelabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Page range"), pageFrame);
    pageRangeCombo = new DComboBox(pageFrame);
    pageRangeCombo->addItem(qApp->translate("DPrintPreviewDialogPrivate", "All"));
    pageRangeCombo->addItem(qApp->translate("DPrintPreviewDialogPrivate", "Current page"));
    pageRangeCombo->addItem(qApp->translate("DPrintPreviewDialogPrivate", "Select pages"));
    QHBoxLayout *hrangebox = new QHBoxLayout();
    hrangebox->addWidget(pagerangelabel, 4);
    hrangebox->addWidget(pageRangeCombo, 9);

    pageRangeEdit = new DLineEdit;
    pagelayout->addLayout(hrangebox);
    pagelayout->addWidget(pageRangeEdit);
    pageRangeEdit->installEventFilter(q);

    QRegularExpression reg(R"(^([1-9][0-9]*?(-[1-9][0-9]*?)?,)*?([1-9][0-9]*?|[1-9][0-9]*?-[1-9][0-9]*?)$)");
    QRegularExpressionValidator *val = new QRegularExpressionValidator(reg, pageRangeEdit);
    pageRangeEdit->lineEdit()->setValidator(val);

    //????????????
    DLabel *orientationLabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Orientation"), basicsettingwdg);
    setwidgetfont(orientationLabel);
    QHBoxLayout *orientationtitlelayout = new QHBoxLayout;
    orientationtitlelayout->setContentsMargins(10, 0, 0, 0);
    orientationtitlelayout->addWidget(orientationLabel);
    orientationtitlelayout->setAlignment(orientationLabel, Qt::AlignLeft | Qt::AlignBottom);
    layout->addLayout(orientationtitlelayout);

    QVBoxLayout *orientationlayout = new QVBoxLayout;
    orientationlayout->setContentsMargins(0, 0, 0, 0);
    DRadioButton *verRadio = new DRadioButton;
    verRadio->setIcon(QIcon::fromTheme("printer_portrait"));
    verRadio->setIconSize(QSize(36, 36));
    DRadioButton *horRadio = new DRadioButton;
    horRadio->setIcon(QIcon::fromTheme("printer_landscape"));
    horRadio->setIconSize(QSize(36, 36));
    orientationgroup = new QButtonGroup(q);
    orientationgroup->addButton(verRadio, 0);
    orientationgroup->addButton(horRadio, 1);

    //??????
    DWidget *portraitwdg = new DWidget;
    portraitwdg->setMinimumSize(WIDTH_NORMAL, HEIGHT_NORMAL);
    QHBoxLayout *portraitlayout = new QHBoxLayout;
    DLabel *orientationTextLabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Portrait"), portraitwdg);
    portraitlayout->addWidget(verRadio);
    portraitlayout->addWidget(orientationTextLabel);
    portraitlayout->addStretch();
    portraitwdg->setLayout(portraitlayout);

    //??????
    DWidget *landscapewdg = new DWidget;
    landscapewdg->setMinimumSize(WIDTH_NORMAL, HEIGHT_NORMAL);
    QHBoxLayout *landscapelayout = new QHBoxLayout;
    DLabel *landscapeTextLabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Landscape"), portraitwdg);
    landscapelayout->addWidget(horRadio);
    landscapelayout->addWidget(landscapeTextLabel);
    landscapelayout->addStretch();
    landscapewdg->setLayout(landscapelayout);

    orientationlayout->addWidget(portraitwdg);
    orientationlayout->addWidget(landscapewdg);
    DBackgroundGroup *back = new DBackgroundGroup(orientationlayout);
    back->setObjectName("OrientationBackgroundGroup");
    back->setItemSpacing(2);
    DPalette pa = DPaletteHelper::instance()->palette(back);
    pa.setBrush(DPalette::Base, pa.itemBackground());
    DPaletteHelper::instance()->setPalette(back, pa);
    layout->addWidget(back);
}

void DPrintPreviewDialogPrivate::initadvanceui()
{
    Q_Q(DPrintPreviewDialog);
    QVBoxLayout *layout = new QVBoxLayout(advancesettingwdg);
    layout->setContentsMargins(0, 0, 0, 0);
    advancesettingwdg->setMinimumWidth(WIDTH_NORMAL);

    //????????????
    QVBoxLayout *pagelayout = new QVBoxLayout;
    pagelayout->setSpacing(10);
    pagelayout->setContentsMargins(10, 0, 10, 0);
    DLabel *pagesLabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Pages"), advancesettingwdg);
    setwidgetfont(pagesLabel, DFontSizeManager::T5);
    QHBoxLayout *pagestitlelayout = new QHBoxLayout;
    pagestitlelayout->setContentsMargins(10, 20, 0, 0);
    pagestitlelayout->addWidget(pagesLabel, Qt::AlignLeft | Qt::AlignBottom);

    DFrame *colorframe = new DFrame;
    setfrmaeback(colorframe);
    colorframe->setFixedHeight(HEIGHT_NORMAL);
    QHBoxLayout *colorlayout = new QHBoxLayout(colorframe);
    DLabel *colorlabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Color mode"));
    colorModeCombo = new DComboBox;
    colorModeCombo->addItems(QStringList() << qApp->translate("DPrintPreviewDialogPrivate", "Color") << qApp->translate("DPrintPreviewDialogPrivate", "Grayscale"));
    colorlayout->addWidget(colorlabel, 4);
    colorlayout->addWidget(colorModeCombo, 9);
    colorlayout->setContentsMargins(10, 4, 10, 4);

    DFrame *marginsframe = new DFrame;
    marginsframe->setObjectName("marginsFrame");
    setfrmaeback(marginsframe);
    QVBoxLayout *marginslayout = new QVBoxLayout(marginsframe);
    marginslayout->setContentsMargins(10, 5, 10, 5);
    QHBoxLayout *marginscombolayout = new QHBoxLayout;
    DLabel *marginlabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Margins"));
    marginsCombo = new DComboBox;
    marginsCombo->addItems(QStringList() << qApp->translate("DPrintPreviewDialogPrivate", "Narrow (mm)") << qApp->translate("DPrintPreviewDialogPrivate", "Normal (mm)") << qApp->translate("DPrintPreviewDialogPrivate", "Moderate (mm)") << qApp->translate("DPrintPreviewDialogPrivate", "Customize (mm)"));
    marginscombolayout->addWidget(marginlabel, 4);
    marginscombolayout->addWidget(marginsCombo, 9);

    QHBoxLayout *marginsspinlayout = new QHBoxLayout;
    marginsspinlayout->setContentsMargins(0, 0, 0, 0);
    DLabel *toplabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Top"));
    marginTopSpin = new DDoubleSpinBox;
    marginTopSpin->installEventFilter(q);
    DLabel *leftlabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Left"));
    marginLeftSpin = new DDoubleSpinBox;
    marginLeftSpin->installEventFilter(q);
    DLabel *bottomlabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Bottom"));
    marginBottomSpin = new DDoubleSpinBox;
    marginBottomSpin->installEventFilter(q);
    DLabel *rightlabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Right"));
    marginRightSpin = new DDoubleSpinBox;
    marginRightSpin->installEventFilter(q);
    QVBoxLayout *marginslabellayout1 = new QVBoxLayout;
    marginslabellayout1->addWidget(toplabel);
    marginslabellayout1->addWidget(leftlabel);
    QVBoxLayout *marginsspinboxlayout1 = new QVBoxLayout;
    marginsspinboxlayout1->addWidget(marginTopSpin);
    marginsspinboxlayout1->addWidget(marginLeftSpin);
    marginsspinboxlayout1->setContentsMargins(0, 0, 10, 0);
    QVBoxLayout *marginslabellayout2 = new QVBoxLayout;
    marginslabellayout2->addWidget(bottomlabel);
    marginslabellayout2->addWidget(rightlabel);
    QVBoxLayout *marginsspinboxlayout2 = new QVBoxLayout;
    marginsspinboxlayout2->addWidget(marginBottomSpin);
    marginsspinboxlayout2->addWidget(marginRightSpin);
    spacer = new QSpacerItem(130, 72);
    marginsspinlayout->addSpacerItem(spacer);
    marginsLayout(true);

    marginsspinlayout->setContentsMargins(20, 0, 0, 0);
    marginsspinlayout->addLayout(marginslabellayout1);
    marginsspinlayout->addLayout(marginsspinboxlayout1);
    marginsspinlayout->addLayout(marginslabellayout2);
    marginsspinlayout->addLayout(marginsspinboxlayout2);
    marginslayout->addLayout(marginscombolayout);
    marginslayout->addLayout(marginsspinlayout);

    QRegExp reg("^([5-5][0-4]|[1-4][0-9]|[0-9])(\\.[0-9][0-9])|55(\\.[8-8][0-8])|55(\\.[0-7][0-9])");
    QRegExpValidator *val = new QRegExpValidator(reg, marginsframe);
    QList<DDoubleSpinBox *> list = marginsframe->findChildren<DDoubleSpinBox *>();
    for (int i = 0; i < list.size(); i++) {
        list.at(i)->setEnabledEmbedStyle(true);
        list.at(i)->lineEdit()->setClearButtonEnabled(false);
        list.at(i)->lineEdit()->setValidator(val);
        list.at(i)->setMaximum(55.88);
    }

    pagelayout->addLayout(pagestitlelayout);
    pagelayout->addWidget(colorframe);
    pagelayout->addWidget(marginsframe);

    //??????
    QVBoxLayout *scalinglayout = new QVBoxLayout;
    scalinglayout->setContentsMargins(10, 0, 10, 0);
    DLabel *scalingLabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Scaling"), advancesettingwdg);
    QHBoxLayout *scalingtitlelayout = new QHBoxLayout;
    scalingtitlelayout->setContentsMargins(10, 20, 0, 0);
    scalingtitlelayout->addWidget(scalingLabel, Qt::AlignLeft | Qt::AlignBottom);
    setwidgetfont(scalingLabel, DFontSizeManager::T5);

    scaleGroup = new QButtonGroup(q);
    QVBoxLayout *scalingcontentlayout = new QVBoxLayout;
    scalingcontentlayout->setContentsMargins(0, 0, 0, 0);

    DWidget *actualwdg = new DWidget;
    actualwdg->setFixedHeight(HEIGHT_NORMAL);
    QHBoxLayout *actuallayout = new QHBoxLayout(actualwdg);
    actuallayout->setContentsMargins(10, 0, 10, 0);
    DRadioButton *actualSizeRadio = new DRadioButton(qApp->translate("DPrintPreviewDialogPrivate", "Actual size"));

    scaleGroup->addButton(actualSizeRadio, ACTUAL_SIZE);
    actuallayout->addWidget(actualSizeRadio);

    DWidget *customscalewdg = new DWidget;
    customscalewdg->setFixedHeight(HEIGHT_NORMAL);
    QHBoxLayout *customlayout = new QHBoxLayout(customscalewdg);
    customlayout->setContentsMargins(10, 0, 10, 0);
    DRadioButton *customSizeRadio = new DRadioButton(qApp->translate("DPrintPreviewDialogPrivate", "Scale"));
    scaleGroup->addButton(customSizeRadio, SCALE);
    scaleRateEdit = new DSpinBox;
    QRegExp scaleReg("^([1-9][0-9]?|[1][0-9]{2}|200)$");
    QRegExpValidator *scaleVal = new QRegExpValidator(scaleReg, scaleRateEdit);
    scaleRateEdit->lineEdit()->setValidator(scaleVal);
    scaleRateEdit->setEnabledEmbedStyle(true);
    scaleRateEdit->setRange(1, 200);
    scaleRateEdit->setFixedWidth(80);
    scaleRateEdit->installEventFilter(q);
    DLabel *scaleLabel = new DLabel("%");
    customlayout->addWidget(customSizeRadio);
    customlayout->addWidget(scaleRateEdit);
    customlayout->addWidget(scaleLabel);
    customlayout->addStretch(1);

    scalingcontentlayout->addWidget(actualwdg);
    scalingcontentlayout->addWidget(customscalewdg);

    DBackgroundGroup *back = new DBackgroundGroup(scalingcontentlayout);
    back->setObjectName("ScalingContentBackgroundGroup");
    back->setItemSpacing(1);
    DPalette pa = DPaletteHelper::instance()->palette(back);
    pa.setBrush(DPalette::Base, pa.itemBackground());
    DPaletteHelper::instance()->setPalette(back, pa);
    scalinglayout->addLayout(scalingtitlelayout);
    scalinglayout->addWidget(back);

    //??????
    QVBoxLayout *paperlayout = new QVBoxLayout;
    paperlayout->setContentsMargins(10, 0, 10, 0);
    DLabel *paperLabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Paper"), advancesettingwdg);
    setwidgetfont(paperLabel, DFontSizeManager::T5);
    QHBoxLayout *papertitlelayout = new QHBoxLayout;
    papertitlelayout->setContentsMargins(10, 0, 0, 0);
    papertitlelayout->addWidget(paperLabel, Qt::AlignLeft | Qt::AlignBottom);

    DFrame *paperframe = new DFrame;
    setfrmaeback(paperframe);
    paperframe->setFixedHeight(HEIGHT_NORMAL);
    QHBoxLayout *paperframelayout = new QHBoxLayout(paperframe);
    DLabel *papersizelabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Paper size"));
    paperSizeCombo = new DComboBox;
    paperSizeCombo->setFixedHeight(36);
    paperframelayout->addWidget(papersizelabel, 4);
    paperframelayout->addWidget(paperSizeCombo, 9);
    paperframelayout->setContentsMargins(10, 4, 10, 4);
    paperlayout->addLayout(papertitlelayout);
    paperlayout->addWidget(paperframe);

    //????????????
    QVBoxLayout *drawinglayout = new QVBoxLayout;
    drawinglayout->setSpacing(10);
    drawinglayout->setContentsMargins(10, 0, 10, 0);
    DLabel *drawingLabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Print Layout"), advancesettingwdg);
    setwidgetfont(drawingLabel, DFontSizeManager::T5);
    QHBoxLayout *drawingtitlelayout = new QHBoxLayout;
    drawingtitlelayout->setContentsMargins(10, 20, 0, 0);
    drawingtitlelayout->addWidget(drawingLabel, Qt::AlignLeft | Qt::AlignBottom);

    DFrame *duplexframe = new DFrame;
    setfrmaeback(duplexframe);
    duplexframe->setFixedHeight(HEIGHT_NORMAL);
    QHBoxLayout *duplexlayout = new QHBoxLayout(duplexframe);
    duplexCombo = new DComboBox;
    duplexCheckBox = new DCheckBox(qApp->translate("DPrintPreviewDialogPrivate", "Duplex"));
    duplexCombo->setFixedHeight(36);
    duplexlayout->setContentsMargins(5, 5, 10, 5);
    duplexlayout->addWidget(duplexCheckBox, 4);
    duplexlayout->addWidget(duplexCombo, 9);

    //????????????
    DFrame *sidebysideframe = new DFrame;
    sidebysideframe->setObjectName("btnframe");
    setfrmaeback(sidebysideframe);
    QVBoxLayout *sidebysideframelayout = new QVBoxLayout(sidebysideframe);
    sidebysideframelayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *pagepersheetlayout = new QHBoxLayout;
    sidebysideCheckBox = new DCheckBox(qApp->translate("DPrintPreviewDialogPrivate", "N-up printing"));
    pagePerSheetCombo = new DComboBox;
    pagePerSheetCombo->addItems(QStringList() << qApp->translate("DPrintPreviewDialogPrivate", "2 pages/sheet, 1??2") << qApp->translate("DPrintPreviewDialogPrivate", "4 pages/sheet, 2??2") << qApp->translate("DPrintPreviewDialogPrivate", "6 pages/sheet, 2??3") << qApp->translate("DPrintPreviewDialogPrivate", "9 pages/sheet, 3??3") << qApp->translate("DPrintPreviewDialogPrivate", "16 pages/sheet, 4??4"));
    pagePerSheetCombo->setFixedHeight(36);
    pagepersheetlayout->setContentsMargins(5, 5, 10, 5);
    pagepersheetlayout->addWidget(sidebysideCheckBox, 4);
    pagepersheetlayout->addWidget(pagePerSheetCombo, 9);
    sidebysideframelayout->addLayout(pagepersheetlayout);

    QHBoxLayout *printdirectlayout = new QHBoxLayout;
    printdirectlayout->setContentsMargins(0, 0, 10, 5);
    DLabel *directlabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Layout direction"));
    DToolButton *lrtbBtn = new DToolButton;
    DToolButton *rltbBtn = new DToolButton;
    DToolButton *tblrBtn = new DToolButton;
    DToolButton *tbrlBtn = new DToolButton;
    DToolButton *repeatBtn = new DToolButton;
    lrtbBtn->setIcon(QIcon::fromTheme("printer_lrtb_1"));
    rltbBtn->setIcon(QIcon::fromTheme("printer_lrtb_2"));
    tblrBtn->setIcon(QIcon::fromTheme("printer_lrtb_3"));
    tbrlBtn->setIcon(QIcon::fromTheme("printer_lrtb_4"));
    repeatBtn->setIcon(QIcon::fromTheme("printer_lrtb_5"));
    DWidget *btnWidget = new DWidget;
    directGroup = new QButtonGroup(q);
    QHBoxLayout *btnLayout = new QHBoxLayout(btnWidget);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    printdirectlayout->addSpacing(30);
    btnLayout->addWidget(rltbBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(lrtbBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(tblrBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(tbrlBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(repeatBtn);
    printdirectlayout->addWidget(directlabel, 2);
    printdirectlayout->addWidget(btnWidget, 5);
    sidebysideframelayout->addLayout(printdirectlayout);
    QList<DToolButton *> listBtn = btnWidget->findChildren<DToolButton *>();
    int num = 0;
    for (DToolButton *btn : listBtn) {
        btn->setIconSize(QSize(18, 18));
        btn->setFixedSize(QSize(36, 36));
        btn->setCheckable(true);
        btn->setEnabled(false);
        directGroup->addButton(btn, num);
        num++;
    }
    drawinglayout->addLayout(drawingtitlelayout);
    drawinglayout->addWidget(duplexframe);
    drawinglayout->addWidget(sidebysideframe);

    //????????????
    QVBoxLayout *orderlayout = new QVBoxLayout;
    orderlayout->setContentsMargins(10, 0, 10, 0);
    DLabel *orderLabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Page Order"), advancesettingwdg);
    setwidgetfont(orderLabel, DFontSizeManager::T5);
    QHBoxLayout *ordertitlelayout = new QHBoxLayout;
    ordertitlelayout->setContentsMargins(0, 20, 0, 0);
    ordertitlelayout->addWidget(orderLabel, Qt::AlignLeft | Qt::AlignBottom);

    QVBoxLayout *ordercontentlayout = new QVBoxLayout;
    ordercontentlayout->setContentsMargins(0, 0, 0, 0);
    DWidget *collatewdg = new DWidget;
    collatewdg->setObjectName("CollateWidget");
    QHBoxLayout *collatelayout = new QHBoxLayout(collatewdg);
    DRadioButton *printCollateRadio = new DRadioButton(qApp->translate("DPrintPreviewDialogPrivate", "Collate pages")); //????????????
    collatelayout->addWidget(printCollateRadio);
    printCollateRadio->setChecked(true);
    inorderwdg = new DWidget;
    inorderwdg->setObjectName("InorderWidget");
    QHBoxLayout *inorderlayout = new QHBoxLayout(inorderwdg);
    printInOrderRadio = new DRadioButton(qApp->translate("DPrintPreviewDialogPrivate", "Print pages in order")); //???????????????
    inorderCombo = new DComboBox;
    inorderCombo->addItems(QStringList() << qApp->translate("DPrintPreviewDialogPrivate", "Front to back") << qApp->translate("DPrintPreviewDialogPrivate", "Back to front"));
    inorderCombo->setEnabled(false);
    inorderlayout->addWidget(printInOrderRadio, 4);
    inorderlayout->addWidget(inorderCombo, 9);

    ordercontentlayout->addWidget(collatewdg);
    ordercontentlayout->addWidget(inorderwdg);

    printOrderGroup = new QButtonGroup(q);
    printOrderGroup->addButton(printCollateRadio, 0);
    printOrderGroup->addButton(printInOrderRadio, 1);

    DBackgroundGroup *backorder = new DBackgroundGroup(ordercontentlayout);
    backorder->setItemSpacing(1);
    pa = DPaletteHelper::instance()->palette(backorder);
    pa.setBrush(DPalette::Base, pa.itemBackground());
    DPaletteHelper::instance()->setPalette(backorder, pa);

    orderlayout->addLayout(ordertitlelayout);
    orderlayout->addWidget(backorder);

    //??????
    QVBoxLayout *watermarklayout = new QVBoxLayout;
    watermarklayout->setContentsMargins(10, 0, 10, 0);
    DLabel *watermarkLabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Watermark"), advancesettingwdg);
    QHBoxLayout *watermarktitlelayout = new QHBoxLayout;
    watermarktitlelayout->setContentsMargins(10, 20, 0, 0);
    watermarktitlelayout->addWidget(watermarkLabel, Qt::AlignLeft | Qt::AlignBottom);
    setwidgetfont(watermarkLabel, DFontSizeManager::T5);

    DFrame *watermarkframe = new DFrame;
    watermarkframe->setObjectName("WaterMarkFrame");
    setfrmaeback(watermarkframe);
    QHBoxLayout *texttypelayout = new QHBoxLayout;
    texttypelayout->setContentsMargins(10, 10, 10, 10);
    DLabel *addlabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Add watermark"));
    waterMarkBtn = new DSwitchButton;
    waterMarkBtn->setChecked(false);
    texttypelayout->addWidget(addlabel, Qt::AlignLeft);
    texttypelayout->addWidget(waterMarkBtn, Qt::AlignRight);
    watermarkframe->setLayout(texttypelayout);

    watermarksettingwdg = new DWidget;
    watermarksettingwdg->setMinimumWidth(WIDTH_NORMAL);
    initWaterMarkui();
    watermarksettingwdg->hide();
    wmSpacer = new QSpacerItem(WIDTH_NORMAL, SPACER_HEIGHT_SHOW);

    watermarklayout->addLayout(watermarktitlelayout);
    watermarklayout->addWidget(watermarkframe);
    watermarklayout->addWidget(watermarksettingwdg);
    watermarklayout->addSpacerItem(wmSpacer);

    layout->addLayout(paperlayout);
    layout->addLayout(drawinglayout);
    layout->addLayout(orderlayout);
    layout->addLayout(pagelayout);
    layout->addLayout(scalinglayout);
    layout->addLayout(watermarklayout);
    layout->addStretch();
}

void DPrintPreviewDialogPrivate::initWaterMarkui()
{
    Q_Q(DPrintPreviewDialog);
    QVBoxLayout *vContentLayout = new QVBoxLayout;
    vContentLayout->setContentsMargins(0, 5, 0, 5);
    vContentLayout->setSpacing(10);
    QVBoxLayout *vWatertypeLayout = new QVBoxLayout;
    textWatermarkWdg = new DWidget;
    picWatermarkWdg = new DWidget;
    vWatertypeLayout->addWidget(textWatermarkWdg);
    vWatertypeLayout->addWidget(picWatermarkWdg);

    QVBoxLayout *textVlayout = new QVBoxLayout;
    textVlayout->setContentsMargins(0, 0, 5, 0);
    QHBoxLayout *hlayout1 = new QHBoxLayout;
    DRadioButton *textBtn = new DRadioButton(qApp->translate("DPrintPreviewDialogPrivate", "Text watermark"));
    waterTextCombo = new DComboBox;
    waterTextCombo->addItems(QStringList() << qApp->translate("DPrintPreviewDialogPrivate", "Confidential") << qApp->translate("DPrintPreviewDialogPrivate", "Draft") << qApp->translate("DPrintPreviewDialogPrivate", "Sample") << qApp->translate("DPrintPreviewDialogPrivate", "Custom"));
    hlayout1->addWidget(textBtn, 4);
    hlayout1->addWidget(waterTextCombo, 9);

    QHBoxLayout *hlayout2 = new QHBoxLayout;
    waterTextEdit = new DLineEdit;
    waterTextEdit->setEnabled(false);
    waterTextEdit->lineEdit()->setMaxLength(16);
    waterTextEdit->lineEdit()->setPlaceholderText(qApp->translate("DPrintPreviewDialogPrivate", "Input your text"));
    hlayout2->addWidget(new DLabel, 4);
    hlayout2->addWidget(waterTextEdit, 9);

    QHBoxLayout *hlayout3 = new QHBoxLayout;
    fontCombo = new DComboBox;

    waterColorBtn = new DIconButton(textWatermarkWdg);
    waterColorBtn->setFixedSize(36, 36);
    waterColor = QColor("#6f6f6f");
    _q_selectColorButton(waterColor);
    isInitBtnColor = true;
    hlayout3->addWidget(new DLabel, 18);
    hlayout3->addWidget(fontCombo, 35);
    hlayout3->addWidget(waterColorBtn, 1);

    textVlayout->addLayout(hlayout1);
    textVlayout->addLayout(hlayout2);
    textVlayout->addLayout(hlayout3);
    textWatermarkWdg->setLayout(textVlayout);

    QHBoxLayout *picHlayout = new QHBoxLayout;
    picHlayout->setContentsMargins(0, 0, 5, 0);
    DRadioButton *picBtn = new DRadioButton(qApp->translate("DPrintPreviewDialogPrivate", "Picture watermark"));
    picPathEdit = new DFileChooserEdit;
    picPathEdit->setNameFilters(QStringList() << "*.png *.jpg");
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    picPathEdit->setDirectoryUrl(QUrl(desktopPath, QUrl::TolerantMode));

    picHlayout->addWidget(picBtn, 4);
    picHlayout->addWidget(picPathEdit, 9);
    picWatermarkWdg->setLayout(picHlayout);

    waterTypeGroup = new QButtonGroup(q);
    waterTypeGroup->addButton(textBtn, 0);
    waterTypeGroup->addButton(picBtn, 1);

    DBackgroundGroup *back = new DBackgroundGroup(vWatertypeLayout);
    back->setItemSpacing(2);

    DFrame *posframe = new DFrame;
    setfrmaeback(posframe);
    posframe->setFixedHeight(HEIGHT_NORMAL);
    QHBoxLayout *posframelayout = new QHBoxLayout(posframe);
    DLabel *poslabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Layout"));
    waterPosCombox = new DComboBox;
    waterPosCombox->addItems(QStringList() << qApp->translate("DPrintPreviewDialogPrivate", "Tile") << qApp->translate("DPrintPreviewDialogPrivate", "Center"));
    waterPosCombox->setCurrentIndex(waterPosCombox->count() - 1);
    waterPosCombox->setFixedHeight(36);
    posframelayout->addWidget(poslabel, 4);
    posframelayout->addWidget(waterPosCombox, 9);
    posframelayout->setContentsMargins(10, 4, 10, 4);

    DFrame *inclinatframe = new DFrame;
    setfrmaeback(inclinatframe);
    inclinatframe->setFixedHeight(HEIGHT_NORMAL);
    QHBoxLayout *inclinatframelayout = new QHBoxLayout(inclinatframe);
    DLabel *inclinatlabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Angle"));
    inclinatBox = new DSpinBox;
    inclinatBox->setSuffix("??");
    inclinatBox->setValue(30);
    inclinatBox->setSingleStep(5);
    inclinatBox->setRange(0, 360);
    inclinatBox->setFixedHeight(36);
    inclinatBox->setEnabledEmbedStyle(true);
    inclinatframelayout->addWidget(inclinatlabel, 4);
    inclinatframelayout->addWidget(inclinatBox, 9);
    inclinatframelayout->setContentsMargins(10, 4, 10, 4);

    DFrame *sizeframe = new DFrame;
    setfrmaeback(sizeframe);
    sizeframe->setFixedHeight(HEIGHT_NORMAL);
    QHBoxLayout *sizeframelayout = new QHBoxLayout(sizeframe);
    DLabel *sizelabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Size"));
    waterSizeSlider = new DSlider;
    sizeBox = new DSpinBox;
    sizeBox->lineEdit()->setReadOnly(true);
    sizeBox->setFixedWidth(78);
    sizeBox->setRange(10, 200);
    sizeBox->setValue(100);
    sizeBox->setSuffix("%");
    sizeBox->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);
    waterSizeSlider->setMaximum(200);
    waterSizeSlider->setValue(100);
    waterSizeSlider->setMinimum(10);
    sizeframelayout->addWidget(sizelabel, 4);
    sizeframelayout->addWidget(waterSizeSlider, 7);
    sizeframelayout->addWidget(sizeBox, 2);
    sizeframelayout->setContentsMargins(10, 4, 10, 4);

    DFrame *opaframe = new DFrame;
    setfrmaeback(opaframe);
    opaframe->setFixedHeight(HEIGHT_NORMAL);
    QHBoxLayout *opaframelayout = new QHBoxLayout(opaframe);
    DLabel *opalabel = new DLabel(qApp->translate("DPrintPreviewDialogPrivate", "Transparency"));
    wmOpaSlider = new DSlider;
    opaBox = new DSpinBox;
    opaBox->lineEdit()->setReadOnly(true);
    opaBox->setFixedWidth(78);
    opaBox->setRange(0, 100);
    opaBox->setValue(30);
    opaBox->setSuffix("%");
    opaBox->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);
    wmOpaSlider->setValue(30);
    wmOpaSlider->setMaximum(100);
    opaframelayout->addWidget(opalabel, 4);
    opaframelayout->addWidget(wmOpaSlider, 7);
    opaframelayout->addWidget(opaBox, 2);
    opaframelayout->setContentsMargins(10, 4, 10, 4);

    vContentLayout->addWidget(back);
    vContentLayout->addWidget(posframe);
    vContentLayout->addWidget(inclinatframe);
    vContentLayout->addWidget(sizeframe);
    vContentLayout->addWidget(opaframe);

    DPalette pa = DPaletteHelper::instance()->palette(back);
    pa.setBrush(DPalette::Base, pa.itemBackground());
    DPaletteHelper::instance()->setPalette(back, pa);
    watermarksettingwdg->setLayout(vContentLayout);
}

/*!
 * \~chinese DPrintPreviewDialogPrivate::initWaterSettings ???????????????????????????
 */
void DPrintPreviewDialogPrivate::initWaterSettings()
{
    Q_EMIT waterPosCombox->currentIndexChanged(waterPosCombox->currentIndex());
    Q_EMIT waterSizeSlider->valueChanged(waterSizeSlider->value());
    Q_EMIT wmOpaSlider->valueChanged(wmOpaSlider->value());
    Q_EMIT inclinatBox->editingFinished();
    if (waterTypeGroup->button(0)->isChecked()) {
        Q_EMIT fontCombo->currentIndexChanged(fontCombo->currentIndex());
        pview->setWaterMarkColor(waterColor);
    }
}

void DPrintPreviewDialogPrivate::marginsLayout(bool adapted)
{
    if (adapted) {
        spacer->changeSize(130, 72);
    } else {
        spacer->changeSize(0, 72);
    }
}

void DPrintPreviewDialogPrivate::initdata()
{
    Q_Q(DPrintPreviewDialog);
    QStringList itemlist;
    itemlist << QPrinterInfo::availablePrinterNames()
             << qApp->translate("DPrintPreviewDialogPrivate", "Print to PDF")
             << qApp->translate("DPrintPreviewDialogPrivate", "Save as Image");
    printDeviceCombo->addItems(itemlist);
    QString defauledevice = QPrinterInfo::defaultPrinterName();
    for (int i = 0; i < itemlist.size(); ++i) {
        if (defauledevice.compare(itemlist.at(i)) == 0) {
            printDeviceCombo->setCurrentIndex(i);
            break;
        }
    }
    _q_pageRangeChanged(0);
    _q_pageMarginChanged(0);
    _q_printerChanged(printDeviceCombo->currentIndex());
    scaleGroup->button(1)->setChecked(true);
    orientationgroup->button(0)->setChecked(true);
    scaleRateEdit->setValue(100);
    scaleRateEdit->setEnabled(false);
    duplexCombo->setEnabled(false);
    pagePerSheetCombo->setEnabled(false);
    isInited = true;
    fontSizeMore = true;
}

void DPrintPreviewDialogPrivate::initconnections()
{
    Q_Q(DPrintPreviewDialog);

    QObject::connect(pview, QOverload<DPrinter *>::of(&DPrintPreviewWidget::paintRequested), q, QOverload<DPrinter *>::of(&DPrintPreviewDialog::paintRequested));
    QObject::connect(pview, QOverload<DPrinter *, const QVector<int> &>::of(&DPrintPreviewWidget::paintRequested), q, QOverload<DPrinter *, const QVector<int> &>::of(&DPrintPreviewDialog::paintRequested));

    QObject::connect(advanceBtn, &QPushButton::clicked, q, [this] { this->showadvancesetting(); });
    QObject::connect(printDeviceCombo, SIGNAL(currentIndexChanged(int)), q, SLOT(_q_printerChanged(int)));
    QObject::connect(cancelBtn, &DPushButton::clicked, q, &DPrintPreviewDialog::close);
    QObject::connect(pageRangeCombo, SIGNAL(currentIndexChanged(int)), q, SLOT(_q_pageRangeChanged(int)));
    QObject::connect(marginsCombo, SIGNAL(currentIndexChanged(int)), q, SLOT(_q_pageMarginChanged(int)));
    QObject::connect(printBtn, SIGNAL(clicked(bool)), q, SLOT(_q_startPrint(bool)));
    QObject::connect(waterColorBtn, SIGNAL(clicked(bool)), q, SLOT(_q_colorButtonCliked(bool)));
    QObject::connect(colorModeCombo, SIGNAL(currentIndexChanged(int)), q, SLOT(_q_ColorModeChange(int)));
    QObject::connect(orientationgroup, SIGNAL(buttonClicked(int)), q, SLOT(_q_orientationChanged(int)));
    QObject::connect(waterTextCombo, SIGNAL(currentIndexChanged(int)), q, SLOT(_q_textWaterMarkModeChanged(int)));
    QObject::connect(inorderCombo, SIGNAL(currentIndexChanged(int)), q, SLOT(_q_printOrderComboIndexChanged(int)));
    QObject::connect(waterTextEdit, SIGNAL(editingFinished()), q, SLOT(_q_customTextWatermarkFinished()));
    QObject::connect(pagePerSheetCombo, SIGNAL(currentIndexChanged(int)), q, SLOT(_q_pagePersheetComboIndexChanged(int)));
    QObject::connect(picPathEdit, &DFileChooserEdit::fileChoosed, q, [=](const QString &filename) { customPictureWatermarkChoosed(filename); });
    QObject::connect(sizeBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), q, [=](int value) {
        waterSizeSlider->setValue(value);
    });
    QObject::connect(opaBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), q, [=](int value) {
        wmOpaSlider->setValue(value);
    });
    QObject::connect(fontCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), q, [=] {
        QFont font(fontCombo->currentText());
        font.setPointSize(WATERFONT_SIZE);
        pview->setWaterMarkFont(font);
    });
    QObject::connect(pickColorWidget, SIGNAL(selectColorButton(QColor)), q, SLOT(_q_selectColorButton(QColor)));
    QObject::connect(waterPosCombox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), q, [=](int index) {
        if (index == waterPosCombox->count() - 1) {
            pview->setWaterMarkLayout(WATERLAYOUT_CENTER);
        } else {
            pview->setWaterMarkLayout(WATERLAYOUT_TILED);
        }
    });
    QObject::connect(directGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), q, [=](int index) {
        directGroup->button(index)->setChecked(true);
        directChoice = index;
        pview->setOrder(DPrintPreviewWidget::Order(index));
    });
    QObject::connect(inclinatBox, &QSpinBox::editingFinished, q, [=]() {
        _d_setSpinboxDefaultValue(spinboxTextCaches, inclinatBox);
        pview->setWaterMarkRotate(inclinatBox->value());
    });
    QObject::connect(waterSizeSlider, &DSlider::valueChanged, q, [=](int value) {
        sizeBox->setValue(value);
        qreal m_value = static_cast<qreal>(value) / 100.00;
        pview->setWaterMarkScale(m_value);
    });
    QObject::connect(wmOpaSlider, &DSlider::valueChanged, q, [=](int value) {
        opaBox->setValue(value);
        qreal m_value = static_cast<qreal>(value) / 100.00;
        pview->setWaterMarkOpacity(m_value);
    });
    QObject::connect(printOrderGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), q, [=](int index) {
        Q_Q(DPrintPreviewDialog);
        if (index == 0) {
            inorderCombo->setEnabled(false);
            // ???????????????????????????????????? ?????????????????????
            if (q->printFromPath().isEmpty())
                q->findChild<DFrame *>("btnframe")->setEnabled(true);
        } else {
            inorderCombo->setEnabled(true);
            q->findChild<DFrame *>("btnframe")->setEnabled(false);
        }
    });
    QObject::connect(waterMarkBtn, &DSwitchButton::clicked, q, [=](bool isClicked) { this->waterMarkBtnClicked(isClicked); });
    QObject::connect(waterTypeGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), q, [=](int index) { this->watermarkTypeChoosed(index); });
    QObject::connect(pageRangeEdit, &DLineEdit::editingFinished, [=] {
        _q_customPagesFinished();
    });
    QObject::connect(pageRangeEdit, &DLineEdit::focusChanged, q, [this](bool onFocus) {
        isOnFocus = true;
        if (pageRangeEdit->text().right(1) == "-" && !onFocus) {
            this->_q_customPagesFinished();
        } else if (pageRangeEdit->text().isEmpty() && !onFocus) {
            pageRangeError(NullTip);
        }
    });
    QObject::connect(sidebysideCheckBox, &DCheckBox::stateChanged, q, [=](int status) {
        if (status == 0) {
            if (printDeviceCombo->currentIndex() < printDeviceCombo->count() - 2)
                inorderwdg->setEnabled(true);
            setPageLayoutEnable(false);
            pview->setImposition(DPrintPreviewWidget::One);
            originTotalPageLabel->setVisible(false);
        } else {
            inorderwdg->setEnabled(false);
            inorderCombo->setEnabled(false);
            printOrderGroup->button(0)->setChecked(true);
            setPageLayoutEnable(true);
            directGroup->button(directChoice)->setChecked(true);
            _q_pagePersheetComboIndexChanged(pagePerSheetCombo->currentIndex());
            originTotalPageLabel->setVisible(true);
        }
    });
    QObject::connect(jumpPageEdit->lineEdit(), &QLineEdit::textChanged, q, [ = ](QString str) {
        if (str.toInt() > totalPageLabel->text().toInt())
            jumpPageEdit->lineEdit()->setText(totalPageLabel->text());
    });

    QObject::connect(pview, &DPrintPreviewWidget::totalPages, [this](int pages) {
        int targetPage = pview->targetPageCount(pages);
        jumpPageEdit->setRange(FIRST_PAGE, targetPage);
        totalPageLabel->setText(QString::number(targetPage));
        totalPages = pages;
        originTotalPageLabel->setText(QString("(%1)").arg(pages));
        if (sidebysideCheckBox->isChecked())
            originTotalPageLabel->setVisible(true);
        else
            originTotalPageLabel->setVisible(false);
        jumpPageEdit->setMaximum(targetPage);
        setTurnPageBtnStatus();
    });
    QObject::connect(pview, &DPrintPreviewWidget::pagesCountChanged, [this](int pages) {
        totalPageLabel->setNum(pview->targetPageCount(pages));
        if (pview->isAsynPreview()) {
            originTotalPageLabel->setText(QString("(%1)").arg(pview->originPageCount()));
            if (sidebysideCheckBox->isChecked())
                originTotalPageLabel->setVisible(true);
            else
                originTotalPageLabel->setVisible(false);
        }
        setTurnPageBtnStatus();
    });
    QObject::connect(firstBtn, &DIconButton::clicked, pview, &DPrintPreviewWidget::turnBegin);
    QObject::connect(prevPageBtn, &DIconButton::clicked, pview, &DPrintPreviewWidget::turnFront);
    QObject::connect(nextPageBtn, &DIconButton::clicked, pview, &DPrintPreviewWidget::turnBack);
    QObject::connect(lastBtn, &DIconButton::clicked, pview, &DPrintPreviewWidget::turnEnd);
    QObject::connect(pview, &DPrintPreviewWidget::currentPageChanged, q, [this](int page) {
        if (jumpPageEdit->value() == page)
            return;
        jumpPageEdit->setValue(page);
        setTurnPageBtnStatus();
    });
    QObject::connect(jumpPageEdit->lineEdit(), &QLineEdit::editingFinished, q, [this]() {
        _d_setSpinboxDefaultValue(spinboxTextCaches, jumpPageEdit);
        pview->setCurrentPage(jumpPageEdit->value());
        setTurnPageBtnStatus();
    });
    QObject::connect(paperSizeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), q, [this](int) {
        QPrinterInfo prInfo(*printer);
        if (paperSizeCombo->count() == 0) {
            printer->setPageSize(QPrinter::A4);
            return ;
        }
        if (printDeviceCombo->currentIndex() != printDeviceCombo->count() - 1 && printDeviceCombo->currentIndex() != printDeviceCombo->count() - 2) {
            printer->setPageSize(prInfo.supportedPageSizes().at(paperSizeCombo->currentIndex()));
        } else {
            switch (paperSizeCombo->currentIndex()) {
            case 0:
                printer->setPageSize(QPrinter::A3);
                break;
            case 1:
                printer->setPageSize(QPrinter::A4);
                break;
            case 2:
                printer->setPageSize(QPrinter::A5);
                break;
            case 3:
                printer->setPageSize(QPrinter::B4);
                break;
            case 4:
                printer->setPageSize(QPrinter::B5);
                break;
            case 5:
                printer->setPageSize(QPrinter::Custom);
                printer->setPageSizeMM(QSizeF(EightK_Weight, EightK_Height));
                break;
            case 6:
                printer->setPageSize(QPrinter::Custom);
                printer->setPageSizeMM(QSizeF(SixteenK_Weight, SixteenK_Height));
                break;
            }
        }
        if (isInited) {
            this->marginsUpdate(false);
        }
        if (pview->pageRangeMode() == DPrintPreviewWidget::SelectPage)
            pageRangeCombo->setCurrentIndex(PAGERANGE_ALL);

    });

    QObject::connect(scaleRateEdit->lineEdit(), &QLineEdit::editingFinished, q, [=] {
        if (scaleGroup->checkedId() == SCALE) {
            _d_setSpinboxDefaultValue(spinboxTextCaches, scaleRateEdit);
            if (scaleRateEdit->value() < 10)
                scaleRateEdit->setValue(10);
            qreal scale = scaleRateEdit->value() / 100.0;
            pview->setScale(scale);
            pview->updateView();
        }
    });
    QObject::connect(scaleGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), q, [this](int id) {
        if (id == ACTUAL_SIZE) {
            pview->setScale(1);
            scaleRateEdit->setEnabled(false);
        } else if (id == SCALE) {
            qreal scale = scaleRateEdit->value() / 100.0;
            pview->setScale(scale);
            scaleRateEdit->setEnabled(true);
        }
        pview->updateView();
    });

    QObject::connect(copycountspinbox, &QSpinBox::editingFinished, q, [this]() {
        _d_setSpinboxDefaultValue(spinboxTextCaches, copycountspinbox);
    });

    QObject::connect(marginTopSpin, SIGNAL(valueChanged(double)), q, SLOT(_q_marginspinChanged(double)));
    QObject::connect(marginRightSpin, SIGNAL(valueChanged(double)), q, SLOT(_q_marginspinChanged(double)));
    QObject::connect(marginLeftSpin, SIGNAL(valueChanged(double)), q, SLOT(_q_marginspinChanged(double)));
    QObject::connect(marginBottomSpin, SIGNAL(valueChanged(double)), q, SLOT(_q_marginspinChanged(double)));
    QObject::connect(duplexCheckBox, SIGNAL(stateChanged(int)), q, SLOT(_q_checkStateChanged(int)));
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, pview, &DPrintPreviewWidget::themeTypeChanged);
    QObject::connect(marginTopSpin, SIGNAL(editingFinished()), q, SLOT(_q_marginEditFinished()));
    QObject::connect(marginRightSpin, SIGNAL(editingFinished()), q, SLOT(_q_marginEditFinished()));
    QObject::connect(marginLeftSpin, SIGNAL(editingFinished()), q, SLOT(_q_marginEditFinished()));
    QObject::connect(marginBottomSpin, SIGNAL(editingFinished()), q, SLOT(_q_marginEditFinished()));
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, q, [this](DGuiApplicationHelper::ColorType themeType) { this->themeTypeChange(themeType); });
    QObject::connect(marginTopSpin->lineEdit(), SIGNAL(textEdited(const QString &)), q, SLOT(_q_spinboxValueEmptyChecked(const QString &)));
    QObject::connect(marginRightSpin->lineEdit(), SIGNAL(textEdited(const QString &)), q, SLOT(_q_spinboxValueEmptyChecked(const QString &)));
    QObject::connect(marginLeftSpin->lineEdit(), SIGNAL(textEdited(const QString &)), q, SLOT(_q_spinboxValueEmptyChecked(const QString &)));
    QObject::connect(marginBottomSpin->lineEdit(), SIGNAL(textEdited(const QString &)), q, SLOT(_q_spinboxValueEmptyChecked(const QString &)));
    QObject::connect(jumpPageEdit->lineEdit(), SIGNAL(textEdited(const QString &)), q, SLOT(_q_spinboxValueEmptyChecked(const QString &)));
    QObject::connect(copycountspinbox->lineEdit(), SIGNAL(textEdited(const QString &)), q, SLOT(_q_spinboxValueEmptyChecked(const QString &)));
    QObject::connect(scaleRateEdit->lineEdit(), SIGNAL(textEdited(const QString &)), q, SLOT(_q_spinboxValueEmptyChecked(const QString &)));
    QObject::connect(inclinatBox->lineEdit(), SIGNAL(textEdited(const QString &)), q, SLOT(_q_spinboxValueEmptyChecked(const QString &)));
}

void DPrintPreviewDialogPrivate::setfrmaeback(DFrame *frame)
{
    frame->setLineWidth(0);
    DPalette pa = DPaletteHelper::instance()->palette(frame);
    pa.setBrush(DPalette::Base, pa.itemBackground());
    DPaletteHelper::instance()->setPalette(frame, pa);
}

void DPrintPreviewDialogPrivate::showadvancesetting()
{
    if (advancesettingwdg->isHidden()) {
        advancesettingwdg->show();
        advanceBtn->setText(qApp->translate("DPrintPreviewDialogPrivate", "Collapse"));
        advanceBtn->setIcon(QIcon::fromTheme("printer_dropup"));
    } else {
        advancesettingwdg->hide();
        advanceBtn->setText(qApp->translate("DPrintPreviewDialogPrivate", "Advanced"));
        advanceBtn->setIcon(QIcon::fromTheme("printer_dropdown"));
    }
}

/*!
 * \~chinese DPrintPreviewDialogPrivate::setupPrinter ????????????????????????????????????????????????
 */
void DPrintPreviewDialogPrivate::setupPrinter()
{
    //????????????
    //??????????????????
    printer->setCopyCount(copycountspinbox->value());
    //??????????????????
    if (orientationgroup->checkedId() == 0)
        printer->setOrientation(QPrinter::Portrait);
    else {
        printer->setOrientation(QPrinter::Landscape);
    }
    //????????????
    //??????????????????
    QPrinterInfo prInfo(*printer);
    if (printDeviceCombo->currentIndex() != printDeviceCombo->count() - 1
        && printDeviceCombo->currentIndex() != printDeviceCombo->count() - 2) {
        printer->setPageSize(prInfo.supportedPageSizes().at(paperSizeCombo->currentIndex()));
    } else {
        switch (paperSizeCombo->currentIndex()) {
        case 0:
            printer->setPageSize(QPrinter::A3);
            break;
        case 1:
            printer->setPageSize(QPrinter::A4);
            break;
        case 2:
            printer->setPageSize(QPrinter::A5);
            break;
        case 3:
            printer->setPageSize(QPrinter::B4);
            break;
        case 4:
            printer->setPageSize(QPrinter::B5);
            break;
        case 5:
            printer->setPageSize(QPrinter::Custom);
            printer->setPageSizeMM(QSizeF(EightK_Weight, EightK_Height));
            break;
        case 6:
            printer->setPageSize(QPrinter::Custom);
            printer->setPageSizeMM(QSizeF(SixteenK_Weight, SixteenK_Height));
            break;
        }
    }
    //??????????????????
    if (duplexCheckBox->isChecked()) {
        if (duplexCombo->count() == 1) {
            if (supportedDuplexFlag)
                printer->setDuplex(QPrinter::DuplexLongSide);
            else
                printer->setDuplex(QPrinter::DuplexShortSide);
        } else {
            switch (duplexCombo->currentIndex()) {
            case 0:
                printer->setDuplex(QPrinter::DuplexLongSide);
                break;
            case 1:
                printer->setDuplex(QPrinter::DuplexShortSide);
                break;
            }
        }
    } else {
        printer->setDuplex(QPrinter::DuplexNone);
    }
    //??????????????????
    if (supportedColorMode) {
        QByteArray currentColorModel = pview->printerColorModel().isEmpty() ? QByteArrayLiteral("RGB") : pview->printerColorModel();
        QCUPSSupport::setCupsOption(printer, "ColorModel", currentColorModel);
    } else {
        QCUPSSupport::setCupsOption(printer, "ColorModel", "Gray");
    }
    //????????????????????????
    printer->setPageMargins(QMarginsF(marginLeftSpin->value(), marginTopSpin->value(), marginRightSpin->value(), marginBottomSpin->value()), QPageLayout::Millimeter);
    //??????????????????????????????????????????????????????
    if (printOrderGroup->checkedId() == 1) {
        //???????????????????????????????????????????????????
        bool isFirst = true;
        if (inorderCombo->currentIndex() == 1) {
            isFirst = false;
        }

        pview->isPageByPage(printer->copyCount(), isFirst);
        //????????????????????????????????????????????????????????????????????????????????????1
        printer->setCopyCount(1);
    }
}

void DPrintPreviewDialogPrivate::judgeSupportedAttributes(const QString &lastPaperSize)
{
    Q_Q(DPrintPreviewDialog);
    QPrinterInfo updateinfo(*printer);

    QStringList pageSizeList;
    for (int i = 0; i < updateinfo.supportedPageSizes().size(); i++) {
        pageSizeList.append(updateinfo.supportedPageSizes().at(i).key());
    }
    paperSizeCombo->addItems(pageSizeList);
    if (pageSizeList.contains(lastPaperSize)) {
        paperSizeCombo->setCurrentText(lastPaperSize);
    } else {
        //??????????????????
        paperSizeCombo->blockSignals(false);
        paperSizeCombo->setCurrentText("A4");
    }

    //????????????????????????????????????????????????????????????????????????????????????pdf????????????
    QString lastDuplexComboText = duplexCombo->currentText();
    duplexCombo->clear();
    if (updateinfo.supportedDuplexModes().contains(QPrinter::DuplexLongSide) || updateinfo.supportedDuplexModes().contains(QPrinter::DuplexShortSide)) {
        duplexCheckBox->setEnabled(true);
        if (!updateinfo.supportedDuplexModes().contains(QPrinter::DuplexLongSide)) {
            duplexCombo->addItem(qApp->translate("DPrintPreviewDialogPrivate", "Flip on short edge"));
            supportedDuplexFlag = false;
        } else if (!updateinfo.supportedDuplexModes().contains(QPrinter::DuplexShortSide)) {
            duplexCombo->addItem(qApp->translate("DPrintPreviewDialogPrivate", "Flip on long edge"));
            supportedDuplexFlag = true;
        } else if (updateinfo.supportedDuplexModes().contains(QPrinter::DuplexLongSide) && updateinfo.supportedDuplexModes().contains(QPrinter::DuplexShortSide)) {
            duplexCombo->addItem(qApp->translate("DPrintPreviewDialogPrivate", "Flip on long edge"));
            duplexCombo->addItem(qApp->translate("DPrintPreviewDialogPrivate", "Flip on short edge"));
            duplexCombo->setCurrentText(lastDuplexComboText);
        }
    } else {
        duplexCheckBox->setChecked(false);
        duplexCheckBox->setEnabled(false);
        duplexCombo->setEnabled(false);
    }
}

/*!
 * \~chinese DPrintPreviewDialogPrivate::setMininumMargins ??????????????????????????????,?????????????????????
 */
void DPrintPreviewDialogPrivate::setMininumMargins()
{
    if (marginLeftSpin->value() < minnumMargins.first())
        marginLeftSpin->setValue(minnumMargins.first());
    if (marginTopSpin->value() < minnumMargins.at(1))
        marginTopSpin->setValue(minnumMargins.at(1));
    if (marginRightSpin->value() < minnumMargins.at(2))
        marginRightSpin->setValue(minnumMargins.at(2));
    if (marginBottomSpin->value() < minnumMargins.last())
        marginBottomSpin->setValue(minnumMargins.last());
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::marginsUpdate ???????????????????????????????????????????????????????????????????????????
 * \~chinese \param isPrinterchanged ?????????????????????????????????????????????
 */
void DPrintPreviewDialogPrivate::marginsUpdate(bool isPrinterChanged)
{
    //???????????????,?????????????????????,?????????????????????,??????????????????,????????????????????????,???????????????????????????,????????????
    minnumMargins.clear();
    QMarginsF minumMargin = printer->pageLayout().minimumMargins();
    minnumMargins << minumMargin.left() << minumMargin.top() << minumMargin.right() << minumMargin.bottom();
    //???????????????,???????????????????????????????????????,??????????????????
    if ((marginsCombo->currentIndex() == 0)) {
        _q_pageMarginChanged(0);
    } else if (marginsCombo->currentIndex() == marginsCombo->count() - 1) {
        //?????????????????????????????????,???????????????????????????????????????printer???????????????,??????????????????,??????????????????,??????,?????????????????????????????????,???????????????,??????????????????,??????????????????
        if (marginLeftSpin->value() < minumMargin.left() || marginTopSpin->value() < minumMargin.top() || marginRightSpin->value() < minumMargin.right() || marginBottomSpin->value() < minumMargin.bottom()) {
            setMininumMargins();
            QMarginsF currentMargin = QMarginsF(marginLeftSpin->value(), marginTopSpin->value(), marginRightSpin->value(), marginBottomSpin->value());
            printer->setPageMargins(currentMargin, QPageLayout::Millimeter);
            pview->updatePreview();
        } else {
            if (!isPrinterChanged)
                pview->updatePreview();
        }
    } else {
        //??????????????????????????????,??????????????????,??????????????????
        if (!isPrinterChanged)
            pview->updatePreview();
    }
}

void DPrintPreviewDialogPrivate::themeTypeChange(DGuiApplicationHelper::ColorType themeType)
{
    Q_Q(DPrintPreviewDialog);
    DWidget *titleWidget = q->findChild<DWidget *>("titlewidget");
    DPalette m_pa = DPaletteHelper::instance()->palette(titleWidget);
    m_pa.setBrush(DPalette::Background, m_pa.base());
    DPaletteHelper::instance()->setPalette(titleWidget, m_pa);
    titleWidget->setAutoFillBackground(true);

    QList<DFrame *> m_frameList = q->findChildren<DFrame *>();
    QList<DBackgroundGroup *> m_back = q->findChildren<DBackgroundGroup *>();

    DPalette pa;
    if (themeType == DGuiApplicationHelper::LightType) {
        pa.setBrush(DPalette::Base, QColor(0, 0, 0, 8));
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        pa.setBrush(DPalette::Base, QColor(255, 255, 255, 10));
    }
    pa.setBrush(DPalette::FrameBorder, pa.base());
    for (int i = 1; i < m_frameList.size(); i++) {
        DPaletteHelper::instance()->setPalette(m_frameList.at(i), pa);
    }
    for (int i = 0; i < m_back.size(); i++) {
        DPaletteHelper::instance()->setPalette(m_back.at(i), pa);
    }
}

/*!
 * \~chinese DPrintPreviewDialogPrivate::setPageIsLegal ?????????????????????????????????,?????????????????????????????????,????????????????????????
 */
void DPrintPreviewDialogPrivate::setPageIsLegal(bool islegal)
{
    printBtn->setEnabled(islegal);
    pageRangeEdit->setAlert(!islegal);
}

void DPrintPreviewDialogPrivate::tipSelected(TipsNum tipNum)
{
    switch (tipNum) {
    case NullTip:
        pageRangeEdit->showAlertMessage(qApp->translate("DPrintPreviewDialogPrivate", "Input page numbers please"), static_cast<DFrame *>(pageRangeEdit->parent()), 2000);
        break;
    case MaxTip:
        pageRangeEdit->showAlertMessage(qApp->translate("DPrintPreviewDialogPrivate", "Maximum page number reached"), static_cast<DFrame *>(pageRangeEdit->parent()), 2000);
        break;
    case CommaTip:
        pageRangeEdit->showAlertMessage(qApp->translate("DPrintPreviewDialogPrivate", "Input English comma please"), static_cast<DFrame *>(pageRangeEdit->parent()), 2000);
        break;
    case FormatTip:
        pageRangeEdit->showAlertMessage(qApp->translate("DPrintPreviewDialogPrivate", "Input page numbers like this: 1,3,5-7,11-15,18,21"), static_cast<DFrame *>(pageRangeEdit->parent()), 2000);
        break;
    default:
        break;
    }
}

QVector<int> DPrintPreviewDialogPrivate::checkDuplication(QVector<int> data)
{
    for (int i = 0; i < data.size(); i++) {
        for (int j = i + 1; j < data.size(); j++) {
            if (data.at(i) == data.at(j)) {
                data.remove(j);
                j--;
            }
        }
    }
    qSort(data.begin(), data.end());
    return data;
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::setEnable ??????????????????????????????????????????
 * \~chinese \param value ???????????????????????????
 * \~chinese \param combox ??????????????????combox
 */
void DPrintPreviewDialogPrivate::setEnable(const int &value, DComboBox *combox)
{
    if (combox == pageRangeCombo) {
        if (value != pageRangeCombo->count() - 1) {
            pageRangeEdit->setEnabled(false);
        } else {
            pageRangeEdit->setEnabled(true);
        }
    }
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::setEnable ????????????????????????
 */
void DPrintPreviewDialogPrivate::setTurnPageBtnStatus()
{
    int currentPage = jumpPageEdit->value();
    int totalPage = totalPageLabel->text().toInt();
    if (currentPage > 1 && currentPage < totalPage) {
        firstBtn->setEnabled(true);
        prevPageBtn->setEnabled(true);
        nextPageBtn->setEnabled(true);
        lastBtn->setEnabled(true);
    } else if (currentPage == 1 && currentPage == totalPage) {
        firstBtn->setEnabled(false);
        prevPageBtn->setEnabled(false);
        nextPageBtn->setEnabled(false);
        lastBtn->setEnabled(false);
    } else if (currentPage == 1) {
        firstBtn->setEnabled(false);
        prevPageBtn->setEnabled(false);
        nextPageBtn->setEnabled(true);
        lastBtn->setEnabled(true);
    } else if (totalPage == 0) {
        firstBtn->setEnabled(false);
        prevPageBtn->setEnabled(false);
        nextPageBtn->setEnabled(false);
        lastBtn->setEnabled(false);
    } else if (currentPage == totalPage) {
        firstBtn->setEnabled(true);
        prevPageBtn->setEnabled(true);
        nextPageBtn->setEnabled(false);
        lastBtn->setEnabled(false);
    }
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::watermarkTypeChoosed ??????????????????
 * \~chinese \param index ???????????????????????????
 */
void DPrintPreviewDialogPrivate::watermarkTypeChoosed(int index)
{
    if (index == 0) {
        pview->refreshBegin();
        waterTextCombo->setEnabled(true);
        fontCombo->setEnabled(true);
        picPathEdit->setEnabled(false);
        if (colorModeCombo->count() == 2 && colorModeCombo->currentIndex() == colorModeCombo->count() - 2)
            waterColorBtn->setEnabled(true);
        _q_textWaterMarkModeChanged(waterTextCombo->currentIndex());
        initWaterSettings();
        //??????????????????????????????
        QFontDatabase fdb;
        QStringList fontList = fdb.families(QFontDatabase::Any);
        Q_FOREACH (const QString &font, fontList) {
            if (fontCombo->findText(font) != -1)
                continue;

            fontCombo->addItem(font);
        }

        static bool watermarkIsInited = false;
        if (!watermarkIsInited) {
            // ???????????????????????????????????? ????????????????????????????????????
            //??????????????????,???????????????????????????????????????????????????
            QFont font;
            QFontInfo fontName(font);
            QString defaultFontName = fontName.family();
            //????????????????????????????????????????????????
            for (QString itemName : fontList) {
                if (itemName == defaultFontName) {
                    fontCombo->setCurrentText(itemName);
                }
            }
            watermarkIsInited = true;
        }
        pview->setWaterMarkType(Type_Text);
        pview->refreshEnd();
    } else if (index == 1) {
        waterTextCombo->setEnabled(false);
        fontCombo->setEnabled(false);
        waterColorBtn->setEnabled(false);
        waterTextEdit->setEnabled(false);
        picPathEdit->setEnabled(true);
        pview->setWaterMarkType(Type_Image);
    }
    typeChoice = index;
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_printerChanged ??????????????????????????????????????????????????????
 * \~chinese \param index ???????????????????????????
 */
void DPrintPreviewDialogPrivate::_q_printerChanged(int index)
{
    Q_Q(DPrintPreviewDialog);
    QString lastPaperSize = paperSizeCombo->currentText();
    QString lastColormode = colorModeCombo->currentText();
    paperSizeCombo->clear();
    paperSizeCombo->blockSignals(true);
    colorModeCombo->blockSignals(true);
    if (index >= printDeviceCombo->count() - 2) {
        //pdf
        copycountspinbox->setDisabled(true);
        copycountspinbox->setValue(1);
        duplexCheckBox->setCheckState(Qt::Unchecked);
        duplexCheckBox->setEnabled(false);
        duplexCombo->clear();
        duplexCombo->setEnabled(false);
        waterColorBtn->setEnabled(true);
        if (colorModeCombo->count() == 1)
            colorModeCombo->insertItem(0, qApp->translate("DPrintPreviewDialogPrivate", "Color"));
        colorModeCombo->blockSignals(false);
        colorModeCombo->setCurrentIndex(0);
        colorModeCombo->setEnabled(false);
        supportedColorMode = true;
        printBtn->setText(qApp->translate("DPrintPreviewDialogPrivate", "Save", "button"));
        paperSizeCombo->setCurrentIndex(1);
        QStringList pdfPaperSize = QStringList() << "A3"
                                                 << "A4"
                                                 << "A5"
                                                 << "B4"
                                                 << "B5"
                                                 << "8K"
                                                 << "16K";
        paperSizeCombo->addItems(pdfPaperSize);
        if (pdfPaperSize.contains(lastPaperSize))
            paperSizeCombo->setCurrentText(lastPaperSize);
        else {
            //??????????????????
            paperSizeCombo->blockSignals(false);
            paperSizeCombo->setCurrentIndex(1);
        }
        printer->setPrinterName("");
        printOrderGroup->button(0)->setChecked(true);
        inorderwdg->setEnabled(false);
        if (!isInited) {
            waterColor = QColor("#6f6f6f");
            _q_selectColorButton(waterColor);
            pickColorWidget->convertColor(waterColor);
            pickColorWidget->setRgbEdit(waterColor);
        }
    } else {
        //actual printer
        if (printer) {
            if (q->printFromPath().isEmpty() && !sidebysideCheckBox->isChecked()) {
                inorderwdg->setEnabled(true);
            } else {
                inorderwdg->setEnabled(false);
            }
            copycountspinbox->setDisabled(false);
            paperSizeCombo->setEnabled(true);
            colorModeCombo->setEnabled(true);
            printer->setPrinterName(printDeviceCombo->itemText(index));
            printBtn->setText(qApp->translate("DPrintPreviewDialogPrivate", "Print", "button"));
            judgeSupportedAttributes(lastPaperSize);
        }
        //??????????????????????????????????????????????????????????????????????????????????????????????????????pdf????????????
        QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
        QPrintDevice currentDevice = ps->createPrintDevice(printDeviceCombo->currentText());
        colorModeCombo->clear();
        if (!currentDevice.supportedColorModes().contains(QPrint::Color)) {
            colorModeCombo->blockSignals(false);
            colorModeCombo->addItem(qApp->translate("DPrintPreviewDialogPrivate", "Grayscale"));
            supportedColorMode = false;
            waterColorBtn->setEnabled(false);
            waterColor = QColor("#6f6f6f");
            _q_selectColorButton(waterColor);
            pickColorWidget->convertColor(waterColor);
            pickColorWidget->setRgbEdit(waterColor);
        } else {
            if (!isInited) {
                waterColor = QColor("#6f6f6f");
                _q_selectColorButton(waterColor);
                pickColorWidget->convertColor(waterColor);
                pickColorWidget->setRgbEdit(waterColor);
            }

            colorModeCombo->addItems(QStringList() << qApp->translate("DPrintPreviewDialogPrivate", "Color") << qApp->translate("DPrintPreviewDialogPrivate", "Grayscale"));
            colorModeCombo->blockSignals(false);
            if (colorModeCombo->currentText() == lastColormode) {
                colorModeCombo->setCurrentIndex(0);
                supportedColorMode = true;
            } else {
                colorModeCombo->setCurrentIndex(1);
                supportedColorMode = false;
            }
            if (colorModeCombo->currentIndex() == colorModeCombo->count() - 2) {
                waterColorBtn->setEnabled(true);
            }
        }
    }
    marginsUpdate(true);
    if (pview->pageRangeMode() == DPrintPreviewWidget::SelectPage)
        pageRangeCombo->setCurrentIndex(PAGERANGE_ALL);
    paperSizeCombo->blockSignals(false);
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::slotPageRangeCombox ?????????????????????????????????????????????????????????
 * \~chinese \param index ???????????????????????????
 */
void DPrintPreviewDialogPrivate::_q_pageRangeChanged(int index)
{
    Q_Q(DPrintPreviewDialog);
    setEnable(index, pageRangeCombo);
    pageRangeEdit->lineEdit()->setPlaceholderText("");
    pageRangeEdit->setText("");
    if (index == DPrintPreviewWidget::AllPage || index == DPrintPreviewWidget::CurrentPage) {
        pview->setPageRangeMode((DPrintPreviewWidget::PageRange)index);
        setPageIsLegal(true);
        if (totalPages != 0) {
            totalPageLabel->setNum(pview->targetPageCount(totalPages));
            if (isInited) {
                pview->setPageRange(FIRST_PAGE, pview->originPageCount());
            }
            if (index == DPrintPreviewWidget::AllPage)
                pview->setCurrentPage(FIRST_PAGE);
        }
    } else {
        pview->setPageRangeMode(DPrintPreviewWidget::SelectPage);
        if (lastPageRange.isEmpty()) {
            setPageIsLegal(false);

        } else {
            pageRangeEdit->setText(lastPageRange);
            _q_customPagesFinished();
        }
        if (pageRangeEdit->isAlert()) {
            pageRangeEdit->clear();
            pageRangeEdit->lineEdit()->setPlaceholderText(qApp->translate("DPrintPreviewDialogPrivate", "For example, 1,3,5-7,11-15,18,21"));
        }
    }
    setTurnPageBtnStatus();
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_pageMarginChanged ???????????????????????????????????????????????????
 * \~chinese \param index ?????????????????????????????????????????????
 */
void DPrintPreviewDialogPrivate::_q_pageMarginChanged(int index)
{
    setEnable(index, marginsCombo);
    marginLeftSpin->blockSignals(true);
    marginTopSpin->blockSignals(true);
    marginRightSpin->blockSignals(true);
    marginBottomSpin->blockSignals(true);
    if (index == 1) {
        marginTopSpin->setValue(NORMAL_MODERATE_TOP_BOTTRM);
        marginLeftSpin->setValue(NORMAL_LEFT_RIGHT);
        marginRightSpin->setValue(NORMAL_LEFT_RIGHT);
        marginBottomSpin->setValue(NORMAL_MODERATE_TOP_BOTTRM);
        printer->setPageMargins(QMarginsF(NORMAL_LEFT_RIGHT, NORMAL_MODERATE_TOP_BOTTRM, NORMAL_LEFT_RIGHT, NORMAL_MODERATE_TOP_BOTTRM), QPageLayout::Millimeter);
    } else if (index == 2) {
        marginLeftSpin->setValue(MODERATE_LEFT_RIGHT);
        marginTopSpin->setValue(NORMAL_MODERATE_TOP_BOTTRM);
        marginRightSpin->setValue(MODERATE_LEFT_RIGHT);
        marginBottomSpin->setValue(NORMAL_MODERATE_TOP_BOTTRM);
        printer->setPageMargins(QMarginsF(MODERATE_LEFT_RIGHT, NORMAL_MODERATE_TOP_BOTTRM, MODERATE_LEFT_RIGHT, NORMAL_MODERATE_TOP_BOTTRM), QPageLayout::Millimeter);
    } else if (index == 3) {
        marginTopSpin->setValue(printer->pageLayout().minimumMargins().top());
        marginLeftSpin->setValue(printer->pageLayout().minimumMargins().left());
        marginRightSpin->setValue(printer->pageLayout().minimumMargins().right());
        marginBottomSpin->setValue(printer->pageLayout().minimumMargins().bottom());
        printer->setPageMargins(QMarginsF(marginLeftSpin->value(), marginTopSpin->value(), marginRightSpin->value(), marginBottomSpin->value()), QPageLayout::Millimeter);
    } else {
        marginTopSpin->setValue(printer->pageLayout().minimumMargins().top());
        marginLeftSpin->setValue(printer->pageLayout().minimumMargins().left());
        marginRightSpin->setValue(printer->pageLayout().minimumMargins().right());
        marginBottomSpin->setValue(printer->pageLayout().minimumMargins().bottom());
        printer->setPageMargins(QMarginsF(printer->pageLayout().minimumMargins().left(), printer->pageLayout().minimumMargins().top(), printer->pageLayout().minimumMargins().right(), printer->pageLayout().minimumMargins().bottom()), QPageLayout::Millimeter);
    }
    marginLeftSpin->blockSignals(false);
    marginTopSpin->blockSignals(false);
    marginRightSpin->blockSignals(false);
    marginBottomSpin->blockSignals(false);
    if (isInited) {
        pview->updatePreview();
    }

    if (pview->pageRangeMode() == DPrintPreviewWidget::SelectPage)
        pageRangeCombo->setCurrentIndex(PAGERANGE_ALL);

    if (marginOldValue.length() > 4)
        marginOldValue.clear();

    marginOldValue << marginTopSpin->value() << marginLeftSpin->value() << marginRightSpin->value() << marginBottomSpin->value();
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_ColorModeChange ?????????????????????????????????????????????????????????????????????
 * \~chinese \param index ??????????????????????????????????????????
 */
void DPrintPreviewDialogPrivate::_q_ColorModeChange(int index)
{
    setEnable(index, colorModeCombo);
    //??????????????????????????????
    if (colorModeCombo->count() == 1) {
        pview->setColorMode(DPrinter::GrayScale);
        supportedColorMode = false;
        return;
    }
    if (index == 0) {
        // color
        pview->setColorMode(DPrinter::Color);
        waterColorBtn->setEnabled(true);
        supportedColorMode = true;
    } else {
        // gray
        pview->setColorMode(DPrinter::GrayScale);
        waterColorBtn->setEnabled(false);
        supportedColorMode = false;
        waterColor = QColor("#6f6f6f");
    }
    _q_selectColorButton(waterColor);
    pickColorWidget->convertColor(waterColor);
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_orientationChanged ?????????????????????????????????????????????????????????
 * \~chinese \param index ?????????????????????????????????????????????
 */
void DPrintPreviewDialogPrivate::_q_orientationChanged(int index)
{
    if (index == 0) {
        // ????????????
        if (isInited) {
            pview->setOrientation(DPrinter::Portrait);
        }
    } else {
        // ????????????
        pview->setOrientation(DPrinter::Landscape);
    }
    if (pview->pageRangeMode() == DPrintPreviewWidget::SelectPage)
        pageRangeCombo->setCurrentIndex(PAGERANGE_ALL);
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_customPagesFinished ?????????????????????????????????
 * \~chinese \param index
 */
void DPrintPreviewDialogPrivate::_q_customPagesFinished()
{
    if (pageRangeCombo->currentIndex() != 2)
        return;
    QString cuspages = pageRangeEdit->text();
    lastPageRange = cuspages;
    QVector<int> pagesrange;
    setPageIsLegal(true);
    //?????????????????????????????????????????????????????????
    if (!cuspages.isEmpty()) {
        //????????????????????????????????????1-??????1,????????????????????????????????????????????????????????????
        if (pageRangeEdit->text().right(1) != "-" && pageRangeEdit->text().right(1) != ",") {
            QStringList list = cuspages.split(",");
            //?????????????????????
            for (int i = 0; i < list.size(); i++) {
                //???????????????????????????-?????????????????????????????????????????????
                if (list.at(i).contains("-")) {
                    QStringList list1 = list.at(i).split("-");
                    bool convertFirst = false;
                    bool convertSercond = false;
                    if (list1.at(0).toInt(&convertFirst) <= list1.at(1).toInt(&convertSercond) && convertFirst && convertSercond) {
                        for (int page = list1.at(0).toInt(); page <= list1.at(1).toInt(); page++) {
                            if (page != 0 && page <= totalPages) {
                                pagesrange.append(page);
                            } else {
                                pageRangeError(MaxTip);
                                return;
                            }
                        }
                    } else { //???-????????????????????????????????????????????????
                        if (!convertFirst || !convertSercond) {
                            pageRangeError(MaxTip);
                            return;
                        }
                        QString temp = "";
                        temp = list1.at(0);
                        list1.replace(0, list1.at(1));
                        list1.replace(1, temp);
                        QString str = list1.join("-");
                        list.replace(i, str);
                        pageRangeEdit->setText(list.join(","));
                        if (list1.at(0).toInt() > totalPages || list1.at(1).toInt() > totalPages) {
                            pageRangeError(MaxTip);
                            return;
                        } else {
                            for (int page = list1.at(0).toInt(); page <= list1.at(1).toInt(); page++) {
                                pagesrange.append(page);
                            }
                        }
                    }
                } else {
                    if (list.at(i).toInt() != 0 && list.at(i).toInt() <= totalPages) {
                        pagesrange.append(list.at(i).toInt());
                    } else {
                        pageRangeError(MaxTip);
                        return;
                    }
                }
            }
        } else {
            pageRangeError(FormatTip);
            return;
        }
    } else {
        pageRangeError(NullTip);
        return;
    }
    QVector<int> page = checkDuplication(pagesrange);
    pview->setPageRange(page);
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::adjustMargins ???????????????????????????????????????
 */
void DPrintPreviewDialogPrivate::adjustMargins()
{
    setMininumMargins();
    qreal leftMarginF = this->marginLeftSpin->value();
    qreal topMarginF = this->marginTopSpin->value();
    qreal rightMarginF = this->marginRightSpin->value();
    qreal bottomMarginF = this->marginBottomSpin->value();
    if (qFuzzyCompare(topMarginF, marginOldValue[0]) && qFuzzyCompare(leftMarginF, marginOldValue[1]) && qFuzzyCompare(rightMarginF, marginOldValue[2]) && qFuzzyCompare(bottomMarginF, marginOldValue[3]))
        return;

    marginOldValue.clear();
    marginOldValue << topMarginF << leftMarginF << rightMarginF << bottomMarginF;
    this->printer->setPageMargins(QMarginsF(leftMarginF, topMarginF, rightMarginF, bottomMarginF), QPageLayout::Millimeter);
    this->pview->updatePreview();
    if (pview->pageRangeMode() == DPrintPreviewWidget::SelectPage)
        pageRangeCombo->setCurrentIndex(PAGERANGE_ALL);
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_marginspinChanged ??????????????????spinbox?????????
 * \~chinese \param marginValue ?????????????????????
 */
void DPrintPreviewDialogPrivate::_q_marginspinChanged(double marginValue)
{
    Q_UNUSED(marginValue);
    marginsCombo->blockSignals(true);
    marginsCombo->setCurrentIndex(marginsCombo->count() - 1);
    marginsCombo->blockSignals(false);
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_marginspinChanged ??????????????????spinbox????????????????????????
 */
void DPrintPreviewDialogPrivate::_q_marginEditFinished()
{
    Q_Q(DPrintPreviewDialog);
    if (DDoubleSpinBox *spinbox = qobject_cast<DDoubleSpinBox *>(q->sender())) {
        if (spinboxTextCaches.contains(spinbox->lineEdit()) && spinboxTextCaches.value(spinbox->lineEdit()).isEmpty()) {
            QVariant defalutVariant = spinbox->property("_d_printPreview_spinboxDefalutValue");
            if (defalutVariant.isValid()) {
                spinbox->setValue(defalutVariant.toDouble());
            }
        }
    }

    if (q->focusWidget() == marginTopSpin || q->focusWidget() == marginLeftSpin
            || q->focusWidget() == marginBottomSpin || q->focusWidget() == marginRightSpin)
        return;

    adjustMargins();
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_currentPageSpinChanged ?????????????????????????????????,???????????????????????????
 * \~chinese \param value ????????????
 */
void DPrintPreviewDialogPrivate::_q_currentPageSpinChanged(int value)
{
    if (value == 1 && value != totalPageLabel->text().toInt()) {
        firstBtn->setEnabled(false);
        prevPageBtn->setEnabled(false);
        lastBtn->setEnabled(true);
        nextPageBtn->setEnabled(true);
    } else if (value == totalPageLabel->text().toInt() && value != 1) {
        lastBtn->setEnabled(false);
        nextPageBtn->setEnabled(false);
        firstBtn->setEnabled(true);
        prevPageBtn->setEnabled(true);
    } else if (value == 1 && value == totalPageLabel->text().toInt()) {
        lastBtn->setEnabled(false);
        nextPageBtn->setEnabled(false);
        firstBtn->setEnabled(false);
        prevPageBtn->setEnabled(false);
    } else {
        firstBtn->setEnabled(true);
        prevPageBtn->setEnabled(true);
        lastBtn->setEnabled(true);
        nextPageBtn->setEnabled(true);
    }
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_checkStateChanged ????????????????????????
 * \~chinese \param state ????????????????????????????????????
 */
void DPrintPreviewDialogPrivate::_q_checkStateChanged(int state)
{
    if (!state)
        duplexCombo->setEnabled(false);
    else {
        duplexCombo->setEnabled(true);
    }
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_textWaterMarkModeChanged ???????????????????????????
 * \~chinese \param state ??????????????????????????????
 */
void DPrintPreviewDialogPrivate::_q_textWaterMarkModeChanged(int index)
{
    if (index != waterTextCombo->count() - 1) {
        waterTextEdit->setEnabled(false);
        pview->setTextWaterMark(waterTextCombo->currentText());
        if (!waterTextEdit->text().isEmpty())
            waterTextEdit->clear();
    } else {
        waterTextEdit->setEnabled(true);
        if (!lastCusWatermarkText.isEmpty()) {
            waterTextEdit->setText(lastCusWatermarkText);
            pview->setTextWaterMark(lastCusWatermarkText);
        }
    }
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_customTextWatermarkFinished ??????????????????????????????
 * \~chinese \param state ???????????????????????????
 */
void DPrintPreviewDialogPrivate::_q_customTextWatermarkFinished()
{
    QString cusText = waterTextEdit->text();
    pview->setTextWaterMark(cusText);
    lastCusWatermarkText = cusText;
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::customPictureWatermarkChoosed ????????????
 * \~chinese \param state ??????????????????
 */
void DPrintPreviewDialogPrivate::customPictureWatermarkChoosed(const QString &filename)
{
    QImage image(filename);
    if (!image.isNull()) {
        pview->refreshBegin();
        initWaterSettings();
        pview->refreshEnd();
        pview->setWaterMargImage(image);
    }
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::waterMarkBtnClicked ??????????????????
 * \~chinese \param state ????????????
 */
void DPrintPreviewDialogPrivate::waterMarkBtnClicked(bool isClicked)
{
    if (isClicked) {
        wmSpacer->changeSize(WIDTH_NORMAL, SPACER_HEIGHT_HIDE);
        watermarksettingwdg->show();
        waterTypeGroup->button(0)->setChecked(true);
        watermarkTypeChoosed(typeChoice);
        if (typeChoice == Type_Image - 1 && !picPathEdit->text().isEmpty())
            customPictureWatermarkChoosed(picPathEdit->text());
    } else {
        wmSpacer->changeSize(WIDTH_NORMAL, SPACER_HEIGHT_SHOW);
        watermarksettingwdg->hide();
        pview->setWaterMarkType(Type_None);
    }
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::disablePrintSettings ?????????????????????????????????????????????????????????????????????
 */
void DPrintPreviewDialogPrivate::disablePrintSettings()
{
    Q_Q(DPrintPreviewDialog);

    if (pview->printFromPath().isEmpty())
        return;

    q->findChild<DBackgroundGroup *>("OrientationBackgroundGroup")->setEnabled(false);
    q->findChild<DFrame *>("marginsFrame")->setEnabled(false);
    q->findChild<DBackgroundGroup *>("ScalingContentBackgroundGroup")->setEnabled(false);
    q->findChild<DFrame *>("WaterMarkFrame")->setEnabled(false);
    q->findChild<DFrame *>("btnframe")->setEnabled(false);
    q->findChild<DWidget *>("InorderWidget")->setEnabled(false);
    q->findChild<DWidget *>("CollateWidget")->setEnabled(false);
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::setPageLayoutEnable ??????????????????????????????
 * \~chinese \param checked ??????????????????????????????
 */
void DPrintPreviewDialogPrivate::setPageLayoutEnable(const bool &checked)
{
    QList<DToolButton *> btnList = advancesettingwdg->findChild<DFrame *>("btnframe")->findChildren<DToolButton *>();
    for (DToolButton *button : btnList) {
        button->setEnabled(checked);
    }
    pagePerSheetCombo->setEnabled(checked);
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_colorButtonCliked ??????????????????????????????????????????
 */
void DPrintPreviewDialogPrivate::_q_colorButtonCliked(bool cliked)
{
    Q_Q(DPrintPreviewDialog);
    Q_UNUSED(cliked)
    if (colorWidget->isHidden()) {
        isChecked = false;
    } else {
        isChecked = true;
    }
    if (!isChecked) {
        QPoint colorWidgetPoint;
        QPoint globalBtnPos = textWatermarkWdg->mapToGlobal(QPoint(0, 0));
        QPoint globalDialogPos = q->mapToGlobal(QPoint(0, 0));
        int waterBtnX = globalBtnPos.x() - globalDialogPos.x() + waterColorBtn->pos().x() - 28 - colorWidget->width();
        int waterBtnY = globalBtnPos.y() - globalDialogPos.y() + waterColorBtn->pos().y() + waterColorBtn->height() / 2;
        if (waterBtnY < colorWidget->height() / 2) {
            colorWidgetPoint = QPoint(waterBtnX, waterBtnY);
        } else if (waterBtnY > colorWidget->height() && q->height() - waterBtnY < colorWidget->height() / 2) {
            colorWidgetPoint = QPoint(waterBtnX, waterBtnY - colorWidget->height());
        } else {
            colorWidgetPoint = QPoint(waterBtnX, waterBtnY - colorWidget->height() / 2);
        }
        colorWidget->setGeometry(colorWidgetPoint.x(), colorWidgetPoint.y(), 314, 357);
        colorWidget->show();
    } else {
        colorWidget->hide();
    }
    isChecked = !isChecked;
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_selectColorButton ???????????????????????????????????????
 * \~chinese \param color ???????????????
 */
void DPrintPreviewDialogPrivate::_q_selectColorButton(QColor color)
{
    QPixmap pic(QSize(32, 32));
    pic.fill(Qt::transparent);
    QPainter painter(&pic);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    QBrush brush(color);
    painter.setBrush(brush);
    painter.drawRoundedRect(pic.rect(), PICKCOLOR_RADIUS, PICKCOLOR_RADIUS);
    waterColorBtn->setIcon(QIcon(pic));
    waterColorBtn->setIconSize(QSize(24, 24));
    waterColor = color;
    if (isInitBtnColor)
        pview->setWaterMarkColor(color);
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_printOrderComboIndexChanged ????????????
 * \~chinese \param index ??????????????????
 */
void DPrintPreviewDialogPrivate::_q_printOrderComboIndexChanged(int index)
{
}

void DPrintPreviewDialogPrivate::_q_spinboxValueEmptyChecked(const QString &text)
{
    Q_Q(DPrintPreviewDialog);

    if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(q->sender()))
        spinboxTextCaches.insert(lineEdit, text);
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_pagePersheetComboIndexChanged ??????????????????
 * \~chinese \param index ??????????????????
 */
void DPrintPreviewDialogPrivate::_q_pagePersheetComboIndexChanged(int index)
{
    pview->setImposition(DPrintPreviewWidget::Imposition(index + 1));
}

/*!
 * \~chinese \brief DPrintPreviewDialogPrivate::_q_startPrint ?????????????????????????????????
 * \~chinese \param clicked ????????????????????????
 */
void DPrintPreviewDialogPrivate::_q_startPrint(bool clicked)
{
    Q_Q(DPrintPreviewDialog);
    if (!clicked) {
        setupPrinter();
    }

    bool isSavePicture = (printDeviceCombo->currentIndex() == printDeviceCombo->count() - 1);
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    desktopPath += QStringLiteral("/");
    if (printDeviceCombo->currentIndex() == printDeviceCombo->count() - 2) {
        /*????????????setDocName???????????????????????????????????????????????????print.pdf*/
        if (printer == nullptr) {
            return;
        }
        if (q->docName().isEmpty()) {
            desktopPath += QStringLiteral("print.pdf");
        } else {
            desktopPath += q->docName();
        }
        if (desktopPath.right(4).compare(".pdf", Qt::CaseInsensitive)) {
            desktopPath += ".pdf";
        }
        QFileInfo file(desktopPath);
        QString path = desktopPath;
        if (file.isFile()) {
            int i = 1;
            do {
                QString stres("(%1).pdf");
                path = desktopPath.remove(path.right(4)) + stres.arg(i);
                file.setFile(path);
                i++;
            } while (file.isFile());
            desktopPath = path;
        }

        QString str = DFileDialog::getSaveFileName(q, qApp->translate("DPrintPreviewDialogPrivate", "Save as PDF"), desktopPath, qApp->translate("DPrintPreviewDialogPrivate", "*.pdf"));
        if (str.isEmpty())
            return;

        printer->setOutputFileName(str);
        pview->setPrintMode(DPrintPreviewWidget::PrintToPdf);
    } else if (isSavePicture) {
        if (printer == nullptr) {
            return;
        }

        if (q->docName().isEmpty()) {
            desktopPath += QStringLiteral("print");
        } else {
            desktopPath += q->docName();
        }

        QFileInfo file(desktopPath);
        QString suffix = file.suffix();
        QString stres("(%1)");

        if (!suffix.isEmpty()) {
            // ??????????????? + .
            desktopPath.remove(desktopPath.right(suffix.length() + 1));
        }

        desktopPath.append(QStringLiteral("/"));
        stres.append(QStringLiteral("/"));

        file.setFile(desktopPath);
        QString path = desktopPath;
        int i = 1;
        while (file.isDir()) {
            path = desktopPath.left(desktopPath.length() - 1) + stres.arg(i);
            file.setFile(path);
            i++;
        };

        desktopPath = path;
        QString str = DFileDialog::getSaveFileName(q, qApp->translate("DPrintPreviewDialogPrivate", "Save as image"),
                                                   desktopPath.left(desktopPath.length() - 1),
                                                   qApp->translate("DPrintPreviewDialogPrivate", "Images"));

        if (str.isEmpty())
            return;

        QDir savedDir(str);
        if (!savedDir.exists() && !savedDir.mkpath(str))
            return;

        QString imageSuffix = QFileInfo(q->docName()).suffix();
        imageSuffix = !imageSuffix.compare("jpeg", Qt::CaseInsensitive) ? "jpeg" : "png";
        QString imageName = QFileInfo(str).fileName();
        str.append("/").append(imageName).append(".").append(imageSuffix);
        printer->setOutputFileName(str);
        pview->setPrintMode(DPrintPreviewWidget::PrintToImage);
    } else {
        pview->setPrintMode(DPrintPreviewWidget::PrintToPrinter);
    }

    pview->print();

    q->done(0);
}

void DPrintPreviewDialogPrivate::pageRangeError(TipsNum tipNum)
{
    setPageIsLegal(false);
    if (isOnFocus)
        tipSelected(tipNum);
    isOnFocus = false;
}

DPrintPreviewDialog::DPrintPreviewDialog(QWidget *parent)
    : DDialog(*new DPrintPreviewDialogPrivate(this), parent)
{
    Q_D(DPrintPreviewDialog);
    setMinimumSize(851, 606);
    setWindowFlag(Qt::WindowMaximizeButtonHint);
    if (qApp->isDXcbPlatform()) {
        DPlatformWindowHandle *handle = new DPlatformWindowHandle(this, nullptr);
        handle->setEnableSystemResize(true);
        handle->deleteLater();
    }
    d->startup();
}

DPrintPreviewDialog::~DPrintPreviewDialog()
{
    Q_D(DPrintPreviewDialog);
    if (nullptr != d->printer)
        delete d->printer;
}

bool DPrintPreviewDialog::event(QEvent *event)
{
    Q_D(DPrintPreviewDialog);
    if (event->type() == QEvent::ApplicationFontChange || d->fontSizeMore == true) {
        if (d->waterTypeGroup->button(0)->isChecked()) {
            d->watermarkTypeChoosed(0);
        }
        if (DFontSizeManager::fontPixelSize(qGuiApp->font()) <= 15)
            d->marginsLayout(true);
        else {
            d->marginsLayout(false);
        }
        d->fontSizeMore = false;
    }
    return DDialog::event(event);
}

bool DPrintPreviewDialog::eventFilter(QObject *watched, QEvent *event)
{
    Q_D(DPrintPreviewDialog);

    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *keye = dynamic_cast<QKeyEvent *>(event);
        if (keye->key() == Qt::Key_Enter || keye->key() == Qt::Key_Return) {
            if (watched == d->marginTopSpin || watched == d->marginLeftSpin || watched == d->marginRightSpin || watched == d->marginBottomSpin) {
                d->setMininumMargins();
                d->adjustMargins();
            } else if (watched == d->pageRangeEdit){
                d->_q_customPagesFinished();
                d->isOnFocus = true;
                return true;
            } else if (watched == d->scaleRateEdit){
                Q_EMIT d->scaleRateEdit->lineEdit()->editingFinished();
                return true;
            } else if (watched == d->waterTextEdit) {
                d->_q_customTextWatermarkFinished();
                return true;
            }
         }

         QString str = d->pageRangeEdit->text();
         int strLengthNow = str.length();
         if (watched == d->pageRangeEdit) {
             if ((keye->key() >= Qt::Key_A && keye->key() <= Qt::Key_Z) || (keye->key() >= Qt::Key_Space && keye->key() <= Qt::Key_Slash && keye->key() != Qt::Key_Comma) || keye->key() == Qt::Key_0) {
                 if (str.isEmpty() || (d->strLengths == strLengthNow)) {
                     d->tipSelected(DPrintPreviewDialogPrivate::TipsNum::FormatTip);
                 }
                 d->strLengths = strLengthNow;
                 return true;
             }
             if (keye->key() == Qt::Key_Comma) {
                 if (str.isEmpty() || ((d->strLengths == strLengthNow) && (d->pageRangeEdit->text().right(1) == "," || d->pageRangeEdit->text().right(1) == "-"))) {
                     d->tipSelected(DPrintPreviewDialogPrivate::TipsNum::FormatTip);
                 } else if (!str.isEmpty() && (d->strLengths == strLengthNow)) {
                     d->tipSelected(DPrintPreviewDialogPrivate::TipsNum::CommaTip);
                 }
                 d->strLengths = strLengthNow;
                 return true;
             }
             d->strLengths = strLengthNow;
         }

        return false;
    }
    //??????????????????popup??????
    if (event->type() == QEvent::MouseButtonPress) {
        QRect rect = QRect(d->colorWidget->x(), d->colorWidget->y(), d->colorWidget->width(), d->colorWidget->height());
        QMouseEvent *e = dynamic_cast<QMouseEvent *>(event);
        QPoint pos = mapFromGlobal(QCursor::pos());
        QPoint globalBtnPos = d->textWatermarkWdg->mapToGlobal(QPoint(0, 0));
        QPoint globalDialogPos = mapToGlobal(QPoint(0, 0));
        int waterBtnX = globalBtnPos.x() - globalDialogPos.x() + d->waterColorBtn->pos().x();
        int waterBtnY = globalBtnPos.y() - globalDialogPos.y() + d->waterColorBtn->pos().y();
        QRect btnRect = QRect(waterBtnX, waterBtnY, d->waterColorBtn->width(), d->waterColorBtn->height());
        if (e && !rect.contains(pos) && !btnRect.contains(pos)) {
            d->colorWidget->hide();
        }
    }

    if (event->type() == QEvent::FocusIn) {
        if (watched->inherits("QSpinBox")) {
            if (QSpinBox *spinbox = qobject_cast<QSpinBox *>(watched)) {
                spinbox->setProperty("_d_printPreview_spinboxDefalutValue", spinbox->value());
            }
        } else if (watched->inherits("QDoubleSpinBox")) {
            if (QDoubleSpinBox *spinbox = qobject_cast<QDoubleSpinBox *>(watched)) {
                spinbox->setProperty("_d_printPreview_spinboxDefalutValue", spinbox->value());
            }
        }
    }
    return DDialog::eventFilter(watched, event);
}

/*!
 * \~chinese \brief DPrintPreviewDialog::setDocName ????????????PDF???????????????
 * \~chinese \param str ????????????
 */
void DPrintPreviewDialog::setDocName(const QString &str)
{
    Q_D(DPrintPreviewDialog);
    d->printer->setDocName(str);
}

/*!
 * \~chinese \brief DPrintPreviewDialog::docName ??????PDF???????????????
 * \~chinese \return  ????????????
 */
QString DPrintPreviewDialog::docName() const
{
    D_DC(DPrintPreviewDialog);
    return d->printer->docName();
}

/*!
 * \~chinese \brief DPrintPreviewDialog::setPrintFromPath ?????????????????????????????????
 * \~chinese \param path ????????????
 */
bool DPrintPreviewDialog::setPrintFromPath(const QString &path)
{
    if (path.isEmpty() || !QFileInfo(path).isFile() || !QFileInfo(path).isReadable())
        return false;

    Q_D(DPrintPreviewDialog);

    d->pview->setPrintFromPath(path);
    d->disablePrintSettings();

    return true;
}

/*!
 * \~chinese \brief DPrintPreviewDialog::docName ????????????????????????
 * \~chinese \return  ?????????????????????
 */
QString DPrintPreviewDialog::printFromPath() const
{
    D_DC(DPrintPreviewDialog);

    return d->pview->printFromPath();
}

bool DPrintPreviewDialog::setAsynPreview(int totalPage)
{
    Q_D(DPrintPreviewDialog);

    if (totalPage < 0)
        return false;

    d->pview->setAsynPreview(totalPage);
    return true;
}

bool DPrintPreviewDialog::isAsynPreview() const
{
    D_DC(DPrintPreviewDialog);

    return d->pview->isAsynPreview();
}

void DPrintPreviewDialog::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    Q_D(DPrintPreviewDialog);
    this->findChild<DWidget *>("titlewidget")->setGeometry(0, 0, this->width(), 50);
    this->findChild<DWidget *>("mainwidget")->setGeometry(0, 0, this->width(), this->height());
    double per = static_cast<double>(this->width()) / static_cast<double>(851);
    if (per >= 1.2) {
        this->findChild<DWidget *>("rightWidget")->setMaximumWidth(452 * 1.2);
        this->findChild<DWidget *>("leftWidget")->setMaximumWidth(this->width() - 20 - 10 - 452 * 1.2);
        this->findChild<DBackgroundGroup *>("backGround")->setItemSpacing(10);
    } else {
        this->findChild<DWidget *>("rightWidget")->setMaximumWidth(452 * per);
        this->findChild<DWidget *>("leftWidget")->setMaximumWidth(this->width() - 20 - 2 - 452 * per);
        this->findChild<DBackgroundGroup *>("backGround")->setItemSpacing(2);
    }
}

DWIDGET_END_NAMESPACE
#include "moc_dprintpreviewdialog.cpp"
