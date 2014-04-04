/******************************************************************************
**
** This file is part of libcommhistory.
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: John Brooks <john.brooks@jolla.com>
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

#ifndef COMMHISTORY_RECIPIENT_H
#define COMMHISTORY_RECIPIENT_H

#include <QObject>
#include <QDBusArgument>
#include <QSharedPointer>
#include <QHash>
#include <QDebug>

#include "libcommhistoryexport.h"

namespace CommHistory {

/* TODO:
 * - Details of caching instances
 * - Notification of changes in contact resolution
 * - Handling for multiple matching contacts on a recipient
 * - hidden/private numbers
 * - Outgoing events with multiple recipients
 * - Number normalization
 * - DB representation
 */

/* WIP for groups:
 *
 * Contact resolution
 *
 * To satisfy model needs, we need a type which can take a list
 * of Recipient, make sure they're all resolved, and emit a signal.
 *
 * We also need a type which can monitor for changes to any of a
 * list of recipient. RecipientMonitor does this now. Can possibly
 * be the same type as the previous point.
 */

class RecipientPrivate;
typedef QWeakPointer<RecipientPrivate> WeakRecipient;

/* Represents one remote peer's address
 *
 * A recipient represents the pair of a localUid (local account address)
 * and remoteUid (remote peer's address on that account). It centralizes
 * logic for comparing addresses, display, and resolving matching contacts.
 *
 * Recipient's data is shared among instances which represent exactly the
 * same address, which ensures contact caching and consistent representation.
 *
 * Instances may be equal without being exactly identical, e.g. when using
 * minimized phone number comparisons. In that case, they may not be shared.
 */
class LIBCOMMHISTORY_EXPORT Recipient
{
public:
    Recipient();
    Recipient(const QString &localUid, const QString &remoteUid);
    Recipient(const Recipient &o);
    Recipient(const WeakRecipient &weak);
    Recipient &operator=(const Recipient &o);
    ~Recipient();

    bool isNull() const;
 
    QString displayName() const;

    QString localUid() const;
    QString remoteUid() const;
    bool isPhoneNumber() const;
    QString minimizedPhoneNumber() const;

    bool operator==(const Recipient &o) const;
    bool matches(const Recipient &o) const;
    bool matches(const QString &remoteUid) const;
    bool isSameContact(const Recipient &o) const;

    int contactId() const;
    QString contactName() const;
    bool isContactResolved() const;

    /* Update the resolved contact information for this recipient
     *
     * Generally, this is only called by the contact resolver, but it can be
     * used to inject known contact matches. The change will apply to all
     * Recipient instances that compare equal to this instance.
     *
     * A contactId of 0 is taken to mean that no contact matches. In this
     * case, the contact is still considered resolved.
     */
    void setResolvedContact(int contactId, const QString &contactName) const;

    /* Get all existing recipients that are resolved to a contact ID
     *
     * This is primarily used for contact change notifications. contactId
     * may be 0, which returns all Recipient which have been resolved but
     * had no contact matches.
     */
    static QList<Recipient> recipientsForContact(int contactId);

private:
    QSharedPointer<RecipientPrivate> d;

    friend uint qHash(const CommHistory::Recipient &value, uint seed);
};

class LIBCOMMHISTORY_EXPORT RecipientList
{
public:
    RecipientList();
    RecipientList(const Recipient &recipient);
    RecipientList(const QList<Recipient> &recipients);

    static RecipientList fromUids(const QString &localUid, const QStringList &remoteUids);

    bool isEmpty() const;
    int size() const;

    QList<Recipient> recipients() const;
    QStringList displayNames() const;
    QList<int> contactIds() const;

    QStringList remoteUids() const;
    
    bool operator==(const RecipientList &o) const;
    bool matches(const RecipientList &o) const;
    bool hasSameContacts(const RecipientList &o) const;

    bool allContactsResolved() const;

    QString debugString() const;

    typedef QList<Recipient>::const_iterator const_iterator;
    typedef QList<Recipient>::iterator iterator;
    const_iterator begin() const;
    const_iterator end() const;

    Recipient value(int index) const;

    RecipientList &operator<<(const Recipient &recipient);

private:
    QList<Recipient> m_recipients;
};

inline uint qHash(const CommHistory::Recipient &value, uint seed = 0)
{
    return ::qHash(value.d.data(), seed);
}

}

LIBCOMMHISTORY_EXPORT QDebug &operator<<(QDebug &debug, const CommHistory::Recipient &recipient);
LIBCOMMHISTORY_EXPORT QDebug &operator<<(QDebug &debug, const CommHistory::RecipientList &recipients);

QDBusArgument &operator<<(QDBusArgument &argument, const CommHistory::Recipient &recipient);
const QDBusArgument &operator>>(const QDBusArgument &argument, CommHistory::Recipient &recipient);

QDBusArgument &operator<<(QDBusArgument &argument, const CommHistory::RecipientList &recipients);
const QDBusArgument &operator>>(const QDBusArgument &argument, CommHistory::RecipientList &recipients);

Q_DECLARE_METATYPE(CommHistory::Recipient)
Q_DECLARE_METATYPE(CommHistory::RecipientList)

#endif

