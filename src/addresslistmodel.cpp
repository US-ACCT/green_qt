#include "addresslistmodel.h"

#include "account.h"
#include "address.h"
#include "handlers/getaddresseshandler.h"
#include "resolver.h"

AddressListModel::AddressListModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_reload_timer(new QTimer(this))
{
    m_reload_timer->setSingleShot(true);
    m_reload_timer->setInterval(200);
    connect(m_reload_timer, &QTimer::timeout, this, [&] {
        fetch(true);
    });
}

AddressListModel::~AddressListModel()
{
}

void AddressListModel::setAccount(Account* account)
{
    if (m_account) {
        disconnect(m_account, &Account::addressGenerated, this, &AddressListModel::reload);
        beginResetModel();
        m_handler = nullptr;
        m_addresses.clear();
        m_account = nullptr;
        emit accountChanged();
        endResetModel();
    }
    if (!account) return;
    m_account = account;
    emit accountChanged();
    if (m_account) {
        connect(m_account, &Account::addressGenerated, this, &AddressListModel::reload);
        fetchMore(QModelIndex());
    }
}

void AddressListModel::fetch(bool reset)
{
    if (m_handler) return;

    auto handler = new GetAddressesHandler(m_last_pointer, m_account);

    QObject::connect(handler, &Handler::done, this, [this, reset, handler] {
        handler->deleteLater();

        if (m_handler != handler) {
            reload();
            return;
        }

        m_fetching = false;
        m_handler = nullptr;
        m_last_pointer = handler->lastPointer();
        emit fetchingChanged();
        // instantiate missing addresses
        QVector<Address*> addresses;
        for (QJsonValue data : handler->addresses()) {
            auto address = m_account->getOrCreateAddress(data.toObject());
            addresses.append(address);
        }
        if (reset) {
            // just swap rows instead of incremental update
            // this happens after a bump fee for instance
            beginResetModel();
            m_addresses = addresses;
            endResetModel();
        } else if (addresses.size() > 0) {
            // new page of addresses, just append
            beginInsertRows(QModelIndex(), m_addresses.size(), m_addresses.size() + addresses.size() - 1);
            m_addresses.append(addresses);
            endInsertRows();
        }
    });

    connect(handler, &Handler::resolver, this, [](Resolver* resolver) {
        resolver->resolve();
    });

    handler->exec();
    m_handler = handler;
    m_fetching = true;
    emit fetchingChanged();
}

QHash<int, QByteArray> AddressListModel::roleNames() const
{
    return {
        { AddressRole, "address" },
        { PointerRole, "last_pointer" },
        { AddressStringRole, "address_string" },
        { CountRole, "tx_count" }
    };
}

bool AddressListModel::canFetchMore(const QModelIndex& parent) const
{
    Q_ASSERT(!parent.parent().isValid());
    // Prevent concurrent fetchMore
    if (m_handler) return false;
    return m_last_pointer != 1;
}

void AddressListModel::fetchMore(const QModelIndex& parent)
{
    Q_ASSERT(!parent.parent().isValid());
    if (!m_account) return;
    if (m_handler) return;
    fetch(false);
}

int AddressListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_addresses.size();
}

int AddressListModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant AddressListModel::data(const QModelIndex& index, int role) const
{
    switch (role)
    {
        case AddressRole:
            return QVariant::fromValue(m_addresses.at(index.row()));
        case PointerRole:
            return QVariant::fromValue(m_addresses.at(index.row())->data()["last_pointer"].toVariant());
        case AddressStringRole:
            return QVariant::fromValue(m_addresses.at(index.row())->data()["address"].toVariant());
        case CountRole:
            return QVariant::fromValue(m_addresses.at(index.row())->data()["tx_count"].toVariant());
    }

    return QVariant();
}

void AddressListModel::reload()
{
    if (!m_account) return;

    m_handler = nullptr;
    m_has_unconfirmed = false;
    m_last_pointer = 0;

    m_reload_timer->start();
}
