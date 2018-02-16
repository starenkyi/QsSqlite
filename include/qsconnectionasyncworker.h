#ifndef QS_CONNECTION_ASYNC_WORKER_H
#define QS_CONNECTION_ASYNC_WORKER_H

#include <functional>
#include <utility>

#include <QAtomicPointer>
#include <QByteArray>
#include <QMetaObject>
#include <QMutex>
#include <QObject>
#include <QVariant>
#include <QVector>

#include "qsconnection.h"
#include "qsconnectionconfig.h"
#include "qsconnectionworker.h"

class QsWorkerThread;


class QsConnectionAsyncWorker : public QObject
{
    Q_OBJECT

public:

    using Task    = QsConnectionWorker::Task;
    using TaskPtr = QsConnectionWorker::TaskPtr;

    using StmtTask    = QsConnectionWorker::StmtTask;
    using StmtTaskPtr = QsConnectionWorker::StmtTaskPtr;

    using OnSuccess  = QsConnectionWorker::OnSuccess;
    using OnError    = QsConnectionWorker::OnError;
    using Handler    = QsConnectionWorker::Handler;
    using HandlerPtr = QsConnectionWorker::HandlerPtr;

    QsConnectionAsyncWorker(const QsConnectionConfig& config,
                            QObject*                  parent = nullptr);

    QsConnectionAsyncWorker(QsConnectionConfig&& config,
                            QObject*             parent = nullptr);

    virtual ~QsConnectionAsyncWorker();

    std::pair<bool, QByteArray>
    execute(Task      task,
            OnSuccess onSuccess,
            OnError   onError              = OnError(),
            bool      handleInWorkerThread = false) Q_DECL_NOTHROW;

    std::pair<bool, QByteArray>
    execute(TaskPtr    taskPtr,
            HandlerPtr handlerPtr,
            bool       handleInWorkerThread = false) Q_DECL_NOTHROW;

    std::pair<bool, QByteArray>
    execute(Task     task,
            QVariant data = QVariant()) Q_DECL_NOTHROW;

    std::pair<bool, QByteArray>
    execute(TaskPtr  taskPtr,
            QVariant data = QVariant()) Q_DECL_NOTHROW;

    std::pair<bool, QByteArray>
    execute(StmtTask   task,
            QByteArray query,
            OnSuccess  onSuccess,
            OnError    onError              = OnError(),
            bool       inTransaction        = true,
            bool       handleInWorkerThread = false) Q_DECL_NOTHROW;

    std::pair<bool, QByteArray>
    execute(StmtTaskPtr taskPtr,
            QByteArray  query,
            HandlerPtr  handlerPtr,
            bool        inTransaction        = true,
            bool        handleInWorkerThread = false) Q_DECL_NOTHROW;

    std::pair<bool, QByteArray>
    execute(StmtTask   task,
            QByteArray query,
            bool       inTransaction = true,
            QVariant   data          = QVariant()) Q_DECL_NOTHROW;

    std::pair<bool, QByteArray>
    execute(StmtTaskPtr taskPtr,
            QByteArray  query,
            bool        inTransaction = true,
            QVariant    data          = QVariant()) Q_DECL_NOTHROW;

    std::pair<bool, QByteArray>
    stop(unsigned long waitMilliseconds = 0) Q_DECL_NOTHROW;

    std::pair<bool, QByteArray> stopAndWait() Q_DECL_NOTHROW;

    QsConnectionAsyncWorker() = delete;
    QsConnectionAsyncWorker(const QsConnectionAsyncWorker&) = delete;
    QsConnectionAsyncWorker(QsConnectionAsyncWorker&&) = delete;
    QsConnectionAsyncWorker& operator=(const QsConnectionAsyncWorker&) = delete;
    QsConnectionAsyncWorker& operator=(QsConnectionAsyncWorker&&) = delete;

signals:

    void error(QByteArray errorMessage);

    void errorWithData(QByteArray errorMessage,
                       QVariant   helperData);

    void finished(QVariant result,
                  QVariant helperData);

    void execStmtWithDataSignal(StmtTaskPtr taskPtr,
                                QByteArray  query,
                                bool        inTransaction,
                                QVariant    data,
                                QPrivateSignal);

    void execStmtWithHandlerSignal(StmtTaskPtr taskPtr,
                                   QByteArray  query,
                                   bool        inTransaction,
                                   HandlerPtr  handlerPtr,
                                   bool        runHandler,
                                   QPrivateSignal);

    void execWithDataSignal(TaskPtr  taskPtr,
                            QVariant data,
                            QPrivateSignal);

    void execWithHandlerSignal(TaskPtr    taskPtr,
                               HandlerPtr handlerPtr,
                               bool       runHandler,
                               QPrivateSignal);

private slots:

    void onExecuted(
            QsConnectionWorker::ExecResultPtr resultPtr,
            HandlerPtr                        handlerPtr) Q_DECL_NOTHROW;

    void onThreadFinished() Q_DECL_NOTHROW;

private:

    QAtomicPointer<QsWorkerThread>   _activeThread;
    QMutex                           _mutex;
    QsConnectionConfig               _connectionConfig;
    QVector<QMetaObject::Connection> _workerObjConnections;

    QByteArray checkWorkerThread() Q_DECL_NOTHROW;

    void connectTo(QsConnectionWorker* worker);

    void createWorkerThread();

    std::pair<bool, QByteArray>
    disconnectWorkerObject(bool          quitThread,
                           unsigned long waitMilliseconds) Q_DECL_NOTHROW;

};

// helper function for create pointer to Task, StmtTask and Handler

inline QsConnectionAsyncWorker::TaskPtr
createTask(QsConnectionAsyncWorker::Task&& task)
{
    return std::make_shared<QsConnectionWorker::Task>(std::move(task));
}

inline QsConnectionAsyncWorker::StmtTaskPtr
createStmtTask(QsConnectionWorker::StmtTask&& task)
{
    return std::make_shared<QsConnectionWorker::StmtTask>(std::move(task));
}

inline QsConnectionAsyncWorker::HandlerPtr
createHandler(QsConnectionAsyncWorker::OnSuccess&& onSuccess,
              QsConnectionAsyncWorker::OnError&&   onError)
{
    return std::make_shared<QsConnectionWorker::Handler>
            (std::move(onSuccess), std::move(onError));
}

#endif
