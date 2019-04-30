#ifndef PLATFORM_H
#define PLATFORM_H

#include <QObject>

class Platform : public QObject {
    Q_OBJECT
public:
    Platform(QObject *owner = nullptr);

    enum ProgramAutostart { ManualStart, AutomaticStart };
    virtual void setProgramAutostart(ProgramAutostart method) = 0;

    static Platform *platform();
};

#endif // PLATFORM_H
