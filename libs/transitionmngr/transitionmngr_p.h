/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2017-05-24
 * Description : images transition manager.
 *
 * Copyright (C) 2017 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

// Local includes

#include "transitionmngr.h"
#include "digikam_config.h"

namespace Digikam
{

class TransitionMngr::Private
{
public:

    typedef int (TransitionMngr::Private::*EffectMethod)(bool);

public:

    explicit Private()
    {
        eff_curTransition    = TransitionMngr::None;
        eff_isRunning        = false;
        eff_x                = 0;
        eff_y                = 0;
        eff_w                = 0;
        eff_h                = 0;
        eff_dx               = 0;
        eff_dy               = 0;
        eff_ix               = 0;
        eff_iy               = 0;
        eff_i                = 0;
        eff_j                = 0;
        eff_subType          = 0;
        eff_x0               = 0;
        eff_y0               = 0;
        eff_x1               = 0;
        eff_y1               = 0;
        eff_wait             = 0;
        eff_fx               = 0;
        eff_fy               = 0;
        eff_alpha            = 0;
        eff_fd               = 0;
        eff_intArray         = 0;
        eff_pdone            = 0;
        eff_pixelMatrix      = 0;
        eff_px               = 0;
        eff_py               = 0;
        eff_psx              = 0;
        eff_psy              = 0;
        eff_pa               = QPolygon(4);

        registerEffects();
    }

    ~Private()
    {
        if (eff_intArray)
            delete [] eff_intArray;
    }

    QMap<TransitionMngr::TransType, EffectMethod> eff_transList;

    QImage                                        eff_inImage;
    QImage                                        eff_outImage;
    QImage                                        eff_curFrame;
    QSize                                         eff_outSize;

    bool                                          eff_isRunning;
    TransitionMngr::TransType                     eff_curTransition;

    // values for state of various effects:
    int                                           eff_x;
    int                                           eff_y;
    int                                           eff_w;
    int                                           eff_h;
    int                                           eff_dx;
    int                                           eff_dy;
    int                                           eff_ix;
    int                                           eff_iy;
    int                                           eff_i;
    int                                           eff_j;
    int                                           eff_subType;
    int                                           eff_x0;
    int                                           eff_y0;
    int                                           eff_x1;
    int                                           eff_y1;
    int                                           eff_wait;
    double                                        eff_fx;
    double                                        eff_fy;
    double                                        eff_alpha;
    double                                        eff_fd;
    int*                                          eff_intArray;
    bool                                          eff_pdone;
    bool**                                        eff_pixelMatrix;

    //static
    QPolygon                                      eff_pa;

    int                                           eff_px;
    int                                           eff_py;
    int                                           eff_psx;
    int                                           eff_psy;

public:

    void registerEffects();

    TransitionMngr::TransType getRandomEffect() const;

    int effectNone(bool aInit);
    int effectChessboard(bool aInit);
    int effectMeltdown(bool aInit);
    int effectSweep(bool aInit);
    int effectMosaic(bool aInit);
    int effectCubism(bool aInit);
    int effectRandom(bool aInit);
    int effectGrowing(bool aInit);
    int effectHorizLines(bool aInit);
    int effectVertLines(bool aInit);
    int effectMultiCircleOut(bool aInit);
    int effectSpiralIn(bool aInit);
    int effectCircleOut(bool aInit);
    int effectBlobs(bool aInit);
    int effectFade(bool aInit);
    int effectSlideL2R(bool aInit);
};

}  // namespace Digikam
