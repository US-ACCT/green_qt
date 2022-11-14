#ifndef GREEN_ACCOUNT_H
#define GREEN_ACCOUNT_H

#include <QObject>
#include <QtQml>

class Address;
class Balance;
class Output;
class Transaction;
class Wallet;

class Account : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Wallet* wallet READ wallet CONSTANT)
    Q_PROPERTY(int pointer READ pointer CONSTANT)
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(bool mainAccount READ isMainAccount CONSTANT)
    Q_PROPERTY(QJsonObject json READ json NOTIFY jsonChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(bool hidden READ isHidden NOTIFY hiddenChanged)
    Q_PROPERTY(qint64 balance READ balance NOTIFY balanceChanged)
    Q_PROPERTY(QQmlListProperty<Balance> balances READ balances NOTIFY balancesChanged)
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    QML_ELEMENT
public:
    explicit Account(const QJsonObject& data, Wallet* wallet);

    Wallet* wallet() const { return m_wallet; }
    quint32 pointer() const { return m_pointer; }
    QString type() const { return m_type; }
    bool isMainAccount() const;

    QString name() const { return m_name; }
    void setName(const QString& name);
    bool isHidden() const { return m_hidden; }
    QJsonObject json() const;

    void update(const QJsonObject& json);

    void handleNotification(const QJsonObject& notification);

    qint64 balance() const;

    QQmlListProperty<Balance> balances();

    bool hasBalance() const;
    void updateBalance();
    Transaction *getOrCreateTransaction(const QJsonObject &data);
    Output *getOrCreateOutput(const QJsonObject &data);
    Address *getOrCreateAddress(const QJsonObject &data);
    Q_INVOKABLE Balance* getBalanceByAssetId(const QString &id) const;
    Q_INVOKABLE Transaction* getTransactionByTxHash(const QString &id) const;
    bool isReady() const { return m_ready; }
signals:
    void walletChanged();
    void jsonChanged();
    void nameChanged(const QString& name);
    void hiddenChanged(bool hidden);
    void balanceChanged();
    void balancesChanged();
    void notificationHandled(const QJsonObject& notification);
    void addressGenerated();
    void readyChanged();
public slots:
    void reload();
    bool rename(QString name, bool active_focus);
    void show();
    void hide();
private:
    void setHiddenAsync(bool hidden);
    void setHidden(bool hidden);
private:
    Wallet* const m_wallet;
    const quint32 m_pointer;
    const QString m_type;
    QJsonObject m_json;
    QString m_name;
    bool m_hidden{false};
    QMap<QString, Transaction*> m_transactions_by_hash;
    QMap<QPair<QString, int>, Output*> m_outputs_by_hash;
    QMap<QString, Address*> m_address_by_hash;
    QList<Balance*> m_balances;
    QMap<QString, Balance*> m_balance_by_id;
    bool m_ready{false};
    friend class Wallet;
};

#endif // GREEN_ACCOUNT_H
