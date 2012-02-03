/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2012-01-20
 * Description : new items finder.
 *
 * Copyright (C) 2012 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#ifndef NEWITEMSFINDER_H
#define NEWITEMSFINDER_H

// Qt includes

#include <QString>

// Local includes

#include "maintenancetool.h"

namespace Digikam
{

class NewItemsFinder : public MaintenanceTool
{
    Q_OBJECT

public:

    enum FinderMode
    {
        CompleteCollectionScan,   // Scan whole collection imediatly.
        ScanDeferredFiles,        // Defer whole collection scan.
        ScheduleCollectionScan    // Scan imediatly folders list passed in contructor.
    };

public:

    NewItemsFinder(FinderMode mode=CompleteCollectionScan, const QStringList& foldersToScan=QStringList(), 
                   ProgressItem* parent=0);
    ~NewItemsFinder();

private Q_SLOTS:

    void slotTotalFilesToScan(int);
    void slotScanStarted(const QString& info);
    void slotProgressValue(float);
    void slotCancel();
};

} // namespace Digikam

#endif /* NEWITEMSFINDER_H */
