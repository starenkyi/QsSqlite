#include "../include/qsconnectionworker.h"

#include <QMetaType>

#include "qshelper.h"

namespace {

static const int taskId = qRegisterMetaType<QsConnectionWorker::TaskPtr>();

static const int onSuccessId =
        qRegisterMetaType<QsConnectionWorker::StmtTaskPtr>();

static const int onErrroId =
        qRegisterMetaType<QsConnectionWorker::ExecResultPtr>();

static const int handlerId =
        qRegisterMetaType<QsConnectionWorker::HandlerPtr>();

static const QByteArray emptyTaskErr =
        QByteArrayLiteral("Error: task is empty.");

const char* rollbackErr = "Error on rollback";

}


QsConnectionWorker::QsConnectionWorker(const QsConnectionConfig& config,
                                       QObject*                  parent)
    : QObject(parent),
      _connectionConfig {config}
{}

QsConnectionWorker::QsConnectionWorker(QsConnectionConfig&& config,
                                       QObject*             parent)
    : QObject(parent),
      _connectionConfig {std::move(config)}
{}

QsConnectionWorker::ExecResult
QsConnectionWorker::exec(const Task& task) Q_DECL_NOTHROW
{
    ExecResult result;

    tryRunTask(task, result);

    return result;
}

QsConnectionWorker::ExecResult
QsConnectionWorker::exec(const StmtTask&   task,
                         const QByteArray& query,
                         const bool        inTransaction) Q_DECL_NOTHROW
{
    ExecResult result;

    tryRunStmtTask(task, query, result, inTransaction);

    return result;
}

bool QsConnectionWorker::openConnection()
{
    return _connection.isOpen() || _connectionConfig.openAndConfig(_connection)
            == QsConnectionConfig::ResultCode::Ok;
}



void QsConnectionWorker::execWithData(TaskPtr  taskPtr,
                                      QVariant data) Q_DECL_NOTHROW
{
    // check if pointer to task is not null
    if (taskPtr) {
        // execute task
        ExecResult result = exec(*taskPtr.get());

        // process result
        processExecResultWithData(result, data);
    } else {
        emit errorWithData(emptyTaskErr, std::move(data));
    }
}

void QsConnectionWorker::execWithHandler(TaskPtr    taskPtr,
                                         HandlerPtr handlerPtr,
                                         const bool runHandler) Q_DECL_NOTHROW
{
    // check if pointer to task is not null
    if (taskPtr) {
        // execute task
        ExecResult result = exec(*taskPtr.get());

        // process result
        processExecResultWithHandler(result, handlerPtr, runHandler);
    } else {
        emit error(emptyTaskErr);
    }
}

void QsConnectionWorker::execStatementWithData(StmtTaskPtr stmtPtr,
                                               QByteArray  query,
                                               bool        inTransaction,
                                               QVariant    data) Q_DECL_NOTHROW
{
    // check if pointer to statement task is not null
    if (stmtPtr) {
        // execute task
        ExecResult result = exec(*stmtPtr.get(), query, inTransaction);

        // process result
        processExecResultWithData(result, data);
    } else {
        emit errorWithData(emptyTaskErr, std::move(data));
    }
}

void QsConnectionWorker::execStatementWithHandler(
        StmtTaskPtr stmtPtr,
        QByteArray  query,
        bool        inTransaction,
        HandlerPtr  handlerPtr,
        bool        runHandler) Q_DECL_NOTHROW
{
    // check if pointer to task is not null
    if (stmtPtr) {
        // execute task
        ExecResult result = exec(*stmtPtr.get(), query, inTransaction);

        // process result
        processExecResultWithHandler(result, handlerPtr, runHandler);
    } else {
        emit error(emptyTaskErr);
    }
}

void QsConnectionWorker::processExecResultWithHandler(
        ExecResult& result,
        HandlerPtr& handlerPtr,
        const bool  runHandler) Q_DECL_NOTHROW
{
    // check if need run handler (otherwise resend it)
    if (runHandler) {
        qs::processExexResult(&result, handlerPtr);
    } else {
        // create std::shared_ptr from ExecResult (and catch exceptions)
        ExecResultPtr resultPtr;
        QByteArray onThrowMsg;
        try {
            resultPtr = std::make_shared<QsConnectionWorker::ExecResult>
                    (std::move(result.first), std::move(result.second));
        } catch (const std::exception& exception) {
            try {
                onThrowMsg = exception.what();
            } catch (...) {
                onThrowMsg = qs::badAllocErrMsg;
            }
        } catch (...) {
            onThrowMsg = qs::unknownExceptionErrMsg;
        }

        // check if no error message while creating ExecResultPtr
        if (onThrowMsg.isEmpty()) {
            // resend result and handler smart pointers
            emit executed(std::move(resultPtr), std::move(handlerPtr));
        } else {
            // emit signal with error message
            emit error(std::move(onThrowMsg));
        }
    }
}

void
QsConnectionWorker::processExecResultWithData(ExecResult& result,
                                              QVariant&   data) Q_DECL_NOTHROW
{
    // check result and emit needed signal
    if (result.second.isEmpty()) {
        emit finished(std::move(result.first), std::move(data));
    } else {
        emit errorWithData(std::move(result.second), std::move(data));
    }
}

void QsConnectionWorker::tryRunStmtTask(
        const StmtTask&    stmtTask,
        const QByteArray&  query,
        ExecResult&        result,
        bool               inTransaction) Q_DECL_NOTHROW
{
    // check if task is not empty
    if (!stmtTask) {
        result.second = emptyTaskErr;
        return;
    }

    try {
        // check if connection is open (and try open it, if it is closed)
        if (!openConnection()) {
            result.second = _connectionConfig.lastError();
            return;
        }

        // try begin transaction, if needed (or save error and return)
        if (inTransaction && !_connection.transaction()) {
            result.second = qs::buildConnErrMsg(
                        "Error on begin transaction", _connection);
            return;
        }

        // try compile statement
        QsStatement statement(_connection, query);
        if (!statement.isValid()) {
            // save error
            result.second = qs::buildConnErrMsg("Error on compile statement",
                                                _connection);

            // rollback transaction, if started, and save error, if occurred
            if (inTransaction && !_connection.rollback()) {
                result.second.append(' ')
                        .append(qs::buildConnErrMsg(rollbackErr, _connection));
            }
            return;
        }

        // try run statement task
        bool commitChanges = true;
        result.first = stmtTask(std::move(statement), commitChanges);

        // check if need commit (or rollback) try do it
        if (inTransaction) {
            if (commitChanges) {
                if (!_connection.commit()) {
                    result.second = qs::buildConnErrMsg("Error on commit",
                                                        _connection);
                }
            } else if (!_connection.rollback()) {
                 result.second = qs::buildConnErrMsg(rollbackErr, _connection);
            }
        }
    } catch (const std::exception& exception) {
        try {
            result.second = exception.what();
        } catch (...) {
            result.second = qs::badAllocErrMsg;
        }
    } catch (...) {
        result.second = qs::unknownExceptionErrMsg;
    }
}

void QsConnectionWorker::tryRunTask(const Task& task,
                                    ExecResult& result) Q_DECL_NOTHROW
{
    // check if task is not empty
    if (!task) {
        result.second = emptyTaskErr;
        return;
    }

    try {
        // check if connection is open (and try open it, if it is closed)
        if (openConnection()) {
            result.first = task(_connection);
        } else {
            result.second = _connectionConfig.lastError();
        }
    } catch (const std::exception& exception) {
        try {
            result.second = exception.what();
        } catch (...) {
            result.second = qs::badAllocErrMsg;
        }
    } catch (...) {
        result.second = qs::unknownExceptionErrMsg;
    }
}
