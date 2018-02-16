#ifndef QS_LIBRARY_MANAGER_H
#define QS_LIBRARY_MANAGER_H

#include "qsconnection.h"


class QsLibraryManager
{

public:

    enum QueryResult {
        Ok = 0
    };

    static int configureDbLibrary(int  options,
                                  bool shutdownIfNeeded = false);

    static bool isCompileThreadSafe() noexcept;

    static int
    setDefaultThreadMode(QsConnection::ThreadMode newMode,
                         bool                     shutdownIfNeeded = false);

};

#endif
