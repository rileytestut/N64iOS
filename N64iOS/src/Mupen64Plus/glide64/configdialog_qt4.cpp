/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*   Mupen64plus - configdialog_qt4.cpp                                    *
*   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
*   Copyright (C) 2008 slougi                                             *
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

#include "Gfx1.3.h"
#include "configdialog_qt4.h"

ConfigDialog::ConfigDialog(QWidget* parent)
: QDialog(parent)
{
    setupUi(this);
    connect(buttonBox, SIGNAL(clicked(QAbstractButton*)),
             this, SLOT(buttonClicked(QAbstractButton*)));
    readSettings();
}


void ConfigDialog::accept()
{
    writeSettings();
    QDialog::accept();
}

void ConfigDialog::reset()
{
    readSettings();
}

void ConfigDialog::defaults()
{
    autodetectMicrocodeCheck->setChecked(true);
    forceMicrocodeCombo->setCurrentIndex(2);
    windowResolutionCombo->setCurrentIndex(7);
    fullscreenResolutionCombo->setCurrentIndex(7);
    textureFilterCombo->setCurrentIndex(0);
    filteringModeCombo->setCurrentIndex(1);
    lodCalculationCombo->setCurrentIndex(0);
    fogEnabledCheck->setChecked(false);
    bufferClearOnEveryFrameCheck->setChecked(true);
    verticalSyncCheck->setChecked(false);
    verticalSyncCheck->setChecked(false);
    fastCrcCheck->setChecked(false);
    hiresFramebufferCheck->setChecked(false);
    bufferSwappingMethodCombo->setCurrentIndex(1);
    disableDitheredAlphaCheck->setChecked(false);
    disableGlslCombinersCheck->setChecked(false);
    useFramebufferObjectsCheck->setChecked(false);
    customIniSettingsCheck->setChecked(true);
    wrapTexturesCheck->setChecked(false);
    zeldaCoronaFixCheck->setChecked(false);
    readEveryFrameCheck->setChecked(false);
    detectCpuWritesCheck->setChecked(false);
    getFramebufferInfoCheck->setChecked(false);
    depthBufferRendererCheck->setChecked(false);
    fpsCounterCheck->setChecked(false);
    viCounterCheck->setChecked(false);
    percentSpeedCheck->setChecked(false);
    fpsTransparentCheck->setChecked(false);
    clockEnabledCheck->setChecked(false);
    clockIs24HourCheck->setChecked(false);
}

void ConfigDialog::readSettings()
{
    autodetectMicrocodeCheck->setChecked(settings.autodetect_ucode);
    forceMicrocodeCombo->setCurrentIndex(settings.ucode);
    windowResolutionCombo->setCurrentIndex(settings.res_data);
    fullscreenResolutionCombo->setCurrentIndex(settings.full_res);
    textureFilterCombo->setCurrentIndex(settings.tex_filter);
    filteringModeCombo->setCurrentIndex(settings.filtering);
    lodCalculationCombo->setCurrentIndex(settings.lodmode);
    fogEnabledCheck->setChecked(settings.fog);
    bufferClearOnEveryFrameCheck->setChecked(settings.buff_clear);
    verticalSyncCheck->setChecked(settings.vsync);
    fastCrcCheck->setChecked(settings.fast_crc);
    hiresFramebufferCheck->setChecked(settings.fb_hires);
    bufferSwappingMethodCombo->setCurrentIndex(settings.swapmode);
    disableDitheredAlphaCheck->setChecked(settings.noditheredalpha);
    disableGlslCombinersCheck->setChecked(settings.noglsl);
    useFramebufferObjectsCheck->setChecked(settings.FBO);
    customIniSettingsCheck->setChecked(!settings.custom_ini);
    wrapTexturesCheck->setChecked(settings.wrap_big_tex);
    zeldaCoronaFixCheck->setChecked(settings.flame_corona);
    readEveryFrameCheck->setChecked(settings.fb_read_always);
    detectCpuWritesCheck->setChecked(settings.cpu_write_hack);
    getFramebufferInfoCheck->setChecked(settings.fb_get_info);
    depthBufferRendererCheck->setChecked(settings.fb_depth_render);
    fpsCounterCheck->setChecked(settings.show_fps & 1);
    viCounterCheck->setChecked(settings.show_fps & 2);
    percentSpeedCheck->setChecked(settings.show_fps & 4);
    fpsTransparentCheck->setChecked(settings.show_fps & 8);
    clockEnabledCheck->setChecked(settings.clock);
    clockIs24HourCheck->setChecked(settings.clock_24_hr);
}

void ConfigDialog::writeSettings()
{
    settings.autodetect_ucode = autodetectMicrocodeCheck->isChecked();
    settings.ucode = forceMicrocodeCombo->currentIndex();
    settings.res_data = windowResolutionCombo->currentIndex();
    settings.full_res = fullscreenResolutionCombo->currentIndex();
    settings.tex_filter = textureFilterCombo->currentIndex();
    settings.filtering = filteringModeCombo->currentIndex();
    settings.lodmode = lodCalculationCombo->currentIndex();
    settings.fog = fogEnabledCheck->isChecked();
    settings.buff_clear = bufferClearOnEveryFrameCheck->isChecked();
    settings.vsync = verticalSyncCheck->isChecked();
    settings.fast_crc = fastCrcCheck->isChecked();
    settings.fb_hires = hiresFramebufferCheck->isChecked();
    settings.swapmode = bufferSwappingMethodCombo->currentIndex();
    settings.noditheredalpha = disableDitheredAlphaCheck->isChecked();
    settings.noglsl = disableGlslCombinersCheck->isChecked();
    settings.FBO = useFramebufferObjectsCheck->isChecked();
    settings.custom_ini = !customIniSettingsCheck->isChecked();
    settings.wrap_big_tex = wrapTexturesCheck->isChecked();
    settings.flame_corona = zeldaCoronaFixCheck->isChecked();
    settings.fb_read_always = readEveryFrameCheck->isChecked();
    settings.cpu_write_hack = detectCpuWritesCheck->isChecked();
    settings.fb_get_info = getFramebufferInfoCheck->isChecked();
    settings.fb_depth_render = depthBufferRendererCheck->isChecked();
    settings.show_fps = (
    fpsCounterCheck->isChecked() ? 1 : 0 |
    viCounterCheck->isChecked() ? 2 : 0 |
    percentSpeedCheck->isChecked() ? 4 : 0 |
    fpsTransparentCheck->isChecked() ? 8 : 0
    );
    settings.clock = clockEnabledCheck->isChecked();
    settings.clock_24_hr = clockIs24HourCheck->isChecked();
    
    WriteSettings();
    
    // re-init evoodoo graphics to resize window
    if (evoodoo && fullscreen && !ev_fullscreen) {
        ReleaseGfx();
        InitGfx(TRUE);
    }
}

void ConfigDialog::buttonClicked(QAbstractButton* button)
{
    switch (buttonBox->standardButton(button)) {
        case QDialogButtonBox::Reset:
            readSettings();
            break;
        case QDialogButtonBox::RestoreDefaults:
            defaults();
            break;
    }
}

#include "configdialog_qt4.moc"

