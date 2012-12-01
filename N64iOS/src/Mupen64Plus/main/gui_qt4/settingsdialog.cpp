/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - settingsdialog.cpp                                      *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Slougi                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <QSize>
#include <QListWidgetItem>
#include <QDebug>

#include "rommodel.h"
#include "settingsdialog.h"
#include "globals.h"
#include "ui_settingsdialog.h"

namespace core {
    extern "C" {
        #include "../config.h"
        #include "../main.h"
        #include "../plugin.h"
    }
}

SettingsDialog::SettingsDialog(QWidget* parent)
: QDialog(parent)
{
    setupUi(this);

    QList<QListWidgetItem*> items;
    items << new QListWidgetItem(QIcon(icon("preferences-system.png")),
                                 tr("Main Settings"),
                                 listWidget);
    items << new QListWidgetItem(QIcon(icon("applications-utilities.png")),
                                 tr("Plugins"),
                                 listWidget);
    items << new QListWidgetItem(QIcon(icon("preferences-system-network.png")),
                                 tr("Rom Browser"),
                                 listWidget);
    foreach (QListWidgetItem* item, items) {
        item->setTextAlignment(Qt::AlignHCenter);
        item->setSizeHint(QSize(110, 55));
        listWidget->insertItem(0, item);
    }

    connect(listWidget, SIGNAL(currentRowChanged(int)),
            this, SLOT(pageChanged(int)));
    listWidget->setCurrentRow(0);

    QSize labelPixmapSize(32, 32);
    rspPluginLabel->setPixmap(icon("cpu.png").pixmap(labelPixmapSize));
    inputPluginLabel->setPixmap(icon("input-gaming.png").pixmap(labelPixmapSize));
    audioPluginLabel->setPixmap(icon("audio-card.png").pixmap(labelPixmapSize));
    graphicsPluginLabel->setPixmap(icon("video-display.png").pixmap(labelPixmapSize));

    connect(listWidget, SIGNAL(currentRowChanged(int)),
            this, SLOT(pageChanged(int)));

    connect(buttonBox, SIGNAL(clicked(QAbstractButton*)),
            this, SLOT(buttonClicked(QAbstractButton*)));

    readSettings();
}

void SettingsDialog::on_aboutAudioPluginButton_clicked()
{
    QString text = audioPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_about_with_wid(qPrintable(text), wid);
}

void SettingsDialog::on_configAudioPluginButton_clicked()
{
    QString text = audioPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_config_with_wid(qPrintable(text), wid);
}

void SettingsDialog::on_testAudioPluginButton_clicked()
{
    QString text = audioPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_test_with_wid(qPrintable(text), wid);
}

void SettingsDialog::on_aboutGraphicsPluginButton_clicked()
{
    QString text = graphicsPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_about_with_wid(qPrintable(text), wid);
}

void SettingsDialog::on_configGraphicsPluginButton_clicked()
{
    QString text = graphicsPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_config_with_wid(qPrintable(text), wid);
}

void SettingsDialog::on_testGraphicsPluginButton_clicked()
{
    QString text = graphicsPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_test_with_wid(qPrintable(text), wid);
}

void SettingsDialog::on_aboutRspPluginButton_clicked()
{
    QString text = rspPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_about_with_wid(qPrintable(text), wid);
}

void SettingsDialog::on_configRspPluginButton_clicked()
{
    QString text = rspPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_config_with_wid(qPrintable(text), wid);
}

void SettingsDialog::on_testRspPluginButton_clicked()
{
    QString text = rspPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_test_with_wid(qPrintable(text), wid);
}

void SettingsDialog::on_aboutInputPluginButton_clicked()
{
    QString text = inputPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_about_with_wid(qPrintable(text), wid);
}

void SettingsDialog::on_configInputPluginButton_clicked()
{
    QString text = inputPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_config_with_wid(qPrintable(text), wid);
}

void SettingsDialog::on_testInputPluginButton_clicked()
{
    QString text = inputPluginCombo->currentText();
    core::HWND wid = (core::HWND) winId();
    core::plugin_exec_test_with_wid(qPrintable(text), wid);
}

void SettingsDialog::accept()
{
    writeSettings();
    QDialog::accept();
}

void SettingsDialog::buttonClicked(QAbstractButton* button)
{
    switch (buttonBox->standardButton(button)) {
        case QDialogButtonBox::Reset:
            readSettings();
            break;
        default: // we really only care about a few buttons here
            break;
    }
}

void SettingsDialog::pageChanged(int page)
{
    QListWidgetItem* i = listWidget->item(page);
    imageLabel->setPixmap(i->icon().pixmap(32, 32));
    textLabel->setText(QString("<b>%1</b>").arg(i->text()));
}

void SettingsDialog::readSettings()
{
    int core = core::config_get_number("Core", CORE_DYNAREC);
    switch (core) {
        case CORE_DYNAREC:
            dynamicRecompilerRadio->setChecked(true);
            break;
        case CORE_INTERPRETER:
            interpreterRadio->setChecked(true);
            break;
        case CORE_PURE_INTERPRETER:
            pureInterpreterRadio->setChecked(true);
            break;
    }

    disableCompiledJumpCheck->setChecked(
        core::config_get_bool("NoCompiledJump", FALSE)
    );

    disableMemoryExpansionCheck->setChecked(
        core::config_get_bool("NoMemoryExpansion", FALSE)
    );

    alwaysStartInFullScreenModeCheck->setChecked(
        core::config_get_bool("GuiStartFullscreen", FALSE)
    );

    askBeforeLoadingBadRomCheck->setChecked(
        core::config_get_bool("No Ask", !core::g_NoaskParam)
    );

    autoIncrementSaveSlotCheck->setChecked(
        core::config_get_bool("AutoIncSaveSlot", FALSE)
    );

    osdEnabledCheck->setChecked(core::g_OsdEnabled);

    QString inputPlugin = core::config_get_string("Input Plugin", "");
    QString gfxPlugin = core::config_get_string("Gfx Plugin", "");
    QString audioPlugin = core::config_get_string("Audio Plugin", "");
    QString rspPlugin = core::config_get_string("RSP Plugin", "");

    core::list_node_t *node;
    core::plugin *p;
    list_foreach(core::g_PluginList, node) {
        p = reinterpret_cast<core::plugin*>(node->data);
        switch (p->type) {
            case PLUGIN_TYPE_GFX:
                if(!core::g_GfxPlugin ||
                        (core::g_GfxPlugin &&
                         (strcmp(core::g_GfxPlugin, p->file_name) == 0))) {
                    graphicsPluginCombo->addItem(p->plugin_name);
                }
                break;
            case PLUGIN_TYPE_AUDIO:
                if(!core::g_AudioPlugin ||
                    (core::g_AudioPlugin &&
                    (strcmp(core::g_AudioPlugin, p->file_name) == 0)))
                    audioPluginCombo->addItem(p->plugin_name);
                break;
            case PLUGIN_TYPE_CONTROLLER:
                // if plugin was specified at commandline, only add it to the combobox list
                if(!core::g_InputPlugin ||
                    (core::g_InputPlugin &&
                    (strcmp(core::g_InputPlugin, p->file_name) == 0)))
                    inputPluginCombo->addItem(p->plugin_name);
                break;
            case PLUGIN_TYPE_RSP:
                // if plugin was specified at commandline, only add it to the combobox list
                if(!core::g_RspPlugin ||
                    (core::g_RspPlugin &&
                    (strcmp(core::g_RspPlugin, p->file_name) == 0)))
                    rspPluginCombo->addItem(p->plugin_name);
                break;
        }
    }

    int i = 0;

    inputPlugin = core::plugin_name_by_filename(qPrintable(inputPlugin));
    if (!inputPlugin.isEmpty() &&
        ((i = inputPluginCombo->findText(inputPlugin)) != -1)) {
        inputPluginCombo->setCurrentIndex(i);
    }

    audioPlugin = core::plugin_name_by_filename(qPrintable(audioPlugin));
    if (!audioPlugin.isEmpty() &&
        ((i = audioPluginCombo->findText(audioPlugin)) != -1)) {
        audioPluginCombo->setCurrentIndex(i);
    }

    rspPlugin = core::plugin_name_by_filename(qPrintable(rspPlugin));
    if (!rspPlugin.isEmpty() &&
        ((i = rspPluginCombo->findText(rspPlugin)) != -1)) {
        rspPluginCombo->setCurrentIndex(i);
    }

    gfxPlugin = core::plugin_name_by_filename(qPrintable(gfxPlugin));
    if (!gfxPlugin.isEmpty() &&
        ((i = graphicsPluginCombo->findText(gfxPlugin)) != -1)) {
        graphicsPluginCombo->setCurrentIndex(i);
    }

    romDirectoriesListWidget->setDirectories(romDirectories());

    scanDirectoriesRecursivelyCheck->setChecked(
        core::config_get_bool("RomDirsScanRecursive", FALSE)
    );

    showFullPathsInFilenamesCheck->setChecked(
        core::config_get_bool("RomBrowserShowFullPaths", FALSE)
    );
}

void SettingsDialog::writeSettings()
{
    const char* p = 0;

    p = qPrintable(audioPluginCombo->currentText());
    p = core::plugin_filename_by_name(p);
    core::config_put_string("Audio Plugin", p);

    p = qPrintable(graphicsPluginCombo->currentText());
    p = core::plugin_filename_by_name(p);
    core::config_put_string("Gfx Plugin", p);

    p = qPrintable(rspPluginCombo->currentText());
    p = core::plugin_filename_by_name(p);
    core::config_put_string("RSP Plugin", p);

    p = qPrintable(inputPluginCombo->currentText());
    p = core::plugin_filename_by_name(p);
    core::config_put_string("Input Plugin", p);

    if (dynamicRecompilerRadio->isChecked()) {
        core::config_put_number("Core", CORE_DYNAREC);
    }

    if (interpreterRadio->isChecked()) {
        core::config_put_number("Core", CORE_INTERPRETER);
    }

    if (pureInterpreterRadio->isChecked()) {
        core::config_put_number("Core", CORE_PURE_INTERPRETER);
    }

    core::config_put_bool(
        "NoCompiledJump",
        disableCompiledJumpCheck->isChecked()
    );

    core::config_put_bool(
        "NoMemoryExpansion",
        disableMemoryExpansionCheck->isChecked()
    );

    core::config_put_bool(
        "GuiStartFullscreen",
        alwaysStartInFullScreenModeCheck->isChecked()
    );

    core::g_Noask = askBeforeLoadingBadRomCheck->isChecked();
    core::config_put_bool("No Ask", askBeforeLoadingBadRomCheck->isChecked());

    core::g_OsdEnabled = osdEnabledCheck->isChecked();
    core::config_put_bool("OsdEnabled", osdEnabledCheck->isChecked());

    core::config_put_bool(
        "AutoIncSaveSlot",
        autoIncrementSaveSlotCheck->isChecked()
    );

    core::config_put_bool(
        "RomDirsScanRecursive",
        scanDirectoriesRecursivelyCheck->isChecked()
    );

    core::config_put_bool(
        "RomBrowserShowFullPaths",
        showFullPathsInFilenamesCheck->isChecked()
    );

    QStringList romDirs = romDirectoriesListWidget->directories();
    core::config_put_number("NumRomDirs", romDirs.count());
    int i = 0;
    foreach(QString str, romDirs) {
        if (!str.endsWith("/")) {
            str.append("/");
        }
        core::config_put_string(
            qPrintable(QString("RomDirectory[%1]").arg(i++)),
            qPrintable(str)
        );
    }
    RomModel::self()->settingsChanged();
    core::config_write();
}

