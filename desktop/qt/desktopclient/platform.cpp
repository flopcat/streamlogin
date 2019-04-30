#include <QtGlobal>
#include <QCoreApplication>
#include "platform.h"

#ifdef Q_OS_LINUX
#include "platform_linux.h"
typedef LinuxPlatform ThePlatform;
#endif
#ifdef Q_OS_WIN
#include "platform_win.h"
typedef WinPlatform ThePlatform;
#endif


Platform::Platform(QObject *owner)
    : QObject(owner)
{

}

Platform *Platform::platform()
{
    static ThePlatform *plugin = nullptr;
    if (!plugin) {
        plugin = new ThePlatform(qApp);
    }
    return plugin;
}
