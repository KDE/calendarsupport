/*
  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "calprinter.h"
#include "calprintdefaultplugins.h"
#include "journalprint.h"
#include "yearprint.h"

#include <KMessageBox>
#include <KStandardGuiItem>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QSplitter>
#include <QStackedWidget>

#include <PimCommon/KPimPrintPreviewDialog>

using namespace CalendarSupport;

CalPrinter::CalPrinter(QWidget *parent, const Akonadi::ETMCalendar::Ptr &calendar, bool uniqItem)
    : QObject(parent)
    , mUniqItem(uniqItem)
{
    mParent = parent;
    mConfig = new KConfig(QStringLiteral("calendar_printing.rc"), KConfig::SimpleConfig);

    init(calendar);
}

CalPrinter::~CalPrinter()
{
    qDeleteAll(mPrintPlugins);
    delete mConfig;
}

void CalPrinter::init(const Akonadi::ETMCalendar::Ptr &calendar)
{
    mCalendar = calendar;

    qDeleteAll(mPrintPlugins);
    mPrintPlugins.clear();

    if (!mUniqItem) {
        mPrintPlugins.prepend(new CalPrintYear());
        mPrintPlugins.prepend(new CalPrintJournal());
        mPrintPlugins.prepend(new CalPrintTodos());
        mPrintPlugins.prepend(new CalPrintMonth());
        mPrintPlugins.prepend(new CalPrintWeek());
        mPrintPlugins.prepend(new CalPrintDay());
    }
    mPrintPlugins.prepend(new CalPrintIncidence());

    PrintPlugin::List::Iterator it = mPrintPlugins.begin();
    PrintPlugin::List::Iterator end = mPrintPlugins.end();
    for (; it != end; ++it) {
        if (*it) {
            (*it)->setConfig(mConfig);
            (*it)->setCalendar(mCalendar);
            (*it)->doLoadConfig();
        }
    }
}

void CalPrinter::setDateRange(QDate fd, QDate td)
{
    for (const auto plugin : mPrintPlugins) {
        plugin->setDateRange(fd, td);
    }
}

void CalPrinter::print(int type, QDate fd, QDate td, const KCalendarCore::Incidence::List &selectedIncidences, bool preview)
{
    PrintPlugin::List::Iterator it;
    const PrintPlugin::List::Iterator end = mPrintPlugins.end();
    for (it = mPrintPlugins.begin(); it != end; ++it) {
        (*it)->setSelectedIncidences(selectedIncidences);
    }
    QPointer<CalPrintDialog> printDialog = new CalPrintDialog(type, mPrintPlugins, mParent, mUniqItem);

    KConfigGroup grp(mConfig, ""); // orientation setting isn't in a group
    printDialog->setOrientation(CalPrinter::ePrintOrientation(grp.readEntry("Orientation", 1)));
    printDialog->setPreview(preview);
    setDateRange(fd, td);

    if (printDialog->exec() == QDialog::Accepted) {
        grp.writeEntry("Orientation", static_cast<int>(printDialog->orientation()));

        // Save all changes in the dialog
        for (it = mPrintPlugins.begin(); it != mPrintPlugins.end(); ++it) {
            (*it)->doSaveConfig();
        }
        doPrint(printDialog->selectedPlugin(), printDialog->orientation(), preview);
    }
    delete printDialog;

    for (it = mPrintPlugins.begin(); it != mPrintPlugins.end(); ++it) {
        (*it)->setSelectedIncidences(KCalendarCore::Incidence::List());
    }
}

void CalPrinter::doPrint(PrintPlugin *selectedStyle, CalPrinter::ePrintOrientation dlgorientation, bool preview)
{
    if (!selectedStyle) {
        KMessageBox::error(mParent, i18nc("@info", "Unable to print, an invalid print style was specified."), i18nc("@title:window", "Printing error"));
        return;
    }

    QPrinter printer;
    switch (dlgorientation) {
    case eOrientPlugin:
        printer.setPageOrientation(selectedStyle->defaultOrientation());
        break;
    case eOrientPortrait:
        printer.setPageOrientation(QPageLayout::Portrait);
        break;
    case eOrientLandscape:
        printer.setPageOrientation(QPageLayout::Landscape);
        break;
    case eOrientPrinter:
        break;
    }

    if (preview) {
        QPointer<PimCommon::KPimPrintPreviewDialog> printPreview = new PimCommon::KPimPrintPreviewDialog(&printer);
        connect(printPreview.data(), &QPrintPreviewDialog::paintRequested, this, [selectedStyle, &printer]() {
            selectedStyle->doPrint(&printer);
        });
        printPreview->exec();
        delete printPreview;
    } else {
        QPointer<QPrintDialog> printDialog = new QPrintDialog(&printer, mParent);
        if (printDialog->exec() == QDialog::Accepted) {
            selectedStyle->doPrint(&printer);
        }
        delete printDialog;
    }
}

void CalPrinter::updateConfig()
{
}

CalPrintDialog::CalPrintDialog(int initialPrintType, const PrintPlugin::List &plugins, QWidget *parent, bool uniqItem)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Print"));
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto mainLayout = new QVBoxLayout(this);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CalPrintDialog::slotOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CalPrintDialog::reject);
    setModal(true);
    auto page = new QWidget(this);
    auto pageVBoxLayout = new QVBoxLayout(page);
    pageVBoxLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(page);
    mainLayout->addWidget(buttonBox);

    auto splitter = new QSplitter(page);
    pageVBoxLayout->addWidget(splitter);
    splitter->setOrientation(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);
    auto typeBox = new QGroupBox(i18nc("@title:group", "Print Style"), splitter);
    QBoxLayout *typeLayout = new QVBoxLayout(typeBox);
    mTypeGroup = new QButtonGroup(typeBox);

    auto splitterRight = new QWidget(splitter);
    auto splitterRightLayout = new QGridLayout(splitterRight);
    splitterRightLayout->setContentsMargins(0, 0, 0, 0);
    // splitterRightLayout->setMargin( marginHint() );
    // splitterRightLayout->setSpacing( spacingHint() );

    mConfigArea = new QStackedWidget(splitterRight);
    splitterRightLayout->addWidget(mConfigArea, 0, 0, 1, 2);
    auto orientationLabel = new QLabel(i18nc("@label", "Page &orientation:"), splitterRight);
    orientationLabel->setAlignment(Qt::AlignRight);
    splitterRightLayout->addWidget(orientationLabel, 1, 0);

    mOrientationSelection = new QComboBox(splitterRight);
    mOrientationSelection->setToolTip(i18nc("@info:tooltip", "Set the print orientation"));
    mOrientationSelection->setWhatsThis(i18nc("@info:whatsthis",
                                              "Choose if you want your output to be printed in \"portrait\" or "
                                              "\"landscape\". You can also default to the orientation best suited to "
                                              "the selected style or to your printer's default setting."));
    mOrientationSelection->addItem(i18nc("@item:inlistbox", "Use Default Orientation of Selected Style"));
    mOrientationSelection->addItem(i18nc("@item:inlistbox", "Use Printer Default"));
    mOrientationSelection->addItem(i18nc("@item:inlistbox", "Portrait"));
    mOrientationSelection->addItem(i18nc("@item:inlistbox", "Landscape"));
    splitterRightLayout->addWidget(mOrientationSelection, 1, 1);

    // signals and slots connections
    connect(mTypeGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &CalPrintDialog::setPrintType);
    orientationLabel->setBuddy(mOrientationSelection);

    // First insert the config widgets into the widget stack. This possibly assigns
    // proper ids (when two plugins have the same sortID), so store them in a map
    // and use these new IDs to later sort the plugins for the type selection.
    for (PrintPlugin::List::ConstIterator it = plugins.constBegin(), total = plugins.constEnd(); it != total; ++it) {
        int newid = mConfigArea->insertWidget((*it)->sortID(), (*it)->configWidget(mConfigArea));
        mPluginIDs[newid] = (*it);
    }
    // Insert all plugins in sorted order; plugins with clashing IDs will be first
    QMap<int, PrintPlugin *>::ConstIterator mapit;
    bool firstButton = true;
    int id = 0;
    for (mapit = mPluginIDs.constBegin(); mapit != mPluginIDs.constEnd(); ++mapit) {
        PrintPlugin *p = mapit.value();
        auto radioButton = new QRadioButton(p->description());
        radioButton->setEnabled(p->enabled());
        radioButton->setToolTip(i18nc("@info:tooltip", "Select the type of print"));
        radioButton->setWhatsThis(i18nc("@info:whatsthis",
                                        "Select one of the following types of prints you want to make. "
                                        "You may want to print an individual item, or all the items for a "
                                        "specific time range (like a day, week or month), or you may want "
                                        "to print your to-do list."));
        // Check the first available button (to ensure one is selected initially) and then
        // the button matching the desired print type -- if such is available!
        if ((firstButton || p->sortID() == initialPrintType) && p->enabled()) {
            firstButton = false;
            radioButton->setChecked(true);
            changePrintType(id);
        }
        mTypeGroup->addButton(radioButton, mapit.key());
        typeLayout->addWidget(radioButton);
        id++;
    }
    if (uniqItem) {
        typeBox->hide();
    }
    typeLayout->insertStretch(-1, 100);
    setMinimumSize(minimumSizeHint());
    resize(minimumSizeHint());
}

CalPrintDialog::~CalPrintDialog()
{
}

void CalPrintDialog::setPreview(bool preview)
{
    if (preview) {
        mOkButton->setText(i18nc("@action:button", "&Preview"));
    } else {
        mOkButton->setText(KStandardGuiItem::print().text());
    }
}

void CalPrintDialog::changePrintType(int i)
{
    mConfigArea->setCurrentIndex(i);
    mConfigArea->currentWidget()->raise();
    QAbstractButton *btn = mTypeGroup->button(i);
    if (btn) {
        btn->setChecked(true);
    }
}

void CalPrintDialog::setPrintType(QAbstractButton *button)
{
    if (button) {
        const int i = mTypeGroup->id(button);
        mConfigArea->setCurrentIndex(i);
        mConfigArea->currentWidget()->raise();
        button->setChecked(true);
    }
}

void CalPrintDialog::setOrientation(CalPrinter::ePrintOrientation orientation)
{
    mOrientation = orientation;
    mOrientationSelection->setCurrentIndex(mOrientation);
}

CalPrinter::ePrintOrientation CalPrintDialog::orientation() const
{
    return mOrientation;
}

PrintPlugin *CalPrintDialog::selectedPlugin()
{
    int id = mConfigArea->currentIndex();
    if (mPluginIDs.contains(id)) {
        return mPluginIDs[id];
    } else {
        return nullptr;
    }
}

void CalPrintDialog::slotOk()
{
    mOrientation = static_cast<CalPrinter::ePrintOrientation>(mOrientationSelection->currentIndex());

    QMap<int, PrintPlugin *>::ConstIterator it = mPluginIDs.constBegin();
    for (; it != mPluginIDs.constEnd(); ++it) {
        if (it.value()) {
            it.value()->readSettingsWidget();
        }
    }
    accept();
}
