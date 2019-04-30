#ifndef ACCOUNTSDIALOG_H
#define ACCOUNTSDIALOG_H

#include <QDialog>
#include <QList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace Ui {
class AccountsDialog;
}

class Account {
public:
    enum LinkType { LinkUrl, LinkPortPath, LinkPath, LinkNone };
    enum AccountState { ActiveState, InactiveState };
    class State {
    public:
        QString text;
        QString url;
        int     port;
        QString path;
        bool    ipMine;
        LinkType type;
    };
    class Remote {
    public:
        QString hostname;
        QString secret;
        QString username;
        QString password;
    };

    Remote remote;
    State active;
    State inactive;
    static QList<Account> collection;

    QString displayText();
    QByteArray json(AccountState as, const QString &clientIpAddress);

    class CollectionStore;
};

class AccountsXmlWriter
{
public:
    bool write(QIODevice *stream);

    void writeCollection(const QList<Account> &collection);
    void writeAccount(const Account &account);
    void writeRemote(const Account::Remote &remote);
    void writeState(const Account::State &state, const QString &type);

private:
    QXmlStreamWriter xml;
};

class AccountsXmlReader
{
public:
    bool read(QIODevice *stream);

    void readCollection(QList<Account> &collection);
    void readAccount(Account &a);
    void readRemote(Account::Remote &r);
    void readState(Account::State &s);
private:
    QXmlStreamReader xml;
};

class Account::CollectionStore {
public:
    CollectionStore();
    ~CollectionStore();
    QString xmlFile();
};


class AccountsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccountsDialog(QWidget *parent = nullptr);
    ~AccountsDialog();

    void reset();

protected slots:
    void activeSelect();
    void inactiveSelect();

    void updateActiveAccount();
    void updateDisplayText();

private slots:
    void on_accountsAdd_clicked();
    void on_accountsDel_clicked();
    void on_accountsList_currentRowChanged(int currentRow);

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::AccountsDialog *ui;
    QList<Account> accounts;
    int activeIndex = -1;
    Account activeAccount;
};

#endif // ACCOUNTSDIALOG_H
