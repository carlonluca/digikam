/* ============================================================
 * Authors: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 *          Caulier Gilles <caulier dot gilles at kdemail dot net>
 *          Marcel Wiesweg <marcel.wiesweg@gmx.de>
 * Date   : 2002-16-10
 * Description : album icon view 
 * 
 * Copyright 2002-2005 by Renchi Raju and Gilles Caulier
 * Copyright 2006-2007 by Gilles Caulier and Marcel Wiesweg
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// C Ansi includes.

extern "C"
{
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
}

// C++ includes.

#include <cstdio>

// Qt includes.

#include <qpixmap.h>
#include <qimage.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qpopupmenu.h>
#include <qdatetime.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qdragobject.h>
#include <qcursor.h>
#include <qvaluevector.h>
#include <qptrlist.h>
#include <qintdict.h>
#include <qdict.h>
#include <qdatastream.h>
#include <qtimer.h>
#include <qclipboard.h>

// KDE includes.

#include <kapplication.h>
#include <kurl.h>
#include <kurldrag.h>
#include <klocale.h>
#include <kglobal.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kpropsdlg.h>
#include <ktrader.h>
#include <kservice.h>
#include <krun.h>
#include <kaction.h>
#include <kstandarddirs.h>
#include <kiconeffect.h>

#include <kdeversion.h>
#if KDE_IS_VERSION(3,2,0)
#include <kcalendarsystem.h>
#include <kinputdialog.h>
#else
#include <klineeditdlg.h>
#endif

// LibKipi includes.

#include <libkipi/pluginloader.h>
#include <libkipi/plugin.h>

// Local includes.

#include "ddebug.h"
#include "album.h"
#include "albummanager.h"
#include "dio.h"
#include "albumlister.h"
#include "albumfiletip.h"
#include "albumsettings.h"
#include "imagewindow.h"
#include "thumbnailsize.h"
#include "themeengine.h"
#include "dpopupmenu.h"
#include "tagspopupmenu.h"
#include "ratingpopupmenu.h"
#include "pixmapmanager.h"
#include "cameradragobject.h"
#include "dragobjects.h"
#include "dmetadata.h"
#include "albumdb.h"
#include "imageattributeswatch.h"
#include "dcrawbinary.h"
#include "deletedialog.h"
#include "albumiconitem.h"
#include "albumicongroupitem.h"
#include "loadingcacheinterface.h"
#include "statusprogressbar.h"
#include "albumiconview.h"
#include "albumiconview.moc"

namespace Digikam
{

class AlbumIconViewPrivate
{
public:

    void init()
    {
        imageLister   = 0;
        currentAlbum  = 0;
        albumSettings = 0;
        pixMan        = 0;
        toolTip       = 0;
    }

    QString                       albumTitle;
    QString                       albumDate;
    QString                       albumComments;

    QRect                         itemRect;
    QRect                         itemRatingRect;
    QRect                         itemDateRect;
    QRect                         itemModDateRect;
    QRect                         itemPixmapRect;
    QRect                         itemNameRect;
    QRect                         itemCommentsRect;
    QRect                         itemResolutionRect;
    QRect                         itemSizeRect;
    QRect                         itemTagRect;
    QRect                         bannerRect;

    QPixmap                       itemRegPixmap;
    QPixmap                       itemSelPixmap;
    QPixmap                       bannerPixmap;
    QPixmap                       ratingPixmap;

    QFont                         fnReg;
    QFont                         fnCom;
    QFont                         fnXtra;

    QDict<AlbumIconItem>          itemDict;
    
    AlbumLister                  *imageLister;
    Album                        *currentAlbum;
    const AlbumSettings          *albumSettings;
    QIntDict<AlbumIconGroupItem>  albumDict;
    PixmapManager                *pixMan;

    ThumbnailSize                 thumbSize;
    
    AlbumFileTip                 *toolTip;
};

AlbumIconView::AlbumIconView(QWidget* parent)
             : IconView(parent)
{
    d = new AlbumIconViewPrivate;
    d->init();
    d->imageLister = AlbumLister::instance();
    d->pixMan      = new PixmapManager(this);
    d->toolTip     = new AlbumFileTip(this);

    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);

    // -- Load rating Pixmap ------------------------------------------

    KGlobal::dirs()->addResourceType("digikam_rating", KGlobal::dirs()->kde_default("data")
                                     + "digikam/data");
    QString ratingPixPath = KGlobal::dirs()->findResourceDir("digikam_rating", "rating.png");
    ratingPixPath += "/rating.png";
    d->ratingPixmap = QPixmap(ratingPixPath);

    QPainter painter(&d->ratingPixmap);
    painter.fillRect(0, 0, d->ratingPixmap.width(), d->ratingPixmap.height(),
                     ThemeEngine::instance()->textSpecialRegColor());
    painter.end();
    
    // -- ImageLister connections -------------------------------------

    connect(d->imageLister, SIGNAL(signalNewFilteredItems(const ImageInfoList&)),
            this, SLOT(slotImageListerNewItems(const ImageInfoList&)));

    connect(d->imageLister, SIGNAL(signalDeleteFilteredItem(ImageInfo*)),
            this, SLOT(slotImageListerDeleteItem(ImageInfo*)) );

    connect(d->imageLister, SIGNAL(signalClear()),
            this, SLOT(slotImageListerClear()));

    // -- Icon connections --------------------------------------------

    connect(this, SIGNAL(signalDoubleClicked(IconItem*)),
            this, SLOT(slotDoubleClicked(IconItem*)));

    connect(this, SIGNAL(signalReturnPressed(IconItem*)),
            this, SLOT(slotDoubleClicked(IconItem*)));

    connect(this, SIGNAL(signalRightButtonClicked(IconItem*, const QPoint &)),
            this, SLOT(slotRightButtonClicked(IconItem*, const QPoint &)));

    connect(this, SIGNAL(signalRightButtonClicked(const QPoint &)),
            this, SLOT(slotRightButtonClicked(const QPoint &)));

    connect(this, SIGNAL(signalSelectionChanged()),
            this, SLOT(slotSelectionChanged()));

    connect(this, SIGNAL(signalShowToolTip(IconItem*)),
            this, SLOT(slotShowToolTip(IconItem*)));

    // -- ThemeEngine connections ---------------------------------------

    connect(ThemeEngine::instance(), SIGNAL(signalThemeChanged()),
            SLOT(slotThemeChanged()));

    // -- Pixmap manager connections ------------------------------------

    connect(d->pixMan, SIGNAL(signalPixmap(const KURL&)),
            SLOT(slotGotThumbnail(const KURL&)));

    // -- ImageAttributesWatch connections ------------------------------

    ImageAttributesWatch *watch = ImageAttributesWatch::instance();

    connect(watch, SIGNAL(signalImageTagsChanged(Q_LLONG)),
            this, SLOT(slotImageAttributesChanged(Q_LLONG)));

    connect(watch, SIGNAL(signalImagesChanged(int)),
            this, SLOT(slotAlbumImagesChanged(int)));

    connect(watch, SIGNAL(signalImageRatingChanged(Q_LLONG)),
            this, SLOT(slotImageAttributesChanged(Q_LLONG)));

    connect(watch, SIGNAL(signalImageDateChanged(Q_LLONG)),
            this, SLOT(slotImageAttributesChanged(Q_LLONG)));

    connect(watch, SIGNAL(signalImageCaptionChanged(Q_LLONG)),
            this, SLOT(slotImageAttributesChanged(Q_LLONG)));
}

AlbumIconView::~AlbumIconView()
{
    delete d->pixMan;
    delete d->toolTip;
    delete d;
}

void AlbumIconView::applySettings(const AlbumSettings* settings)
{
    if (!settings)
        return;

    d->albumSettings = settings;

    d->imageLister->setNameFilter(d->albumSettings->getAllFileFilter());

    d->thumbSize = (ThumbnailSize::Size)d->albumSettings->getDefaultIconSize();

    setEnableToolTips(d->albumSettings->getShowToolTips());

    updateBannerRectPixmap();
    updateItemRectsPixmap();

    d->imageLister->stop();
    clear();

    d->pixMan->setThumbnailSize(d->thumbSize.size());

    if (d->currentAlbum)
    {
        d->imageLister->openAlbum(d->currentAlbum);
    }
}

void AlbumIconView::setThumbnailSize(const ThumbnailSize& thumbSize)
{
    if ( d->thumbSize != thumbSize)
    {
        d->imageLister->stop();
        clear();

        d->thumbSize = thumbSize;
        d->pixMan->setThumbnailSize(d->thumbSize.size());

        updateBannerRectPixmap();
        updateItemRectsPixmap();

        d->imageLister->openAlbum(d->currentAlbum);
    }
}

void AlbumIconView::setAlbum(Album* album)
{
    if (!album)
    {
        d->currentAlbum = 0;
        d->imageLister->stop();
        clear();

        return;
    }

    if (d->currentAlbum == album)
        return;

    d->imageLister->stop();
    clear();

    d->currentAlbum = album;
    d->imageLister->openAlbum(d->currentAlbum);

    updateBannerRectPixmap();
    updateItemRectsPixmap();
}

void AlbumIconView::refreshIcon(AlbumIconItem* item)
{
    if (!item)
        return;

    emit signalSelectionChanged();
}

void AlbumIconView::clear(bool update)
{
    emit signalCleared();

    d->pixMan->clear();
    d->itemDict.clear();
    d->albumDict.clear();

    IconView::clear(update);

    emit signalSelectionChanged();
}

void AlbumIconView::slotImageListerNewItems(const ImageInfoList& itemList)
{
    if (!d->currentAlbum || d->currentAlbum->isRoot())
        return;

    ImageInfo* item;
    for (ImageInfoListIterator it(itemList); (item = it.current()); ++it)
    {
        KURL url( item->kurl() );
        url.cleanPath();

        if (AlbumIconItem *oldItem = d->itemDict.find(url.url()))
        {
            slotImageListerDeleteItem(oldItem->imageInfo());
        }

        AlbumIconGroupItem* group = d->albumDict.find(item->albumID());
        if (!group)
        {
            group = new AlbumIconGroupItem(this, item->albumID());
            d->albumDict.insert(item->albumID(), group);
        }

        if (!item->album())
        {
            DWarning() << "No album for item: " << item->name()
                       << ", albumID: " << item->albumID() << endl;
            continue;
        }

        AlbumIconItem* iconItem = new AlbumIconItem(group, item);
        item->setViewItem(iconItem);

        d->itemDict.insert(url.url(), iconItem);
    }

    emit signalItemsAdded();
}

void AlbumIconView::slotImageListerDeleteItem(ImageInfo* item)
{
    if (!item->getViewItem())
        return;

    AlbumIconItem* iconItem = static_cast<AlbumIconItem*>(item->getViewItem());

    KURL url(item->kurl());
    url.cleanPath();
    
    AlbumIconItem *oldItem = d->itemDict[url.url()];
    
    if( oldItem &&
       (oldItem->imageInfo()->id() != iconItem->imageInfo()->id()))
    {
        return;
    }

    //d->pixMan->remove(item->kurl());

    emit signalItemDeleted(iconItem);

    delete iconItem;
    item->setViewItem(0);

    d->itemDict.remove(url.url());

    IconGroupItem* group = firstGroup();
    IconGroupItem* tmp;
    
    while (group)
    {
        tmp = group->nextGroup();
        
        if (group->count() == 0)
        {
            d->albumDict.remove(((AlbumIconGroupItem*)group)->albumID());
            delete group;
        }
        
        group = tmp;
    }
}

void AlbumIconView::slotImageListerClear()
{
    clear();
}

void AlbumIconView::slotDoubleClicked(IconItem *item)
{
    if (!item) return;

    if (d->albumSettings->getItemRightClickAction() == AlbumSettings::ShowPreview)
        signalPreviewItem(static_cast<AlbumIconItem *>(item));
    else 
        slotDisplayItem(static_cast<AlbumIconItem *>(item));
}

void AlbumIconView::slotRightButtonClicked(const QPoint& pos)
{
    if (!d->currentAlbum)
        return;

    if (d->currentAlbum->isRoot() ||
         (   d->currentAlbum->type() != Album::PHYSICAL
          && d->currentAlbum->type() != Album::TAG))
    {
        return;
    }
            
    QPopupMenu popmenu(this);
    KAction *paste    = KStdAction::paste(this, SLOT(slotPaste()), 0);
    QMimeSource *data = kapp->clipboard()->data(QClipboard::Clipboard);
    
    if(!data || !QUriDrag::canDecode(data))
    {
        paste->setEnabled(false);
    }
    
    paste->plug(&popmenu);
    popmenu.exec(pos);
    delete paste;    
}

void AlbumIconView::slotRightButtonClicked(IconItem *item, const QPoint& pos)
{
    if (!item)
        return;
    
    AlbumIconItem* iconItem = static_cast<AlbumIconItem *>(item);

    // --------------------------------------------------------

    KMimeType::Ptr mimePtr = KMimeType::findByURL(iconItem->imageInfo()->kurl(), 0, true, true);

    QValueVector<KService::Ptr> serviceVector;
    KTrader::OfferList offers = KTrader::self()->query(mimePtr->name(), "Type == 'Application'");

    QPopupMenu openWithMenu;

    KTrader::OfferList::Iterator iter;
    KService::Ptr ptr;
    int index = 100;

    for( iter = offers.begin(); iter != offers.end(); ++iter )
    {
        ptr = *iter;
        openWithMenu.insertItem( ptr->pixmap(KIcon::Small), ptr->name(), index++);
        serviceVector.push_back(ptr);
    }

    // --------------------------------------------------------

    DPopupMenu popmenu(this);
    popmenu.insertItem(SmallIcon("viewimage"), i18n("View..."), 18);
    popmenu.insertItem(SmallIcon("editimage"), i18n("Edit..."), 10);
    popmenu.insertItem(i18n("Open With"), &openWithMenu, 11);

    // Merge in the KIPI plugins actions ----------------------------

    KIPI::PluginLoader* kipiPluginLoader      = KIPI::PluginLoader::instance();
    KIPI::PluginLoader::PluginList pluginList = kipiPluginLoader->pluginList();
    
    for (KIPI::PluginLoader::PluginList::const_iterator it = pluginList.begin();
         it != pluginList.end(); ++it)
    {
        KIPI::Plugin* plugin = (*it)->plugin();

        if (plugin && (*it)->name() == "JPEGLossless")
        {
            DDebug() << "Found JPEGLossless plugin" << endl;

            KActionPtrList actionList = plugin->actions();
            
            for (KActionPtrList::const_iterator iter = actionList.begin();
                 iter != actionList.end(); ++iter)
            {
                KAction* action = *iter;
                
                if (QString::fromLatin1(action->name())
                    == QString::fromLatin1("jpeglossless_rotate"))
                {
                    action->plug(&popmenu);
                }
            }
        }
    }

    // --------------------------------------------------------

    popmenu.insertItem(SmallIcon("pencil"), i18n("Rename..."), 15);
    popmenu.insertSeparator();

    // --------------------------------------------------------

    if (d->currentAlbum)
    {
        if (d->currentAlbum->type() == Album::PHYSICAL )
            popmenu.insertItem(i18n("Set as Album Thumbnail"), 17);
        else if (d->currentAlbum->type() == Album::TAG )
            popmenu.insertItem(i18n("Set as Tag Thumbnail"), 17);
    }

    popmenu.insertSeparator();

    // --------------------------------------------------------
    
    KAction *copy     = KStdAction::copy(this, SLOT(slotCopy()), 0);
    KAction *paste    = KStdAction::paste(this, SLOT(slotPaste()), 0);
    QMimeSource *data = kapp->clipboard()->data(QClipboard::Clipboard);
    if(!data || !QUriDrag::canDecode(data))
    {
        paste->setEnabled(false);
    }    
    copy->plug(&popmenu);
    paste->plug(&popmenu);
    
    popmenu.insertSeparator();

    // --------------------------------------------------------

    QValueList<Q_LLONG> selectedImageIDs;
    
    for (IconItem *it = firstItem(); it; it=it->nextItem())
    {
        if (it->isSelected())
        {
            AlbumIconItem *selItem = static_cast<AlbumIconItem *>(it);
            selectedImageIDs.append(selItem->imageInfo()->id());
        }
    }

    popmenu.insertItem(SmallIcon("edittrash"),
                       i18n("Move to Trash", "Move %n Files to Trash" , selectedImageIDs.count() ), 16);

    popmenu.insertSeparator();

    // Bulk assignment/removal of tags --------------------------

    TagsPopupMenu* assignTagsPopup = new TagsPopupMenu(selectedImageIDs, 1000, TagsPopupMenu::ASSIGN);
    TagsPopupMenu* removeTagsPopup = new TagsPopupMenu(selectedImageIDs, 1000, TagsPopupMenu::REMOVE);
    
    connect(assignTagsPopup, SIGNAL(signalTagActivated(int)),
            this, SLOT(slotAssignTag(int)));
            
    connect(removeTagsPopup, SIGNAL(signalTagActivated(int)),
            this, SLOT(slotRemoveTag(int)));

    popmenu.insertItem(i18n("Assign Tag"), assignTagsPopup);

    int removeTagId = popmenu.insertItem(i18n("Remove Tag"), removeTagsPopup);

    AlbumManager* man = AlbumManager::instance();

    // Performance: Only check for tags if there are <250 images selected
    if (selectedImageIDs.count() > 250 ||
        !man->albumDB()->hasTags(selectedImageIDs))
            popmenu.setItemEnabled(removeTagId, false);

    popmenu.insertSeparator();

    // Assign Star Rating -------------------------------------------

    RatingPopupMenu ratingMenu;
    
    connect(&ratingMenu, SIGNAL(activated(int)),
            this, SLOT(slotAssignRating(int)));

    popmenu.insertItem(i18n("Assign Rating"), &ratingMenu);

    // --------------------------------------------------------
        
    int id = popmenu.exec(pos);

    switch(id) 
    {
      case 10: 
      {
          slotDisplayItem(iconItem);
          break;
      }
  
      case 15: 
      {
          slotRename(iconItem);
          break;
      }
  
      case 16: 
      {
          slotDeleteSelectedItems();
          break;
      }
  
      case 17: 
      {
          slotSetAlbumThumbnail(iconItem);
          break;
      }

      case 18: 
      {
          signalPreviewItem(iconItem);
          break;
      }
  
      default:
          break;
    }

    //---------------------------------------------------------------

    if (id >= 100 && id < 1000) 
    {
        KService::Ptr imageServicePtr = serviceVector[id-100];
        KURL::List urlList;
        for (IconItem *it = firstItem(); it; it=it->nextItem())
        {
            if (it->isSelected())
            {
                AlbumIconItem *selItem = static_cast<AlbumIconItem *>(it);
                urlList.append(selItem->imageInfo()->kurl());
            }
        }
        if (urlList.count())
            KRun::run(*imageServicePtr, urlList);
    }

    serviceVector.clear();
    delete assignTagsPopup;
    delete removeTagsPopup;
    delete copy;
    delete paste;
}

void AlbumIconView::slotCopy()
{
    if (!d->currentAlbum)
        return;

    KURL::List      urls;
    KURL::List      kioURLs;
    QValueList<int> albumIDs;
    QValueList<int> imageIDs;

    for (IconItem *it = firstItem(); it; it=it->nextItem())
    {
        if (it->isSelected())
        {
            AlbumIconItem *albumItem = static_cast<AlbumIconItem *>(it);
            urls.append(albumItem->imageInfo()->kurl());
            kioURLs.append(albumItem->imageInfo()->kurlForKIO());
            imageIDs.append(albumItem->imageInfo()->id());
        }
    }
    albumIDs.append(d->currentAlbum->id());

    if (urls.isEmpty())
        return;

    QDragObject* drag = 0;

    drag = new ItemDrag(urls, kioURLs, albumIDs, imageIDs, this);
    kapp->clipboard()->setData(drag);
}

void AlbumIconView::slotPaste()
{
    QMimeSource *data = kapp->clipboard()->data(QClipboard::Clipboard);
    if(!data || !QUriDrag::canDecode(data))
        return;

    if(d->currentAlbum->type() == Album::PHYSICAL)
    {
        if (QUriDrag::canDecode(data) &&
            d->currentAlbum->type() == Album::PHYSICAL)
        {
            PAlbum* palbum = (PAlbum*)d->currentAlbum;
            
            // B.K.O #119205: do not handle root album.
            if (palbum->isRoot())
                return;

            KURL destURL(palbum->kurl());

            KURL::List srcURLs;
            KURLDrag::decode(data, srcURLs);

            KIO::Job* job = DIO::copy(srcURLs, destURL);
            connect(job, SIGNAL(result(KIO::Job*)),
                    this, SLOT(slotDIOResult(KIO::Job*)));
        }
    }
}

void AlbumIconView::slotSetAlbumThumbnail(AlbumIconItem *iconItem)
{
    if(!d->currentAlbum)
        return;

    if(d->currentAlbum->type() == Album::PHYSICAL)
    {
        PAlbum *album = static_cast<PAlbum*>(d->currentAlbum);

        QString err;
        AlbumManager::instance()->updatePAlbumIcon( album,
                                                    iconItem->imageInfo()->id(),
                                                    err );
    }
    else if (d->currentAlbum->type() == Album::TAG)
    {
        TAlbum *album = static_cast<TAlbum*>(d->currentAlbum);

        QString err;
        AlbumManager::instance()->updateTAlbumIcon( album,
                                                    QString(),
                                                    iconItem->imageInfo()->id(),
                                                    err );
    }
}

void AlbumIconView::slotRename(AlbumIconItem* item)
{
    if (!item)
        return;

    QFileInfo fi(item->imageInfo()->name());
    QString ext  = QString(".") + fi.extension(false);
    QString name = fi.fileName();
    name.truncate(fi.fileName().length() - ext.length());

    bool ok;

#if KDE_IS_VERSION(3,2,0)
    QString newName = KInputDialog::getText(i18n("Rename Item (%1)").arg(fi.fileName()), 
                                            i18n("Enter new name (without extension):"),
                                            name, &ok, this);
#else
    QString newName = KLineEditDlg::getText(i18n("Rename Item (%1)").arg(fi.fileName()), 
                                            i18n("Enter new name (without extension):"),
                                            name, &ok, this);
#endif

    if (!ok)
        return;

    QString oldURL = item->imageInfo()->kurl().url();

    if (!item->imageInfo()->setName(newName + ext))
        return;

    d->itemDict.remove(oldURL);
    d->itemDict.insert(item->imageInfo()->kurl().url(), item);

    item->repaint();
    signalItemsAdded();
}

void AlbumIconView::slotDeleteSelectedItems(bool deletePermanently)
{
    KURL::List  urlList;
    QStringList nameList;
    KURL url;

    for (IconItem *it = firstItem(); it; it=it->nextItem())
    {
        if (it->isSelected()) 
        {
            AlbumIconItem *iconItem = static_cast<AlbumIconItem *>(it);
            url = iconItem->imageInfo()->kurl();
            urlList.append(url);
            nameList.append(iconItem->imageInfo()->name());
        }
    }

    if (urlList.count() <= 0)
        return;

    DeleteDialog dialog(this);

    if (!dialog.confirmDeleteList(urlList,
                                  DeleteDialogMode::Files,
                                  deletePermanently ?
                                  DeleteDialogMode::NoChoiceDeletePermanently :
                                  DeleteDialogMode::NoChoiceTrash))
        return;

    bool useTrash = !dialog.shouldDelete();

    KIO::Job* job = DIO::del(urlList, useTrash);
    connect(job, SIGNAL(result(KIO::Job*)),
            this, SLOT(slotDIOResult(KIO::Job*)));

    // The AlbumManager KDirWatch will trigger a DIO::scan.
    // When this is completed, DIO will call AlbumLister::instance()->refresh().
}

void AlbumIconView::slotDeleteSelectedItemsDirectly(bool useTrash)
{
    // This method deletes the selected items directly, without confirmation.
    // It is not used in the default setup.

    KURL::List  urlList;

    for (IconItem *it = firstItem(); it; it=it->nextItem())
    {
        if (it->isSelected())
        {
            AlbumIconItem *iconItem = static_cast<AlbumIconItem *>(it);
            urlList.append(iconItem->imageInfo()->kurl());
        }
    }

    if (urlList.count() <= 0)
        return;

    KIO::Job* job = DIO::del(urlList, useTrash);

    connect(job, SIGNAL(result(KIO::Job*)),
            this, SLOT(slotDIOResult(KIO::Job*)));
}

void AlbumIconView::slotFilesModified()
{
    d->imageLister->refresh();
}

void AlbumIconView::slotFilesModified(const KURL& url)
{
    refreshItems(url);
}

void AlbumIconView::slotImageWindowURLChanged(const KURL &url)
{
    IconItem* item = findItem(url.url());
    if (item)
        setCurrentItem(item);
}

void AlbumIconView::slotDisplayItem(AlbumIconItem *item )
{
    if (!item) return;

    AlbumSettings *settings = AlbumSettings::instance();

    if (!settings) return;

    KIconEffect::visualActivate(viewport(), contentsRectToViewport(item->rect()));

    QString currentFileExtension = item->imageInfo()->name().section( '.', -1 );
    QString imagefilter = settings->getImageFileFilter().lower() +
                          settings->getImageFileFilter().upper();

    if (DcrawBinary::instance()->versionIsRight())
    {
        // add raw files only if dcraw is available
        imagefilter += settings->getRawFileFilter().lower() +
                       settings->getRawFileFilter().upper();
    }

    // If the current item isn't an image file.
    if ( !imagefilter.contains(currentFileExtension) )
    {
        KMimeType::Ptr mimePtr = KMimeType::findByURL(item->imageInfo()->kurl(),
                                                      0, true, true);
        KTrader::OfferList offers = KTrader::self()->query(mimePtr->name(),
                                                           "Type == 'Application'");

        if (offers.isEmpty())
            return;

        KService::Ptr ptr = offers.first();
        // Run the dedicated app to show the item.
        KRun::run(*ptr, item->imageInfo()->kurl());
        return;
    }

    // Run Digikam ImageEditor with all image files in the current Album.

    ImageInfoList imageInfoList;
    ImageInfo *currentImageInfo = 0;

    for (IconItem *it = firstItem() ; it ; it = it->nextItem())
    {
        AlbumIconItem *iconItem = static_cast<AlbumIconItem *>(it);
        QString fileExtension = iconItem->imageInfo()->kurl().fileName().section( '.', -1 );

        if ( imagefilter.find(fileExtension) != -1 )
        {
            ImageInfo *info = new ImageInfo(*iconItem->imageInfo());
            info->setViewItem(0);
            imageInfoList.append(info);
            if (iconItem == item)
                currentImageInfo = info;
        }
    }

    ImageWindow *imview = ImageWindow::imagewindow();

    imview->disconnect(this);

    connect(imview, SIGNAL(signalFileAdded(const KURL&)),
            this, SLOT(slotFilesModified()));

    connect(imview, SIGNAL(signalFileModified(const KURL&)),
            this, SLOT(slotFilesModified(const KURL&)));

    connect(imview, SIGNAL(signalFileDeleted(const KURL&)),
            this, SLOT(slotFilesModified()));

    connect(imview, SIGNAL(signalURLChanged(const KURL&)),
            this, SLOT(slotImageWindowURLChanged(const KURL &)));

    imview->loadImageInfos(imageInfoList,
                           currentImageInfo,
                           d->currentAlbum ? i18n("Album \"%1\"").arg(d->currentAlbum->title()) : QString(),
                           true);

    if (imview->isHidden())
        imview->show();

    imview->raise();
    imview->setFocus();
}

// ------------------------------------------------------------------------------

AlbumIconItem* AlbumIconView::firstSelectedItem() const
{
    AlbumIconItem *iconItem = 0;
    for (IconItem *it = firstItem(); it; it = it->nextItem())
    {
        if (it->isSelected())
        {
            iconItem = static_cast<AlbumIconItem *>(it);
            break;
        }
    }

    return iconItem;
}

const AlbumSettings* AlbumIconView::settings() const
{
    return d->albumSettings;
}

ThumbnailSize AlbumIconView::thumbnailSize() const
{
    return d->thumbSize;
}

void AlbumIconView::resizeEvent(QResizeEvent *e)
{
    IconView::resizeEvent(e);

    if (d->bannerRect.width() != frameRect().width())
        updateBannerRectPixmap();
}

// -- DnD ---------------------------------------------------

void AlbumIconView::startDrag()
{
    if (!d->currentAlbum)
        return;

    KURL::List      urls;
    KURL::List      kioURLs;
    QValueList<int> albumIDs;
    QValueList<int> imageIDs;

    for (IconItem *it = firstItem(); it; it=it->nextItem())
    {
        if (it->isSelected())
        {
            AlbumIconItem *albumItem = static_cast<AlbumIconItem *>(it);
            urls.append(albumItem->imageInfo()->kurl());
            kioURLs.append(albumItem->imageInfo()->kurlForKIO());
            imageIDs.append(albumItem->imageInfo()->id());
        }
    }
    albumIDs.append(d->currentAlbum->id());

    if (urls.isEmpty())
        return;

    QPixmap icon(DesktopIcon("image", 48));
    int w = icon.width();
    int h = icon.height();

    QPixmap pix(w+4,h+4);
    QString text(QString::number(urls.count()));

    QPainter p(&pix);
    p.fillRect(0, 0, w+4, h+4, QColor(Qt::white));
    p.setPen(QPen(Qt::black, 1));
    p.drawRect(0, 0, w+4, h+4);
    p.drawPixmap(2, 2, icon);
    QRect r = p.boundingRect(2,2,w,h,Qt::AlignLeft|Qt::AlignTop,text);
    r.setWidth(QMAX(r.width(),r.height()));
    r.setHeight(QMAX(r.width(),r.height()));
    p.fillRect(r, QColor(0,80,0));
    p.setPen(Qt::white);
    QFont f(font());
    f.setBold(true);
    p.setFont(f);
    p.drawText(r, Qt::AlignCenter, text);
    p.end();

    QDragObject* drag = 0;

    drag = new ItemDrag(urls, kioURLs, albumIDs, imageIDs, this);
    if (drag)
    {
        drag->setPixmap(pix);
        drag->drag();
    }
}

void AlbumIconView::contentsDragMoveEvent(QDragMoveEvent *event)
{
    if (!d->currentAlbum || (AlbumDrag::canDecode(event) ||
                             !QUriDrag::canDecode(event) &&
                             !CameraDragObject::canDecode(event) &&
                             !TagListDrag::canDecode(event) &&
                             !TagDrag::canDecode(event))
        || event->source() == this) 
    {
        event->ignore();
        return;
    }
    event->accept();
}

void AlbumIconView::contentsDropEvent(QDropEvent *event)
{
    // TODO: need to rework this with specific to in which
    // groupitem items are dropped

    if (!d->currentAlbum || (AlbumDrag::canDecode(event) ||
                             !QUriDrag::canDecode(event) &&
                             !CameraDragObject::canDecode(event) &&
                             !TagListDrag::canDecode(event) &&
                             !TagDrag::canDecode(event))
         || event->source() == this)
    {
        event->ignore();
        return;
    }

    if (QUriDrag::canDecode(event) &&
        d->currentAlbum->type() == Album::PHYSICAL)
    {
        PAlbum* palbum = (PAlbum*)d->currentAlbum;
        KURL destURL(palbum->kurl());

        KURL::List srcURLs;
        KURLDrag::decode(event, srcURLs);

        QPopupMenu popMenu(this);
        popMenu.insertItem( SmallIcon("goto"), i18n("&Move Here"), 10 );
        popMenu.insertItem( SmallIcon("editcopy"), i18n("&Copy Here"), 11 );
        popMenu.insertSeparator(-1);
        popMenu.insertItem( SmallIcon("cancel"), i18n("C&ancel") );

        popMenu.setMouseTracking(true);
        int id = popMenu.exec(QCursor::pos());
        switch(id) 
        {
            case 10: 
            {
                KIO::Job* job = DIO::move(srcURLs, destURL);
                connect(job, SIGNAL(result(KIO::Job*)),
                        this, SLOT(slotDIOResult(KIO::Job*)));
                break;
            }
            case 11: 
            {
                KIO::Job* job = DIO::copy(srcURLs, destURL);
                connect(job, SIGNAL(result(KIO::Job*)),
                        this, SLOT(slotDIOResult(KIO::Job*)));
                break;
            }
            default:
                break;
        }
    }
    else if(TagDrag::canDecode(event))
    {
        QByteArray ba = event->encodedData("digikam/tag-id");
        QDataStream ds(ba, IO_ReadOnly);
        int i = 0;
        int tagID;
        ds >> tagID;

        AlbumManager* man = AlbumManager::instance();
        TAlbum* talbum    = man->findTAlbum(tagID);

        if (talbum)
        {
            QPopupMenu popMenu(this);

            bool itemsSelected = false;
            for (IconItem *it = firstItem(); it; it = it->nextItem())
                if (it->isSelected())
                    itemsSelected = true;
    
            bool itemDropped = false;
            AlbumIconItem *albumItem = findItem(event->pos());
            if (albumItem) 
                itemDropped = true;

            popMenu.insertItem(SmallIcon("tag"), 
                               i18n("Assign '%1' to &All Items").arg(talbum->tagPath().mid(1)),          11);

            if (itemsSelected)
                popMenu.insertItem(SmallIcon("tag"), 
                                   i18n("Assign '%1' to &Selected Items").arg(talbum->tagPath().mid(1)), 10);
            
            if (itemDropped)
                popMenu.insertItem(SmallIcon("tag"),
                                   i18n("Assign '%1' to &Dropped Item").arg(talbum->tagPath().mid(1)),   12);

            popMenu.insertSeparator(-1);
            popMenu.insertItem(SmallIcon("cancel"), i18n("&Cancel"));

            popMenu.setMouseTracking(true);
            int id = popMenu.exec(QCursor::pos());
            switch(id) 
            {
                case 10:    // Selected and Dropped Items
                {
                    emit signalProgressBarMode(StatusProgressBar::ProgressBarMode, 
                                               i18n("Assign tag to pictures. Please wait..."));
                    
                    float cnt = (float)countSelected();
                    for (IconItem *it = firstItem(); it; it = it->nextItem())
                    {
                        if (it->isSelected())
                        {
                            AlbumIconItem *albumItem = static_cast<AlbumIconItem *>(it);
                            albumItem->imageInfo()->setTag(tagID);

                            // TODO MetadataHub: add call here.

                            emit signalProgressValue((int)((i++/cnt)*100.0));
                            kapp->processEvents();
                        }
                    }

                    AlbumIconItem *albumItem = findItem(event->pos());
                    if (albumItem)
                    {
                        albumItem->imageInfo()->setTag(tagID);

                        // TODO MetadataHub: add call here.
                    }
    
                    d->imageLister->refresh();
                    updateContents();
                    emit signalProgressBarMode(StatusProgressBar::TextMode, QString::null);
                    break;
                }
                case 11:    // All Items
                {
                    emit signalProgressBarMode(StatusProgressBar::ProgressBarMode, 
                                               i18n("Assign tag to pictures. Please wait..."));

                    float cnt = (float)count();
                    for (IconItem *it = firstItem(); it; it = it->nextItem())
                    {
                        AlbumIconItem *albumItem = static_cast<AlbumIconItem *>(it);
                        albumItem->imageInfo()->setTag(tagID);

                        // TODO MetadataHub: add call here.

                        emit signalProgressValue((int)((i++/cnt)*100.0));
                        kapp->processEvents();
                    }
    
                    d->imageLister->refresh();
                    updateContents();
                    emit signalProgressBarMode(StatusProgressBar::TextMode, QString::null);
                    break;
                }
                case 12:    // Dropped Item only.
                {
                    AlbumIconItem *albumItem = findItem(event->pos());
                    if (albumItem)
                    {
                        albumItem->imageInfo()->setTag(tagID);

                        // TODO MetadataHub: add call here.
                    }

                    d->imageLister->refresh();
                    updateContents();
                    break;
                }
                default:
                    break;
            }
        }
    }
    else if(TagListDrag::canDecode(event))
    {
        QByteArray ba = event->encodedData("digikam/taglist");
        QDataStream ds(ba, IO_ReadOnly);
        int i = 0;
        QValueList<int> tagIDs;
        ds >> tagIDs;

        QPopupMenu popMenu(this);

        bool itemsSelected = false;
        for (IconItem *it = firstItem(); it; it = it->nextItem())
            if (it->isSelected())
                itemsSelected = true;

        bool itemDropped = false;
        AlbumIconItem *albumItem = findItem(event->pos());
        if (albumItem) 
            itemDropped = true;

        popMenu.insertItem(SmallIcon("tag"), i18n("Assign Tags to &All Items"),          11);

        if (itemsSelected)
            popMenu.insertItem(SmallIcon("tag"), i18n("Assign Tags to &Selected Items"), 10);

        if (itemDropped)
            popMenu.insertItem(SmallIcon("tag"), i18n("Assign Tags to &Dropped Item"),   12);
    
        popMenu.insertSeparator(-1);
        popMenu.insertItem(SmallIcon("cancel"), i18n("&Cancel"));

        popMenu.setMouseTracking(true);
        int id = popMenu.exec(QCursor::pos());
        switch(id) 
        {
            case 10:    // Selected and Dropped Items
            {
                emit signalProgressBarMode(StatusProgressBar::ProgressBarMode, 
                                            i18n("Assign tags to pictures. Please wait..."));

                float cnt = (float)countSelected();
                for (IconItem *it = firstItem(); it; it = it->nextItem())
                {
                    if (it->isSelected())
                    {
                        AlbumIconItem *albumItem = static_cast<AlbumIconItem*>(it);
                        for (QValueList<int>::iterator it = tagIDs.begin();
                            it != tagIDs.end(); ++it)
                        {
                            albumItem->imageInfo()->setTag(*it);

                            // TODO MetadataHub: add call here.
                        }

                        emit signalProgressValue((int)((i++/cnt)*100.0));
                        kapp->processEvents();
                    }
                }

                AlbumIconItem *albumItem = findItem(event->pos());
                if (albumItem)
                {
                    for (QValueList<int>::iterator it = tagIDs.begin();
                        it != tagIDs.end(); ++it)
                    {
                        albumItem->imageInfo()->setTag(*it);

                        // TODO MetadataHub: add call here.
                    }
                }

                emit signalProgressBarMode(StatusProgressBar::TextMode, QString::null);    
                d->imageLister->refresh();
                updateContents();
                break;
            }
            case 11:    // All Items
            {
                emit signalProgressBarMode(StatusProgressBar::ProgressBarMode, 
                                            i18n("Assign tags to pictures. Please wait..."));
                
                float cnt = (float)count();
                for (IconItem *it = firstItem(); it; it = it->nextItem())
                {
                    AlbumIconItem *albumItem = static_cast<AlbumIconItem*>(it);
                    for (QValueList<int>::iterator it = tagIDs.begin();
                        it != tagIDs.end(); ++it)
                    {
                        albumItem->imageInfo()->setTag(*it);

                        // TODO MetadataHub: add call here.
                    }

                    emit signalProgressValue((int)((i++/cnt)*100.0));
                    kapp->processEvents();
                }

                emit signalProgressBarMode(StatusProgressBar::TextMode, QString::null);    
                d->imageLister->refresh();
                updateContents();
                break;
            }
            case 12:    // Dropped item only.
            {
                AlbumIconItem *albumItem = findItem(event->pos());
                if (albumItem)
                {
                    for (QValueList<int>::iterator it = tagIDs.begin();
                        it != tagIDs.end(); ++it)
                    {
                        albumItem->imageInfo()->setTag(*it);

                        // TODO MetadataHub: add call here.
                    }
                }

                d->imageLister->refresh();
                updateContents();
                break;
            }
            default:
                break;
        }
    }
    else 
    {
        event->ignore();
    }
}

bool AlbumIconView::acceptToolTip(IconItem *item, const QPoint &mousePos)
{
    AlbumIconItem *iconItem = dynamic_cast<AlbumIconItem*>(item);

    if (iconItem && iconItem->thumbnailRect().contains(mousePos))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void AlbumIconView::slotShowToolTip(IconItem* item)
{
    d->toolTip->setIconItem(dynamic_cast<AlbumIconItem*>(item));
}

KURL::List AlbumIconView::allItems()
{
    KURL::List itemList;

     for (IconItem *it = firstItem(); it; it = it->nextItem())
     {
         AlbumIconItem *item = (AlbumIconItem*) it;
         itemList.append(item->imageInfo()->kurl());
     }

    return itemList;
}

KURL::List AlbumIconView::selectedItems()
{
    KURL::List itemList;

     for (IconItem *it = firstItem(); it; it = it->nextItem())
     {
         if (it->isSelected())
         {
             AlbumIconItem *item = (AlbumIconItem*) it;
             itemList.append(item->imageInfo()->kurl());
         }
     }

    return itemList;
}

QPtrList<ImageInfo> AlbumIconView::selectedImageInfos(bool copy) const
{
    // Returns the list of ImageInfos of currently selected items,
    // with the extra feature that the currentItem is the first in the list.
    QPtrList<ImageInfo> list;
    for (IconItem *it = firstItem(); it; it = it->nextItem())
    {
        AlbumIconItem *iconItem = static_cast<AlbumIconItem *>(it);
        if (it->isSelected())
        {
            ImageInfo *info = iconItem->imageInfo();
            if (copy)
                info = new ImageInfo(*info);

            if (iconItem == currentItem())
                list.prepend(info);
            else
                list.append(info);
        }
    }
    return list;
}

void AlbumIconView::refresh()
{
    d->imageLister->stop();
    clear();

    d->imageLister->openAlbum(d->currentAlbum);
}

void AlbumIconView::refreshItems(const KURL::List& urlList)
{
    if (!d->currentAlbum || urlList.empty())
        return;

    // we do two things here:
    // 1. refresh the imageinfo for the file
    // 2. refresh the thumbnails

    for (KURL::List::const_iterator it = urlList.begin();
         it != urlList.end(); ++it)
    {
        AlbumIconItem* iconItem = findItem((*it).url());
        if (!iconItem)
            continue;

        iconItem->imageInfo()->refresh();
        d->pixMan->remove(iconItem->imageInfo()->kurl());
        // clean LoadingCache as well - be pragmatic, do it here.
        LoadingCacheInterface::cleanFromCache((*it).path());
    }

    emit signalItemsUpdated(urlList);

    // trigger a delayed update, in case we need to resort items
    triggerUpdate();
}

void AlbumIconView::slotGotThumbnail(const KURL& url)
{
    AlbumIconItem* iconItem = findItem(url.url());
    if (!iconItem)
        return;

    iconItem->repaint();
}

void AlbumIconView::slotSelectionChanged()
{
    if (firstSelectedItem())
        emitItemsSelected(true);
    else
        emitItemsSelected(false);
}

void AlbumIconView::slotSetExifOrientation( int orientation )
{
    KURL::List urlList;
    int i = 0;

    for (IconItem *it = firstItem(); it; it=it->nextItem())
    {
        if (it->isSelected()) 
        {
            AlbumIconItem *iconItem = static_cast<AlbumIconItem *>(it);
            urlList.append(iconItem->imageInfo()->kurl());
        }
    }

    if (urlList.count() <= 0) return;

    KURL::List::Iterator it;
    float cnt = (float)urlList.count();
    emit signalProgressBarMode(StatusProgressBar::ProgressBarMode, 
                                i18n("Fix Exif Orientation tag on pictures. Please wait..."));

    for( it = urlList.begin(); it != urlList.end(); ++it )
    {
        DDebug() << "Setting Exif Orientation tag to " << orientation << endl;

        DMetadata metadata((*it).path());
        DMetadata::ImageOrientation o = (DMetadata::ImageOrientation)orientation;
        metadata.setImageOrientation(o);

        if (!metadata.applyChanges())
        {
            KMessageBox::sorry(0, i18n("Failed to correct Exif orientation for file %1.")
                               .arg((*it).filename()));
            return;
        }
        else
        {
            ImageAttributesWatch::instance()->fileMetadataChanged((*it));
        }

        emit signalProgressValue((int)((i++/cnt)*100.0));
        kapp->processEvents();
    }

    emit signalProgressBarMode(StatusProgressBar::TextMode, QString::null);    
    refreshItems(urlList);
}

QRect AlbumIconView::itemRect() const
{
    return d->itemRect;
}

QRect AlbumIconView::itemRatingRect() const
{
    return d->itemRatingRect;
}

QRect AlbumIconView::itemDateRect() const
{
    return d->itemDateRect;
}

QRect AlbumIconView::itemModDateRect() const
{
    return d->itemModDateRect;
}

QRect AlbumIconView::itemPixmapRect() const
{
    return d->itemPixmapRect;
}

QRect AlbumIconView::itemNameRect() const
{
    return d->itemNameRect;
}

QRect AlbumIconView::itemCommentsRect() const
{
    return d->itemCommentsRect;
}

QRect AlbumIconView::itemResolutionRect() const
{
    return d->itemResolutionRect;
}

QRect AlbumIconView::itemTagRect() const
{
    return d->itemTagRect;
}

QRect AlbumIconView::itemSizeRect() const
{
    return d->itemSizeRect;
}

QRect AlbumIconView::bannerRect() const
{
    return d->bannerRect;
}

QPixmap* AlbumIconView::itemBaseRegPixmap() const
{
    return &d->itemRegPixmap;
}

QPixmap* AlbumIconView::itemBaseSelPixmap() const
{
    return &d->itemSelPixmap;
}

QPixmap AlbumIconView::bannerPixmap() const
{
    return d->bannerPixmap;
}

QPixmap AlbumIconView::ratingPixmap() const
{
    return d->ratingPixmap;    
}

QFont AlbumIconView::itemFontReg() const
{
    return d->fnReg;
}

QFont AlbumIconView::itemFontCom() const
{
    return d->fnCom;
}

QFont AlbumIconView::itemFontXtra() const
{
    return d->fnXtra;
}

void AlbumIconView::updateBannerRectPixmap()
{
    d->bannerRect = QRect(0, 0, 0, 0);

    // Title --------------------------------------------------------
    QFont fn(font());
    int fnSize = fn.pointSize();
    bool usePointSize;
    if (fnSize > 0)
    {
        fn.setPointSize(fnSize+2);
        usePointSize = true;
    }
    else
    {
        fnSize = fn.pixelSize();
        fn.setPixelSize(fnSize+2);
        usePointSize = false;
    }

    fn.setBold(true);
    QFontMetrics fm(fn);
    QRect tr = fm.boundingRect(0, 0, frameRect().width(),
                               0xFFFFFFFF, Qt::AlignLeft | Qt::AlignVCenter,
                               "XXX");
    d->bannerRect.setHeight(tr.height());

    if (usePointSize)
        fn.setPointSize(font().pointSize());
    else
        fn.setPixelSize(font().pixelSize());

    fn.setBold(false);
    fm = QFontMetrics(fn);

    tr = fm.boundingRect(0, 0, frameRect().width(),
                         0xFFFFFFFF, Qt::AlignLeft | Qt::AlignVCenter,
                         "XXX");

    d->bannerRect.setHeight(d->bannerRect.height() + tr.height() + 10);
    d->bannerRect.setWidth(frameRect().width());

    d->bannerPixmap = ThemeEngine::instance()->bannerPixmap(d->bannerRect.width(),
                                                            d->bannerRect.height());
}

void AlbumIconView::updateItemRectsPixmap()
{
    d->itemRect           = QRect(0,0,0,0);
    d->itemRatingRect     = QRect(0,0,0,0);
    d->itemDateRect       = QRect(0,0,0,0);
    d->itemModDateRect    = QRect(0,0,0,0);
    d->itemPixmapRect     = QRect(0,0,0,0);
    d->itemNameRect       = QRect(0,0,0,0);
    d->itemCommentsRect   = QRect(0,0,0,0);
    d->itemResolutionRect = QRect(0,0,0,0);
    d->itemSizeRect       = QRect(0,0,0,0);
    d->itemTagRect        = QRect(0,0,0,0);

    d->fnReg  = font();
    d->fnCom  = font();
    d->fnXtra = font();
    d->fnCom.setItalic(true);

    int fnSz = d->fnReg.pointSize();
    if (fnSz > 0)
    {
        d->fnCom.setPointSize(fnSz-1);
        d->fnXtra.setPointSize(fnSz-2);
    }
    else
    {
        fnSz = d->fnReg.pixelSize();
        d->fnCom.setPixelSize(fnSz-1);
        d->fnXtra.setPixelSize(fnSz-2);
    }

    int margin  = 5;
    int w = d->thumbSize.size() + 2*margin;

    QFontMetrics fm(d->fnReg);
    QRect oneRowRegRect = fm.boundingRect(0, 0, w, 0xFFFFFFFF,
                                          Qt::AlignTop | Qt::AlignHCenter,
                                          "XXXXXXXXX");
    fm = QFontMetrics(d->fnCom);
    QRect oneRowComRect = fm.boundingRect(0, 0, w, 0xFFFFFFFF,
                                          Qt::AlignTop | Qt::AlignHCenter,
                                          "XXXXXXXXX");
    fm = QFontMetrics(d->fnXtra);
    QRect oneRowXtraRect = fm.boundingRect(0, 0, w, 0xFFFFFFFF,
                                           Qt::AlignTop | Qt::AlignHCenter,
                                           "XXXXXXXXX");

    int y = margin;

    d->itemPixmapRect = QRect(margin, y, w, d->thumbSize.size()+margin);
    y = d->itemPixmapRect.bottom();

    if (d->albumSettings->getIconShowRating())
    {
        d->itemRatingRect = QRect(margin, y, w, d->ratingPixmap.height());
        y = d->itemRatingRect.bottom();
    }
    
    if (d->albumSettings->getIconShowName())
    {
        d->itemNameRect = QRect(margin, y, w, oneRowRegRect.height());
        y = d->itemNameRect.bottom();
    }

    if (d->albumSettings->getIconShowComments())
    {
        d->itemCommentsRect = QRect(margin, y, w, oneRowComRect.height());
        y = d->itemCommentsRect.bottom();
    }


    if (d->albumSettings->getIconShowDate())
    {
        d->itemDateRect = QRect(margin, y, w, oneRowXtraRect.height());
        y = d->itemDateRect.bottom();
    }

    if (d->albumSettings->getIconShowModDate())
    {
        d->itemModDateRect = QRect(margin, y, w, oneRowXtraRect.height());
        y = d->itemModDateRect.bottom();
    }

    if (d->albumSettings->getIconShowResolution())
    {
        d->itemResolutionRect = QRect(margin, y, w, oneRowXtraRect.height());
        y = d->itemResolutionRect.bottom() ;
    }

    if (d->albumSettings->getIconShowSize())
    {
        d->itemSizeRect = QRect(margin, y, w, oneRowXtraRect.height());
        y = d->itemSizeRect.bottom();
    }

    if (d->albumSettings->getIconShowTags())
    {
        d->itemTagRect = QRect(margin, y, w, oneRowComRect.height());
        y = d->itemTagRect.bottom();
    }

    d->itemRect = QRect(0, 0, w+2*margin, y+margin);

    d->itemRegPixmap = ThemeEngine::instance()->thumbRegPixmap(d->itemRect.width(),
                                                               d->itemRect.height());

    d->itemSelPixmap = ThemeEngine::instance()->thumbSelPixmap(d->itemRect.width(),
                                                               d->itemRect.height());
}

void AlbumIconView::slotThemeChanged()
{
    QPalette plt(palette());
    QColorGroup cg(plt.active());
    cg.setColor(QColorGroup::Base, ThemeEngine::instance()->baseColor());
    cg.setColor(QColorGroup::Text, ThemeEngine::instance()->textRegColor());
    cg.setColor(QColorGroup::HighlightedText, ThemeEngine::instance()->textSelColor());
    plt.setActive(cg);
    plt.setInactive(cg);
    setPalette(plt);

    QPainter painter(&d->ratingPixmap);
    painter.fillRect(0, 0, d->ratingPixmap.width(), d->ratingPixmap.height(),
                     ThemeEngine::instance()->textSpecialRegColor());
    painter.end();
    
    updateBannerRectPixmap();
    updateItemRectsPixmap();

    viewport()->update();
}

AlbumIconItem* AlbumIconView::findItem(const QPoint& pos)
{
    return dynamic_cast<AlbumIconItem*>(IconView::findItem(pos));
}

AlbumIconItem* AlbumIconView::findItem(const QString& url) const
{
    return d->itemDict.find(url);
}

AlbumIconItem* AlbumIconView::nextItemToThumbnail() const
{
    QRect r(contentsX(), contentsY(), visibleWidth(), visibleHeight());
    IconItem *fItem = findFirstVisibleItem(r);
    IconItem *lItem = findLastVisibleItem(r);
    if (!fItem || !lItem)
        return 0;

    AlbumIconItem* firstItem = static_cast<AlbumIconItem*>(fItem);
    AlbumIconItem* lastItem  = static_cast<AlbumIconItem*>(lItem);
    AlbumIconItem* item      = firstItem;
    while (item)
    {
        if (item->isDirty())
            return item;
        if (item == lastItem)
            break;
        item = (AlbumIconItem*)item->nextItem();
    }

    return 0;
}

PixmapManager* AlbumIconView::pixmapManager() const
{
    return d->pixMan;
}

void AlbumIconView::slotAlbumModified()
{
    d->imageLister->stop();
    clear();

    d->imageLister->openAlbum(d->currentAlbum);

    updateBannerRectPixmap();
    updateItemRectsPixmap();
}

void AlbumIconView::slotAssignTag(int tagID)
{
    emit signalProgressBarMode(StatusProgressBar::ProgressBarMode, 
                                i18n("Assign tag to pictures. Please wait..."));

    int   i   = 0;
    float cnt = (float)countSelected();

    for (IconItem *it = firstItem(); it; it = it->nextItem())
    {
        if (it->isSelected())
        {
            AlbumIconItem *albumItem = static_cast<AlbumIconItem *>(it);
            ImageInfo* info          = albumItem->imageInfo();
            QStringList oldKeywords  = info->tagPaths();
            for (QStringList::iterator it = oldKeywords.begin(); it != oldKeywords.end(); ++it)
                (*it).remove(0, 1);

            info->setTag(tagID);

            // TODO MetadataHub: fix this part to use it instead.

            // Store Image Tags like Iptc keywords tag.
        
            if (AlbumSettings::instance())
            {
                if (AlbumSettings::instance()->getSaveIptcTags())
                {
                    QStringList tagPaths = info->tagPaths();
                    for (QStringList::iterator it = tagPaths.begin(); it != tagPaths.end(); ++it)
                        (*it).remove(0, 1);

                    DMetadata metadata(info->filePath());
                    metadata.setImageKeywords(oldKeywords, tagPaths);
                    metadata.applyChanges();
                    ImageAttributesWatch::instance()->fileMetadataChanged(info->kurl());
                }
            }

            emit signalProgressValue((int)((i++/cnt)*100.0));
            kapp->processEvents();
        }
    }

    emit signalProgressBarMode(StatusProgressBar::TextMode, QString::null);
    updateContents();
}

void AlbumIconView::slotRemoveTag(int tagID)
{
    emit signalProgressBarMode(StatusProgressBar::ProgressBarMode, 
                                i18n("Remove tag from pictures. Please wait..."));

    int   i   = 0;
    float cnt = (float)countSelected();

    for (IconItem *it = firstItem(); it; it = it->nextItem())
    {
        if (it->isSelected())
        {
            AlbumIconItem *albumItem = static_cast<AlbumIconItem *>(it);
            ImageInfo* info          = albumItem->imageInfo();
            QStringList oldKeywords  = info->tagPaths();
            for (QStringList::iterator it = oldKeywords.begin(); it != oldKeywords.end(); ++it)
                (*it).remove(0, 1);

            info->removeTag(tagID);

            // TODO MetadataHub: fix this part to use it instead.

            // Update Image Tags like Iptc keywords tags.

            if (AlbumSettings::instance())
            {
                if (AlbumSettings::instance()->getSaveIptcTags())
                {
                    QStringList tagPaths = info->tagPaths();
                    for (QStringList::iterator it = tagPaths.begin(); it != tagPaths.end(); ++it)
                        (*it).remove(0, 1);

                    DMetadata metadata(info->filePath());
                    metadata.setImageKeywords(oldKeywords, tagPaths);
                    metadata.applyChanges();
                    ImageAttributesWatch::instance()->fileMetadataChanged(info->kurl());
                }
            }

            emit signalProgressValue((int)((i++/cnt)*100.0));
            kapp->processEvents();
        }
    }

    emit signalProgressBarMode(StatusProgressBar::TextMode, QString::null);

    if (d->currentAlbum && d->currentAlbum->type() == Album::TAG)
    {
        d->imageLister->refresh();
    }
    updateContents();
}

void AlbumIconView::slotAssignRating(int rating)
{
    emit signalProgressBarMode(StatusProgressBar::ProgressBarMode, 
                                i18n("Assign rating to pictures. Please wait..."));

    int   i   = 0;
    float cnt = (float)countSelected();
    rating    = QMIN(5, QMAX(0, rating));
    
    for (IconItem *it = firstItem() ; it ; it = it->nextItem())
    {
        if (it->isSelected())
        {
            AlbumIconItem *albumItem = static_cast<AlbumIconItem *>(it);
            ImageInfo* info          = albumItem->imageInfo();
            info->setRating(rating);

            // TODO MetadataHub: fix this part to use it instead.
            
            // Store Image rating as Iptc tag.
        
            if (AlbumSettings::instance())
            {
                if (AlbumSettings::instance()->getSaveIptcRating())
                {
                    DMetadata metadata(info->filePath());
                    metadata.setImageRating(rating);
                    metadata.applyChanges();
                    ImageAttributesWatch::instance()->fileMetadataChanged(info->kurl());
                }
            }

            emit signalProgressValue((int)((i++/cnt)*100.0));
            kapp->processEvents();
        }
    }

    emit signalProgressBarMode(StatusProgressBar::TextMode, QString::null);
    triggerUpdate();
}

void AlbumIconView::slotAssignRatingNoStar()
{
    slotAssignRating(0);
}

void AlbumIconView::slotAssignRatingOneStar()
{
    slotAssignRating(1);
}

void AlbumIconView::slotAssignRatingTwoStar()
{
    slotAssignRating(2);
}

void AlbumIconView::slotAssignRatingThreeStar()
{
    slotAssignRating(3);
}

void AlbumIconView::slotAssignRatingFourStar()
{
    slotAssignRating(4);
}

void AlbumIconView::slotAssignRatingFiveStar()
{
    slotAssignRating(5);
}

void AlbumIconView::slotDIOResult(KIO::Job* job)
{
    if (job->error())
        job->showErrorDialog(this);
}

void AlbumIconView::slotImageAttributesChanged(Q_LLONG /*imageId*/)
{
    // we might check if the item with imageId is currently visible,
    // but I think it is ok to simply repaint in any case.
    // We also do not need to check whether the change originates
    // from our own actions above, the additional update should be killed by Qt.
    updateContents();
}

void AlbumIconView::slotAlbumImagesChanged(int /*albumId*/)
{
    // Same considerations as above
    updateContents();
}

}  // namespace Digikam
