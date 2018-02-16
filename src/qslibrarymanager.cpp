#include "../include/qslibrarymanager.h"

#include <QMutex>
#include <QMutexLocker>

namespace {

// mutex for simultaneous configuration library thread mode
QMutex _mutex;

// function return sqlite flags for QConnection::ThreadMode
int configOptionFor(const QsConnection::ThreadMode value) noexcept
{
    switch (value) {
    case QsConnection::ThreadMode::MultiThread:
        return SQLITE_CONFIG_MULTITHREAD;
    case QsConnection::ThreadMode::Serialized:
        return SQLITE_CONFIG_SERIALIZED;
    default:
        return SQLITE_CONFIG_SINGLETHREAD;
    }
}

}

// function try configure sqlite library and return result code
int configureDbLibrary(const int option,
                       const bool shutdownIfNeeded)
{
    // lock mutex for write
    QMutexLocker lock {&_mutex};

    // try configure sqlite3 library
    int resultCode = sqlite3_config(option);

    // if sqlite3 library is initialized and shutdownIfNeed is true,
    // try shutdown and configure it
    if (resultCode == SQLITE_MISUSE
            && shutdownIfNeeded
            && (resultCode = sqlite3_shutdown()) == SQLITE_OK) {
        resultCode = sqlite3_config(option);
    }

    // return result
    return resultCode;
}

bool QsLibraryManager::isCompileThreadSafe() noexcept
{
    return sqlite3_threadsafe();
}

int
QsLibraryManager::setDefaultThreadMode(const QsConnection::ThreadMode newMode,
                                       const bool shutdownIfNeeded)
{
    // check if newMode is not ThreadMode::Default, otherwise return SQLITE_OK
    return (newMode != QsConnection::ThreadMode::Default)
            ? configureDbLibrary(configOptionFor(newMode), shutdownIfNeeded)
            : SQLITE_OK;
}
