/******************************************************************************
**
** This file is part of libcommhistory.
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: John Brooks <john.brooks@jollamobile.com>
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of the GNU Lesser General Public License version 2.1 as
** published by the Free Software Foundation.
**
** This library is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
** or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
** License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this library; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
**
******************************************************************************/

#ifndef COMMHISTORY_CONTACTLISTENER_P_H
#define COMMHISTORY_CONTACTLISTENER_P_H

#include "contactlistener.h"
#include <QObject>
#include <seasidecache.h>

namespace CommHistory {

class ContactResolver;

class ContactListenerPrivate
    : public QObject,
      public SeasideCache::ChangeListener
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(ContactListener)

public:
    ContactListenerPrivate(ContactListener *q);
    virtual ~ContactListenerPrivate();

    ContactResolver *retryResolver;
    QList<Recipient> retryRecipients;

private slots:
    void retryFinished();
    void resolveAgain(const CommHistory::Recipient &recipient);

protected:
    void itemUpdated(SeasideCache::CacheItem *item);
    void itemAboutToBeRemoved(SeasideCache::CacheItem *item);

private:
    ContactListener *q_ptr;
};

}

#endif
