/******************************************************************************
**
** This file is part of libcommhistory.
**
** Copyright (C) 2014 Jolla Ltd.
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

#include "contactresolver.h"

#include <QElapsedTimer>

#include <seasidecache.h>

#include "commonutils.h"
#include "debug.h"

// The resolver will take a list of events and ask the SeasideCache to resolve their
// addresses to contacts.
//
// When all events are either resolved as unknown or resolved to a contact, the
// resolver will emit eventsResolved(QList<Event>) and finished() signals.
// The events passed to eventsResolved will have valid contacts() lists.

using namespace CommHistory;

namespace CommHistory {

typedef QPair<QString, QString> UidPair;

// Implementation logic:
// - Normalize and minimize the addresses in the events before requesting
//   address resolution, to minimize the number of requests.
// - Assume one addressResolved callback for each request made, and just count
//   them to track completion.
// - When all resolve requests are complete, query the best match for every event
//   by passing the un-minimized addresses to itemByXXX functions. This gets the
//   best match in cases where a minimized phone number matches multiple contacts.
// - Then hand the events list over to the event model (via the eventsResolved signal).
//   The event model becomes responsible for tracking any changes to contacts.
//   The resolver avoids having to track these by only querying the cache when it's done.

class ContactResolverPrivate : public QObject, public SeasideCache::ResolveListener
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(ContactResolver)

public:
    ContactResolver *q_ptr;
    QList<Event> events; // events to be resolved

    // All uidpairs submitted for address resolution
    QSet<UidPair> requestedAddresses;
    // how many have completed
    int resolvedAddresses;

    QElapsedTimer resolveTimer;

    explicit ContactResolverPrivate(ContactResolver *parent);

    void resolveEvent(const Event &event);
    void checkIfResolved();
    QList<Event> applyResolvedContacts() const;

    // Return a localUid/remoteUid pair in a form that can be
    // used for comparisons and set/hash lookups.
    static UidPair foldedAddress(const QString &localUid, const QString &remoteUid);
    static UidPair foldedEventAddress(const Event &event);
    static UidPair foldedEventAddress(const QString &localUid, const QString &remoteUid);

    // ResolveListener callback from SeasideCache
    virtual void addressResolved(const QString &first, const QString &second, SeasideCache::CacheItem *);
};

} // namespace CommHistory

ContactResolver::ContactResolver(QObject *parent)
    : QObject(parent), d_ptr(new ContactResolverPrivate(this))
{
}

ContactResolverPrivate::ContactResolverPrivate(ContactResolver *parent)
    : QObject(parent), q_ptr(parent), resolvedAddresses(0)
{
}

QList<Event> ContactResolver::events() const
{
    Q_D(const ContactResolver);
    return d->applyResolvedContacts();
}

bool ContactResolver::isResolving() const
{
    Q_D(const ContactResolver);
    return !d->events.isEmpty();
}

void ContactResolver::appendEvents(const QList<Event> &events)
{
    Q_D(ContactResolver);

    if (d->events.isEmpty())
        d->resolveTimer.start();

    foreach (const Event &event, events) {
        d->resolveEvent(event);
        d->events.append(event);
    }

    d->checkIfResolved();
}

void ContactResolver::prependEvents(const QList<Event> &events)
{
    Q_D(ContactResolver);

    if (d->events.isEmpty())
        d->resolveTimer.start();

    foreach (const Event &event, events) {
        d->resolveEvent(event);
        d->events.prepend(event);
    }

    d->checkIfResolved();
}

void ContactResolverPrivate::resolveEvent(const Event &event)
{
    if (event.localUid().isEmpty() || event.remoteUid().isEmpty())
        return;

    UidPair uidPair(foldedEventAddress(event));

    if (requestedAddresses.contains(uidPair))
        return;
    requestedAddresses.insert(uidPair);

    if (uidPair.first.isEmpty())
        SeasideCache::resolvePhoneNumber(this, uidPair.second, true);
    else if (uidPair.second.isEmpty())
        SeasideCache::resolveEmailAddress(this, uidPair.first, true);
    else
        SeasideCache::resolveOnlineAccount(this, uidPair.first, uidPair.second, true);
}

void ContactResolverPrivate::addressResolved(const QString &, const QString &, SeasideCache::CacheItem *)
{
    resolvedAddresses++;
    checkIfResolved();
}

QList<Event> ContactResolverPrivate::applyResolvedContacts() const
{
    QList<Event> resolved = events;
    // Give each event the contact that was found for its address,
    // or an empty contact list if none was found.
    QList<Event>::iterator it = resolved.begin(), end = resolved.end();
    for ( ; it != end; ++it) {
        Event &event(*it);
        QString localUid(event.localUid());
        QString remoteUid(event.remoteUid());

        // Ask the cache for the best matching contact for each event.
        SeasideCache::CacheItem *item;
        if (localUidComparesPhoneNumbers(localUid))
            item = SeasideCache::itemByPhoneNumber(remoteUid, true);
        else if (remoteUid.isEmpty())
            item = SeasideCache::itemByEmailAddress(localUid, true);
        else
            item = SeasideCache::itemByOnlineAccount(localUid, remoteUid, true);

        if (item) {
            QString label = SeasideCache::generateDisplayLabel(item->contact, SeasideCache::displayLabelOrder());
            event.setContacts(QList<Event::Contact>() << qMakePair(item->iid, label));
        } else {
            event.setContacts(QList<Event::Contact>());
        }
    }
    return resolved;
}

UidPair ContactResolverPrivate::foldedAddress(const QString &localUid, const QString &remoteUid)
{
    if (localUid.isEmpty()) {
        QString remote = minimizePhoneNumber(remoteUid);
        if (remote.isEmpty())
            remote = remoteUid;
        return qMakePair(QString(), remote.toCaseFolded());
    }
    return qMakePair(localUid.toCaseFolded(),
                     remoteUid.toCaseFolded());
}

UidPair ContactResolverPrivate::foldedEventAddress(const QString &localUid, const QString &remoteUid)
{
    if (localUidComparesPhoneNumbers(localUid))
        return foldedAddress(QString(), remoteUid);
    return qMakePair(localUid.toCaseFolded(),
                     remoteUid.toCaseFolded());
}

UidPair ContactResolverPrivate::foldedEventAddress(const Event &event)
{
    return foldedEventAddress(event.localUid(), event.remoteUid());
}

void ContactResolverPrivate::checkIfResolved()
{
    Q_Q(ContactResolver);

    if (events.isEmpty())
        return;

    if (resolvedAddresses < requestedAddresses.size())
        return;

    QList<Event> resolved = applyResolvedContacts();
    events.clear();

    qDebug() << "Resolved" << resolved.count() << "events in" << resolveTimer.elapsed() << "msec";

    emit q->eventsResolved(resolved);
    emit q->finished();
}

#include "contactresolver.moc"
