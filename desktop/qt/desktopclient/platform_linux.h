#ifndef PLATFORM_LINUX_H
#define PLATFORM_LINUX_H

#include "platform.h"

class LinuxPlatform : public Platform
{
    Q_OBJECT
public:
    LinuxPlatform(QObject *owner);
    void setProgramAutostart(ProgramAutostart method);
};

#endif // PLATFORM_LINUX_H
