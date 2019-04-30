#include "mainwindow.h"
#include "accountsdialog.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Flopcat");
    QCoreApplication::setOrganizationDomain("desktopclient.streamlogin.flopcat");
    QCoreApplication::setApplicationName("StreamLogin Desktop Client");
    QCoreApplication::setApplicationVersion(VERSION_STRING);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(a);

    a.setWindowIcon(QIcon(":/icon-logo.svg"));
    a.setQuitOnLastWindowClosed(false);

#ifdef Q_OS_LINUX
    a.setStyle(QStyleFactory::create("Fusion"));
#endif

    Account::CollectionStore accountStore;
    MainWindow w;
    switch(w.startState()) {
    case MainWindow::StartNormally:
        w.show();
        break;
    case MainWindow::StartMinimized:
        w.showMinimized();
        break;
    case MainWindow::StartHidden:
        break;
    }

    int exitCode = a.exec();

    return exitCode;
}
