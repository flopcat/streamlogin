#include <QAction>
#include <QCoreApplication>
#include <QLabel>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSessionManager>
#include <QSettings>

#include "accountsdialog.h"
#include "platform.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

static QMap<MainWindow::ProgramState, const char *> progStateMap = {
    { MainWindow::NoState, "No state" },
    { MainWindow::LookingForIp, "Looking up ip address"},
    { MainWindow::Ready, "Ready" },
    { MainWindow::SendingActivation, "Sending Activation" },
    { MainWindow::SendingDeactivation, "Sending Deactivation" },
    { MainWindow::ShuttingDown, "Shutting down" }
};

static const char settingStartup[] = "startup";
static const char settingPoweroff[] = "poweroff";
static const char settingAutostart[] = "autostart";
static const char settingMinimzed[] = "minimized";
static const char settingTray[] = "tray";

//---------------------------------------------------------------------------

void Settings::read()
{
    QSettings s;
    activateStartup = s.value(settingStartup, true).toBool();
    deactivatePoweroff = s.value(settingPoweroff, true).toBool();
    autostart = s.value(settingAutostart, true).toBool();
    minimizedStart = s.value(settingMinimzed, false).toBool();
    minimizedTray = s.value(settingTray, true).toBool();
}

void Settings::write()
{
    QSettings s;
    s.setValue(settingStartup, activateStartup);
    s.setValue(settingPoweroff, deactivatePoweroff);
    s.setValue(settingAutostart, autostart);
    s.setValue(settingMinimzed, minimizedStart);
    s.setValue(settingTray, minimizedTray);
}

//---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(this, &MainWindow::networkInitFinished,
            this, &MainWindow::self_networkInitFinished);

    loadSettings();
    setupUi();
    setupTray();
    setupSessionManager();

    qnam = new QNetworkAccessManager(this);
    auto ipRequest = qnam->get(QNetworkRequest {QUrl("http://ifconfig.co/ip")});
    connect(ipRequest, &QNetworkReply::readyRead,
            this, [this,ipRequest]() { ipRequest_readyRead(ipRequest); });
    connect(ipRequest, &QNetworkReply::finished,
            ipRequest, &QObject::deleteLater);
    setProgState(LookingForIp);

    accountsDialog = new AccountsDialog(nullptr);
}

MainWindow::~MainWindow()
{
    saveSettings();
    Platform::platform()->setProgramAutostart(ui->systemStartup->isChecked() ? Platform::AutomaticStart
                                                                             : Platform::ManualStart);
    if (ui->autoQuit->isChecked())
        deactivateServersNow();
    delete accountsDialog;
    delete ui;
}

void MainWindow::setupUi()
{
    labelStatus = new QLabel(this);
    statusBar()->addPermanentWidget(labelStatus, 1);

    labelMyIp = new QLabel(this);
    labelMyIp->setText("XXX.XXX.XXX.XXX");
    statusBar()->addPermanentWidget(labelMyIp);
}

void MainWindow::setupSessionManager()
{
    /*
    connect(qApp, &QGuiApplication::commitDataRequest,
            this, &MainWindow::sessionManager_shutdown);
    */
}

void MainWindow::setupTray()
{
    QPixmap px;
    px.load(":/icon-logo.svg");
    QIcon icon(px);
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(icon);
    trayIcon->setToolTip(QCoreApplication::applicationName());

    QMenu *menu;
    QAction *action;

    menu = new QMenu(this);

    action = new QAction("Show", this);
    connect(action, &QAction::triggered,
            this, &MainWindow::show);
    menu->addAction(action);

    action = new QAction("Exit", this);
    connect(action, &QAction::triggered,
            qApp, &QCoreApplication::quit);
    menu->addAction(action);

    trayIcon->setContextMenu(menu);

    if (ui->systemTray->isChecked())
        trayIcon->setVisible(true);
}

void MainWindow::loadSettings()
{
    settings.read();
    ui->autoStart->setChecked(settings.activateStartup);
    ui->autoQuit->setChecked(settings.deactivatePoweroff);
    ui->systemStartup->setChecked(settings.autostart);
    ui->startMinimized->setChecked(settings.minimizedStart);
    ui->systemTray->setChecked(settings.minimizedTray);
}

void MainWindow::saveSettings()
{
    settings.activateStartup = ui->autoStart->isChecked();
    settings.deactivatePoweroff = ui->autoQuit->isChecked();
    settings.autostart = ui->systemStartup->isChecked();
    settings.minimizedStart = ui->startMinimized->isChecked();
    settings.minimizedTray = ui->systemTray->isChecked();
    settings.write();
}

MainWindow::WindowStart MainWindow::startState()
{
    if (!ui->startMinimized->isChecked())
        return StartNormally;
    if (ui->systemTray->isChecked())
        return StartHidden;
    return StartHidden;
}

void MainWindow::setMyIpAddress(const QString &ipAddress)
{
    myIpAddress = ipAddress;
    labelMyIp->setText(ipAddress);
}

void MainWindow::closeEvent(QCloseEvent *evt)
{
    if (progState != ShuttingDown) {

    }
    evt->accept();
}

void MainWindow::activateServers()
{
    if (progState != Ready)
        return;
    setProgState(SendingActivation);
    for (auto &a : Account::collection) {
        QString url = "http://" + a.remote.hostname + "/" + a.remote.secret;
        QByteArray json = a.json(Account::ActiveState, myIpAddress);
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply *reply = qnam->post(req, json);
        connect(reply,QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
                this, [this, reply](QNetworkReply::NetworkError) { serverMessage_done(reply); });
        connect(reply, &QNetworkReply::finished,
                this, [this, reply]() { serverMessage_done(reply); });
        pendingRequests.insert(reply);
    }
    if (pendingRequests.isEmpty())
        setProgState(Ready);
}

bool MainWindow::deactivateServers()
{
    if (progState != ShuttingDown) {
        if (progState != Ready)
            return false;
        setProgState(SendingDeactivation);
    }
    for (auto &a : Account::collection) {
        QString url = "http://" + a.remote.hostname + "/" + a.remote.secret;
        QByteArray json = a.json(Account::InactiveState, myIpAddress);
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply *reply = qnam->post(req, json);
        connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
                this, [this, reply](QNetworkReply::NetworkError) { serverMessage_done(reply); });
        connect(reply, &QNetworkReply::finished,
                this, [this, reply]() { serverMessage_done(reply); });
        pendingRequests.insert(reply);
    }
    if (pendingRequests.isEmpty())
        setProgState(Ready);
    return true;
}

void MainWindow::deactivateServersNow()
{
    deactivateServers();
    while (!pendingRequests.isEmpty())
        QCoreApplication::processEvents();
    return;
}

/*
void MainWindow::sessionManager_shutdown(QSessionManager &m)
{
    Q_UNUSED(m)
    if (ui->accountsDeactivate->isChecked()) {
        setProgState(ShuttingDown);
        deactivateServersNow();
        settings.write();
        m.release();
    }
    close();
}
*/

void MainWindow::serverMessage_done(QNetworkReply *msg)
{
    if (!pendingRequests.contains(msg))
        return;
    pendingRequests.remove(msg);
    msg->deleteLater();
    if (pendingRequests.isEmpty() && progState != ShuttingDown)
        setProgState(Ready);
}

void MainWindow::setProgState(MainWindow::ProgramState pState)
{
    progState = pState;
    labelStatus->setText(progStateMap.value(progState));
}

void MainWindow::ipRequest_readyRead(QNetworkReply *reply)
{
    setMyIpAddress(QString::fromLocal8Bit(reply->readLine().trimmed()));
    emit networkInitFinished();
}

void MainWindow::self_networkInitFinished()
{
    setProgState(Ready);
    if (settings.activateStartup)
        activateServers();
}

void MainWindow::on_actionAccountsEdit_triggered()
{
    accountsDialog->reset();
    accountsDialog->exec();
}

void MainWindow::on_accountsActivate_clicked()
{
    activateServers();
}

void MainWindow::on_accountsDeactivate_clicked()
{
    deactivateServers();
}

void MainWindow::on_actionAppExit_triggered()
{
    qApp->quit();
}

void MainWindow::on_systemTray_stateChanged(int arg1)
{
    (void)arg1;
    if (trayIcon)
        trayIcon->setVisible(ui->systemTray->isChecked());
}
