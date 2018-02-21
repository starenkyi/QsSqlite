#ifndef QS_HELPER_H
#define QS_HELPER_H

#include <functional>
#include <memory>
#include <utility>

#include <QByteArray>
#include <QVariant>

#include "../include/qsconnection.h"


namespace qs {

using ExecResult = std::pair<QVariant,QByteArray>;
using OnSuccess  = std::function<void (QVariant   result)>;
using OnError    = std::function<void (QByteArray errorMessage)>;
using Handler    = std::pair<OnSuccess, OnError>;
using HandlerPtr = std::shared_ptr<Handler>;

const QByteArray badAllocErrMsg =
        QByteArrayLiteral("Bad allocation exception!");

const QByteArray unknownExceptionErrMsg =
        QByteArrayLiteral("Unknown exception!");

QByteArray processExexResult(const ExecResult* resultPtr,
                             const HandlerPtr& handlerPtr) Q_DECL_NOTHROW;

QByteArray buildConnErrMsg(const char*         message,
                           const QsConnection& connection);

}

#endif
