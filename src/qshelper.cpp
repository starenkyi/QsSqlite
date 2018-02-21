#include "qshelper.h"

QByteArray qs::processExexResult(
        const qs::ExecResult* resultPtr,
        const qs::HandlerPtr& handlerPtr) Q_DECL_NOTHROW
{
    QByteArray resultStr;  // for result

    // check if pointers to execution result and handlers are not null
    if (resultPtr) {
        if (handlerPtr) {
            try {
                // check if error message is null,
                // otherwise check if error handler is not null (run it if true,
                // otherwise save error message into result string)
                if (resultPtr->second.isEmpty()) {
                    // check if onSuccess is not empty (and run it, if true)
                    if (handlerPtr->first) {
                        handlerPtr->first(resultPtr->first);
                    }
                } else if (handlerPtr->second) {
                    handlerPtr->second(resultPtr->second);
                } else {
                    resultStr = resultPtr->second;
                }
            } catch (const std::exception& exception) {
                // try put exception message into result
                try {
                    resultStr = exception.what();
                } catch (...) {
                    resultStr = badAllocErrMsg;
                }
            } catch (...) {
                resultStr = unknownExceptionErrMsg;
            }
        } else {
            resultStr = QByteArrayLiteral("Error: result handlers not exists.");
        }
    } else {
        resultStr = QByteArrayLiteral("Error: execution result not exist.");
    }

    return resultStr;
}



QByteArray qs::buildConnErrMsg(const char*         message,
                               const QsConnection& connection)
{
    QByteArray result(message);
    const QByteArray& error = connection.lastError();

    // if error is not empty, append it to error message
    if (!error.isEmpty()) {
        result.append(" (").append(error).append(')');
    }
    result.append('.');

    return result;
}
