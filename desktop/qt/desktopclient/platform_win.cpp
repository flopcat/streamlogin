#include <QCoreApplication>
#include <QSettings>
#include "platform_win.h"

WinPlatform::WinPlatform(QObject *owner)
    : Platform(owner)
{

}

void WinPlatform::setProgramAutostart(Platform::ProgramAutostart method)
{
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                        QSettings::NativeFormat);
    if (method != AutomaticStart) {
        settings.remove(QCoreApplication::applicationName());
        return;
    }
    QString format("\"%1\"");
    settings.setValue(QCoreApplication::applicationName(),
                      format.arg(QCoreApplication::applicationFilePath()).replace('/','\\'));
}
