#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QSet>
#include <QMainWindow>
#include <QSessionManager>
#include <QSystemTrayIcon>

class AccountsDialog;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;


namespace Ui {
class MainWindow;
}

class Settings {
public:
    Settings() = default;
    bool activateStartup;
    bool deactivatePoweroff;
    bool autostart;
    bool minimizedStart;
    bool minimizedTray;

    void read();
    void write();
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    enum ProgramState { NoState, LookingForIp, Ready, SendingActivation, SendingDeactivation, ShuttingDown };
    enum WindowStart { StartNormally, StartMinimized, StartHidden };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setupUi();
    void setupSessionManager();
    void setupTray();

    void loadSettings();
    void saveSettings();
    WindowStart startState();

    void setMyIpAddress(const QString &ipAddress);

signals:
    void networkInitFinished();

protected:
    void closeEvent(QCloseEvent *evt);

public slots:
    void activateServers();
    bool deactivateServers();
    void deactivateServersNow();

    //void sessionManager_shutdown(QSessionManager &m);
    void serverMessage_done(QNetworkReply *msg);

private slots:
    void setProgState(ProgramState pState);
    void ipRequest_readyRead(QNetworkReply *reply);
    void self_networkInitFinished();
    void on_actionAccountsEdit_triggered();
    void on_accountsActivate_clicked();
    void on_accountsDeactivate_clicked();
    void on_actionAppExit_triggered();
    void on_systemTray_stateChanged(int arg1);

    void on_actionHelpAboutThisProgram_triggered();

    void on_actionHelpAboutQt_triggered();

private:
    ProgramState progState = NoState;
    Ui::MainWindow *ui = nullptr;
    AccountsDialog *accountsDialog = nullptr;
    QLabel *labelStatus = nullptr;
    QLabel *labelMyIp = nullptr;

    QSystemTrayIcon *trayIcon = nullptr;
    QNetworkAccessManager *qnam = nullptr;

    Settings settings;
    QString myIpAddress;
    QSet<QNetworkReply*> pendingRequests;
};

#endif // MAINWINDOW_H
