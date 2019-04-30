#ifndef PLATFORM_WIN_H
#define PLATFORM_WIN_H

#include "platform.h"

class WinPlatform : public Platform
{
    Q_OBJECT
public:
    WinPlatform(QObject *owner);
    void setProgramAutostart(ProgramAutostart method);
};


#endif // PLATFORM_LINUX_H
