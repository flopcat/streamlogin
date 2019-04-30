#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "platform_linux.h"

LinuxPlatform::LinuxPlatform(QObject *owner)
    : Platform(owner)
{

}

void LinuxPlatform::setProgramAutostart(ProgramAutostart method)
{
    QString appPath = QCoreApplication::applicationFilePath();

    QString autostartPath = QDir::homePath() + "/.config/autostart";
    QDir().mkpath(autostartPath);
    QFile f(autostartPath + "/desktopclient.desktop");
    if (method != AutomaticStart) {
        f.remove();
        return;
    }

    QString data = R"EOF([Desktop Entry]
Exec=%1
Icon=system-run
Name=%2
X-GNOME-Autostart-enabled=true
)EOF";

    if (!f.open(QFile::WriteOnly | QFile::Truncate)) {
        throw std::runtime_error("Could not write to autostart folder");
    }
    f.write(data.arg(appPath, QCoreApplication::applicationName()).toUtf8());
}
