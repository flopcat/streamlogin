#include <QJsonDocument>
#include <QJsonObject>
#include <QList>

#include "accountsdialog.h"
#include "ui_accountsdialog.h"

static QMap<QString,Account::LinkType> linkTypeFromString = {
    {"url", Account::LinkUrl},
    {"portpath", Account::LinkPortPath },
    {"path", Account::LinkPath},
    {"none", Account::LinkNone}
};
static QMap<Account::LinkType,QString> linkTypeToString = {
    {Account::LinkUrl, "url" },
    {Account::LinkPortPath, "portpath" },
    {Account::LinkPath, "path" },
    {Account::LinkNone, "none" }
};

static const char tagCollection[] = "collection";
static const char tagRemote[] = "remote";
static const char tagAccount[] = "account";
static const char tagState[] = "state";

static const char fieldVersion[] = "version";
static const char fieldVersionValue[] = "1.0";

static const char fieldType[] = "type";
static const char fieldActive[] = "active";
static const char fieldInactive[] = "inactive";

static const char tagText[] = "text";
static const char tagUrl[] = "url";
static const char tagPort[] = "port";
static const char tagPath[] = "path";
static const char tagIpMine[] = "ipMine";
static const char tagType[] = "type";

static const char fieldHostname[] = "hostname";
static const char fieldSecret[] = "secret";
static const char fieldUsername[] = "username";
static const char fieldPassword[] = "password";


QList<Account> Account::collection;
static Account defaultAccount = {
    Account::Remote {
        "my.remote.server",
        "hackmyserver",
        "myusername",
        "hackme"
    },
    Account::State {
        "mp3 stream",
        "http://my.url/and/path",
        8000,
        "/path/to/stream",
        true,
        Account::LinkPortPath
    },
    Account::State {
        "not streaming",
        "http://my.url/404",
        8000,
        "/404",
        false,
        Account::LinkNone
    }
};


QString Account::displayText()
{
    return remote.username + "@" + remote.hostname;
}

QByteArray Account::json(Account::AccountState as, const QString &clientIpAddress)
{
    State &s = as == ActiveState ? active : inactive;
    QJsonObject obj;
    obj["action"] = "set";
    obj["username"] = remote.username;
    obj["password"] = remote.password;
    obj["text"] = s.text;
    bool setLink = s.type == LinkUrl;
    bool setTld = s.type != LinkNone && s.type != LinkUrl;
    bool setPort = s.type == LinkPortPath;
    bool setPath = s.type == LinkPortPath || s.type == LinkPath;
    if (setLink)
        obj["url"] = s.url;
    if (setTld)
        obj["tld"] = s.ipMine ? clientIpAddress : remote.hostname;
    if (setPort)
        obj["port"] = QString::number(s.port);
    if (setPath)
        obj["path"] = s.path;
    QJsonDocument doc;
    doc.setObject(obj);
    return doc.toJson();
}

//---------------------------------------------------------

AccountsDialog::AccountsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AccountsDialog)
{
    ui->setupUi(this);
    activeSelect();
    inactiveSelect();

    connect(ui->activeLinkUrl, &QCheckBox::clicked,
            this, &AccountsDialog::activeSelect);
    connect(ui->activeLinkPortPath, &QCheckBox::clicked,
            this, &AccountsDialog::activeSelect);
    connect(ui->activeLinkPath, &QCheckBox::clicked,
            this, &AccountsDialog::activeSelect);
    connect(ui->activeLinkNone, &QCheckBox::clicked,
            this, &AccountsDialog::activeSelect);

    connect(ui->inactiveLinkUrl, &QCheckBox::clicked,
            this, &AccountsDialog::inactiveSelect);
    connect(ui->inactiveLinkPortPath, &QCheckBox::clicked,
            this, &AccountsDialog::inactiveSelect);
    connect(ui->inactiveLinkPath, &QCheckBox::clicked,
            this, &AccountsDialog::inactiveSelect);
    connect(ui->inactiveLinkNone, &QCheckBox::clicked,
            this, &AccountsDialog::inactiveSelect);

    connect(ui->remoteHostname, &QLineEdit::textChanged,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->remoteSecret, &QLineEdit::textChanged,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->remoteUsername, &QLineEdit::textChanged,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->remotePassword, &QLineEdit::textChanged,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->activeText, &QLineEdit::textChanged,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->activeUrl, &QLineEdit::textChanged,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->activePort, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->activePath, &QLineEdit::textChanged,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->activeIpMine, &QCheckBox::toggled,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->activeLinkUrl, &QRadioButton::toggled,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->activeLinkPortPath, &QRadioButton::toggled,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->activeLinkPath, &QRadioButton::toggled,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->activeLinkNone, &QRadioButton::toggled,
            this, &AccountsDialog::updateActiveAccount);

    connect(ui->inactiveText, &QLineEdit::textChanged,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->inactiveUrl, &QLineEdit::textChanged,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->inactivePort, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->inactivePath, &QLineEdit::textChanged,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->inactiveIpMine, &QCheckBox::toggled,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->inactiveLinkUrl, &QRadioButton::toggled,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->inactiveLinkPortPath, &QRadioButton::toggled,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->inactiveLinkPath, &QRadioButton::toggled,
            this, &AccountsDialog::updateActiveAccount);
    connect(ui->inactiveLinkNone, &QRadioButton::toggled,
            this, &AccountsDialog::updateActiveAccount);

    connect(ui->remoteHostname, &QLineEdit::textEdited,
            this, &AccountsDialog::updateDisplayText);
    connect(ui->remoteUsername, &QLineEdit::textEdited,
            this, &AccountsDialog::updateDisplayText);
}

AccountsDialog::~AccountsDialog()
{
    delete ui;
}

void AccountsDialog::reset()
{
    accounts = Account::collection;
    activeIndex = -1;
    ui->accountsList->clear();
    for (auto &a : accounts)
        ui->accountsList->addItem(a.displayText());
    ui->accountsList->setCurrentRow(accounts.isEmpty() ? -1 : 0);
    //on_accountsList_currentRowChanged(ui->accountsList->currentRow());
}

void AccountsDialog::activeSelect()
{
    ui->activeUrl->setEnabled(ui->activeLinkUrl->isChecked());
    ui->activePort->setEnabled(ui->activeLinkPortPath->isChecked());
    ui->activePath->setEnabled(ui->activeLinkPortPath->isChecked()
                               || ui->activeLinkPath->isChecked());
}

void AccountsDialog::inactiveSelect()
{
    ui->inactiveUrl->setEnabled(ui->inactiveLinkUrl->isChecked());
    ui->inactivePort->setEnabled(ui->inactiveLinkPortPath->isChecked());
    ui->inactivePath->setEnabled(ui->inactiveLinkPortPath->isChecked()
                                 || ui->inactiveLinkPath->isChecked());
}

void AccountsDialog::updateActiveAccount()
{
    activeAccount = {
        Account::Remote {
            ui->remoteHostname->text(),
            ui->remoteSecret->text(),
            ui->remoteUsername->text(),
            ui->remotePassword->text()
        },
        Account::State {
            ui->activeText->text(),
            ui->activeUrl->text(),
            ui->activePort->value(),
            ui->activePath->text(),
            ui->activeIpMine->isChecked(),
              ui->activeLinkUrl->isChecked() ? Account::LinkUrl
            : ui->activeLinkPortPath->isChecked() ? Account::LinkPortPath
            : ui->activeLinkPath->isChecked() ? Account::LinkPath
            : ui->activeLinkNone->isChecked() ? Account::LinkNone
            : Account::LinkNone
        },
        Account::State {
            ui->inactiveText->text(),
            ui->inactiveUrl->text(),
            ui->inactivePort->value(),
            ui->inactivePath->text(),
            ui->inactiveIpMine->isChecked(),
              ui->inactiveLinkUrl->isChecked() ? Account::LinkUrl
            : ui->inactiveLinkPortPath->isChecked() ? Account::LinkPortPath
            : ui->inactiveLinkPath->isChecked() ? Account::LinkPath
            : ui->inactiveLinkNone->isChecked() ? Account::LinkNone
            : Account::LinkNone
        }
    };
}

void AccountsDialog::updateDisplayText()
{
    activeAccount.remote.hostname = ui->remoteHostname->text();
    activeAccount.remote.username = ui->remoteUsername->text();
    int row = ui->accountsList->currentRow();
    if (row == -1) {
        ui->remoteBox->setTitle("bailed out");
        return;
    }
    ui->accountsList->item(row)->setText(activeAccount.displayText());
}

void AccountsDialog::on_accountsAdd_clicked()
{
    accounts.append(defaultAccount);
    ui->accountsList->addItem(defaultAccount.displayText());
    ui->accountsList->setCurrentRow(ui->accountsList->count()-1);
}

void AccountsDialog::on_accountsDel_clicked()
{
    int index = ui->accountsList->currentRow();
    if (index >= 0) {
        activeIndex = -1;
        accounts.removeAt(index);
        auto i = ui->accountsList->takeItem(index);
        if (i) delete i;
    }
}

void AccountsDialog::on_accountsList_currentRowChanged(int currentRow)
{
    if (activeIndex != -1)
        accounts[activeIndex] = activeAccount;
    activeIndex = currentRow;

    ui->remoteBox->setEnabled(currentRow != -1);
    ui->activeBox->setEnabled(currentRow != -1);
    ui->inactiveBox->setEnabled(currentRow != -1);

    Account a = accounts.value(currentRow);

    ui->remoteHostname->setText(a.remote.hostname);
    ui->remoteSecret->setText(a.remote.secret);
    ui->remoteUsername->setText(a.remote.username);
    ui->remotePassword->setText(a.remote.password);

    ui->activeText->setText(a.active.text);
    ui->activePort->setValue(a.active.port);
    ui->activePath->setText(a.active.path);
    ui->activeIpMine->setChecked(a.active.ipMine);
    ui->activeLinkUrl->setChecked(a.active.type == Account::LinkUrl);
    ui->activeLinkPortPath->setChecked(a.active.type == Account::LinkPortPath);
    ui->activeLinkPath->setChecked(a.active.type == Account::LinkPath);
    ui->activeLinkNone->setChecked(a.active.type == Account::LinkNone);
    activeSelect();

    ui->inactiveText->setText(a.inactive.text);
    ui->inactivePort->setValue(a.inactive.port);
    ui->inactivePath->setText(a.inactive.path);
    ui->inactiveIpMine->setChecked(a.inactive.ipMine);
    ui->inactiveLinkUrl->setChecked(a.inactive.type == Account::LinkUrl);
    ui->inactiveLinkPortPath->setChecked(a.inactive.type == Account::LinkPortPath);
    ui->inactiveLinkPath->setChecked(a.inactive.type == Account::LinkPath);
    ui->inactiveLinkNone->setChecked(a.inactive.type == Account::LinkNone);
    inactiveSelect();
}

void AccountsDialog::on_buttonBox_accepted()
{
    if (activeIndex != -1) {
        accounts[activeIndex] = activeAccount;
        activeIndex = -1;
    }
    Account::collection = accounts;
    accept();
}

void AccountsDialog::on_buttonBox_rejected()
{
    accounts = Account::collection;
    activeIndex = -1;
    reject();
}

//---------------------------------------------------------

bool AccountsXmlWriter::write(QIODevice *stream)
{
    xml.setDevice(stream);
    xml.setAutoFormatting(true);

    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE StreamLoginAccounts>");
    writeCollection(Account::collection);
    xml.writeEndDocument();

    return true;
}

void AccountsXmlWriter::writeCollection(const QList<Account> &collection)
{
    xml.writeStartElement(tagCollection);
    xml.writeAttribute(fieldVersion, fieldVersionValue);
    for (auto &a : collection) {
        writeAccount(a);
    }
    xml.writeEndElement();
}

void AccountsXmlWriter::writeAccount(const Account &account)
{
    xml.writeStartElement(tagAccount);
    writeRemote(account.remote);
    writeState(account.active, fieldActive);
    writeState(account.inactive, fieldInactive);
    xml.writeEndElement();
}

void AccountsXmlWriter::writeRemote(const Account::Remote &remote)
{
    xml.writeStartElement(tagRemote);
    xml.writeTextElement(fieldHostname, remote.hostname);
    xml.writeTextElement(fieldSecret, remote.secret);
    xml.writeTextElement(fieldUsername, remote.username);
    xml.writeTextElement(fieldPassword, remote.password);
    xml.writeEndElement();
}

void AccountsXmlWriter::writeState(const Account::State &state, const QString &type)
{
    xml.writeStartElement(tagState);
    xml.writeAttribute(fieldType, type);
    xml.writeTextElement(tagText, state.text);
    xml.writeTextElement(tagUrl, state.url);
    xml.writeTextElement(tagPort, QString::number(state.port));
    xml.writeTextElement(tagPath, state.path);
    xml.writeTextElement(tagIpMine, state.ipMine ? "yes" : "no");
    xml.writeTextElement(tagType, linkTypeToString[state.type]);
    xml.writeEndElement();
}

//---------------------------------------------------------

bool AccountsXmlReader::read(QIODevice *stream)
{
    xml.setDevice(stream);

    if (xml.readNextStartElement()) {
        if (xml.name() == tagCollection
                && xml.attributes().value(fieldVersion) == fieldVersionValue) {
            readCollection(Account::collection);
        } else {
            xml.raiseError("The file format does not match");
        }
    }
    return xml.error();
}

void AccountsXmlReader::readCollection(QList<Account> &collection)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == tagCollection);
    collection.clear();

    while (xml.readNextStartElement()) {
        if (xml.name() == tagAccount) {
            Account a;
            readAccount(a);
            collection.append(a);
        } else {
            xml.skipCurrentElement();
        }
    }
}

void AccountsXmlReader::readAccount(Account &a)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == tagAccount);

    while (xml.readNextStartElement()) {
        if (xml.name() == tagRemote)
            readRemote(a.remote);
        else if (xml.name() == tagState) {
            QStringRef type = xml.attributes().value(fieldType);
            if (type == fieldActive)
                readState(a.active);
            else if (type == fieldInactive)
                readState(a.inactive);
            else
                xml.skipCurrentElement();
        } else {
            xml.skipCurrentElement();
        }
    }
}

void AccountsXmlReader::readRemote(Account::Remote &r)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == tagRemote);

    while (xml.readNextStartElement()) {
        if (xml.name() == fieldHostname)
            r.hostname = xml.readElementText();
        else if (xml.name() == fieldSecret)
            r.secret = xml.readElementText();
        else if (xml.name() == fieldUsername)
            r.username = xml.readElementText();
        else if (xml.name() == fieldPassword)
            r.password = xml.readElementText();
    }
}

void AccountsXmlReader::readState(Account::State &s)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == tagState);

    while (xml.readNextStartElement()) {
        QStringRef name = xml.name();
        if (name == tagText)
            s.text = xml.readElementText();
        else if (name == tagUrl)
            s.url = xml.readElementText();
        else if (name == tagPort)
            s.port = xml.readElementText().toInt();
        else if (name == tagPath)
            s.path = xml.readElementText();
        else if (name == tagIpMine)
            s.ipMine = xml.readElementText() == "yes";
        else if (name == tagType)
            s.type = linkTypeFromString.value(xml.readElementText(), Account::LinkNone);
    }
}

//---------------------------------------------------------

Account::CollectionStore::CollectionStore()
{
    AccountsXmlReader xReader;
    QFile f(xmlFile());
    if (f.open(QFile::ReadOnly | QFile::Text))
        xReader.read(&f);
}

Account::CollectionStore::~CollectionStore()
{
    AccountsXmlWriter xWriter;
    QFile f(xmlFile());
    if (f.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))
        xWriter.write(&f);
}

QString Account::CollectionStore::xmlFile()
{
    return qApp->applicationDirPath() + "/accounts.xml";
}
