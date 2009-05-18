/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2008-08-11
 * Description : Raw import settings box
 *
 * Copyright (C) 2008-2009 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#ifndef RAWSETTINGSBOX_H
#define RAWSETTINGSBOX_H

// KDE includes

#include <kurl.h>

// Local includes

#include "editortoolsettings.h"
#include "dimg.h"
#include "digikam_export.h"

namespace Digikam
{

class HistogramWidget;
class CurvesWidget;
class RawSettingsBoxPriv;

class DIGIKAM_EXPORT RawSettingsBox : public EditorToolSettings
{
    Q_OBJECT

public:

    RawSettingsBox(const KUrl& url, QWidget *parent);
    ~RawSettingsBox();

    void setBusy(bool b);

    CurvesWidget* curve() const;
    DRawDecoding  settings();

    void writeSettings();
    void readSettings();

    void setDemosaicedImage(DImg& img);
    void setPostProcessedImage(DImg& img);

    void enableUpdateBtn(bool b);

    void resetSettings();

Q_SIGNALS:

    void signalUpdatePreview();
    void signalAbortPreview();
    void signalDemosaicingChanged();
    void signalPostProcessingChanged();

private Q_SLOTS:

    void slotResetCurve();

private:

    RawSettingsBoxPriv* const d;
};

} // namespace Digikam

#endif // RAWSETTINGSBOX_H
