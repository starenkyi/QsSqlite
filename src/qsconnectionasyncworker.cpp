#include "../include/qsconnectionasyncworker.h"

#include <QMutexLocker>
#include <QThread>

#include "qshelper.h"

using OperationResult = std::pair<bool, QByteArray>;


namespace {

template<typename T>
QByteArray createTaskPtr(std::shared_ptr<T>& taskPtr,
                         T&                  task) Q_DECL_NOTHROW
{
    QByteArray result;

    try {
        taskPtr = std::make_shared<T>(std::move(task));
    } catch (const std::exception& exception) {
        try {
            result = exception.what();
        } catch (...) {
            result = qs::badAllocErrMsg;
        }
    } catch (...) {
        result = qs::unknownExceptionErrMsg;
    }

    return result;
}

QByteArray
createHandlerPtr(QsConnectionAsyncWorker::HandlerPtr& handlerPtr,
                 QsConnectionAsyncWorker::OnSuccess&  onSuccess,
                 QsConnectionAsyncWorker::OnError&    onError) Q_DECL_NOTHROW
{
    QByteArray result;

    try {
        handlerPtr = createHandler(std::move(onSuccess), std::move(onError));
    } catch (const std::exception& exception) {
        try {
            result = exception.what();
        } catch (...) {
            result = qs::badAllocErrMsg;
        }
    } catch (...) {
        result = qs::unknownExceptionErrMsg;
    }

    return result;
}

}


class QsWorkerThread : public QThread
{
    Q_OBJECT

public:

    explicit QsWorkerThread()
        : QThread()
    {
        // connect to delete thread after finish running
        connect(this, &QsWorkerThread::finished,
                this, &QsWorkerThread::deleteLater);
    }

    void run() override
    {
        // disable termination
        setTerminationEnabled(false);

        // run event loop
        exec();
    }

    virtual ~QsWorkerThread() = default;

    QsWorkerThread(const QsWorkerThread&) = delete;
    QsWorkerThread(QsWorkerThread&&) = delete;
    QsWorkerThread& operator =(const QsWorkerThread&) = delete;
    QsWorkerThread& operator =(QsWorkerThread&&) = delete;

};


QsConnectionAsyncWorker::QsConnectionAsyncWorker(
        const QsConnectionConfig& config,
        QObject*                  parent)
    : QObject(parent),
      _activeThread {nullptr},
      _connectionConfig(config)
{}

QsConnectionAsyncWorker::QsConnectionAsyncWorker(QsConnectionConfig&& config,
                                                 QObject*             parent)
    : QObject(parent),
      _activeThread {nullptr},
      _connectionConfig {std::move(config)}
{}

QsConnectionAsyncWorker::~QsConnectionAsyncWorker() noexcept
{
    stopAndWait();
}

OperationResult
QsConnectionAsyncWorker::execute(Task      task,
                                 OnSuccess onSuccess,
                                 OnError   onError,
                                 bool      handleInWorkerThread) Q_DECL_NOTHROW
{
    OperationResult result;
    TaskPtr taskPtr;
    HandlerPtr handlerPtr;

    result.first = false;

    // try move task into taskPtr
    result.second = createTaskPtr<Task>(taskPtr, task);
    if (result.second.isEmpty()) {
        // try move onSuccess and onError into handlerPtr
        result.second = createHandlerPtr(handlerPtr, onSuccess, onError);

        // if success, try start execution
        if (result.second.isEmpty()) {
            result = execute(std::move(taskPtr), std::move(handlerPtr),
                             handleInWorkerThread);
        }
    }

    return result;
}

OperationResult QsConnectionAsyncWorker::execute(
        TaskPtr    taskPtr,
        HandlerPtr handlerPtr,
        bool       handleInWorkerThread) Q_DECL_NOTHROW
{
    // check if worker and worker thread exists
    OperationResult result(true, checkWorkerThread());

    // if success, send task to worker,
    // otherwise return error message in result
    if (result.second.isEmpty()) {
        emit execWithHandlerSignal(std::move(taskPtr), std::move(handlerPtr),
                                   handleInWorkerThread, QPrivateSignal());
    } else {
        result.first = false;
    }

    return result;
}

OperationResult QsConnectionAsyncWorker::execute(Task     task,
                                                 QVariant data) Q_DECL_NOTHROW
{
    OperationResult result;
    TaskPtr taskPtr;

    result.first = false;

    // try move task into taskPtr
    result.second = createTaskPtr<Task>(taskPtr, task);
    if (result.second.isEmpty()) {
        // if success, try start execution
        if (result.second.isEmpty()) {
            result = execute(std::move(taskPtr), std::move(data));
        }
    }

    return result;
}

OperationResult QsConnectionAsyncWorker::execute(TaskPtr  taskPtr,
                                                 QVariant data) Q_DECL_NOTHROW
{
    // check if worker and worker thread exists
    OperationResult result(true, checkWorkerThread());

    // if success, send task to worker,
    // otherwise return error message in result
    if (result.second.isEmpty()) {
        emit execWithDataSignal(std::move(taskPtr), std::move(data),
                                QPrivateSignal());
    } else {
        result.first = false;
    }

    return result;
}

OperationResult QsConnectionAsyncWorker::execute(
        StmtTask   task,
        QByteArray query,
        OnSuccess  onSuccess,
        OnError    onError,
        bool       inTransaction,
        bool       handleInWorkerThread) Q_DECL_NOTHROW
{
    OperationResult result;
    StmtTaskPtr taskPtr;
    HandlerPtr handlerPtr;

    result.first = false;

    // try move task into taskPtr
    result.second = createTaskPtr<StmtTask>(taskPtr, task);
    if (result.second.isEmpty()) {
        // try move onSuccess and onError into handlerPtr
        result.second = createHandlerPtr(handlerPtr, onSuccess, onError);

        // if success, try start execution
        if (result.second.isEmpty()) {
            result = execute(std::move(taskPtr), std::move(query),
                             std::move(handlerPtr), inTransaction,
                             handleInWorkerThread);
        }
    }

    return result;
}

OperationResult QsConnectionAsyncWorker::execute(
        StmtTaskPtr taskPtr,
        QByteArray  query,
        HandlerPtr  handlerPtr,
        bool        inTransaction,
        bool        handleInWorkerThread) Q_DECL_NOTHROW
{
    // check if worker and worker thread exists
    OperationResult result(true, checkWorkerThread());

    // if success, send task to worker,
    // otherwise return error message in result
    if (result.second.isEmpty()) {
        emit execStmtWithHandlerSignal(std::move(taskPtr), std::move(query),
                                       inTransaction, std::move(handlerPtr),
                                       handleInWorkerThread, QPrivateSignal());
    } else {
        result.first = false;
    }

    return result;
}

OperationResult
QsConnectionAsyncWorker::execute(StmtTask   task,
                                 QByteArray query,
                                 bool       inTransaction,
                                 QVariant   data) Q_DECL_NOTHROW
{
    OperationResult result;
    StmtTaskPtr taskPtr;

    result.first = false;

    // try move task into taskPtr
    result.second = createTaskPtr<StmtTask>(taskPtr, task);
    if (result.second.isEmpty()) {
        // if success, try start execution
        if (result.second.isEmpty()) {
            result = execute(std::move(taskPtr), std::move(query),
                             inTransaction, std::move(data));
        }
    }

    return result;
}

OperationResult
QsConnectionAsyncWorker::execute(StmtTaskPtr taskPtr,
                                 QByteArray  query,
                                 bool        inTransaction,
                                 QVariant    data) Q_DECL_NOTHROW
{
    // check if worker and worker thread exists
    OperationResult result(true, checkWorkerThread());

    // if success, send task to worker,
    // otherwise return error message in result
    if (result.second.isEmpty()) {
        emit execStmtWithDataSignal(std::move(taskPtr), std::move(query),
                                    inTransaction, std::move(data),
                                    QPrivateSignal());
    } else {
        result.first = false;
    }

    return result;
}

std::pair<bool, QByteArray> QsConnectionAsyncWorker::stop(
        const unsigned long waitMilliseconds) Q_DECL_NOTHROW
{
    return disconnectWorkerObject(true, waitMilliseconds);
}

std::pair<bool, QByteArray>
QsConnectionAsyncWorker::stopAndWait() Q_DECL_NOTHROW
{
    return stop(ULONG_MAX);
}

void QsConnectionAsyncWorker::onExecuted(
        QsConnectionWorker::ExecResultPtr resultPtr,
        HandlerPtr                        handlerPtr) Q_DECL_NOTHROW
{
    // check if resultPtr is not empty
    if (resultPtr) {
        // try process result
        QByteArray errorMsg = qs::processExexResult(
                    resultPtr.get(), handlerPtr);

        // check if no exception while process result
        // (otherwise emit signal with exception error message)
        if (!errorMsg.isEmpty()) {
            emit error(std::move(errorMsg));
        }
    }
}

void QsConnectionAsyncWorker::onThreadFinished() Q_DECL_NOTHROW
{
    disconnectWorkerObject(false, 0);
}

QByteArray QsConnectionAsyncWorker::checkWorkerThread() Q_DECL_NOTHROW
{
    QByteArray result;
    QsWorkerThread* thread = _activeThread.loadAcquire();

    // if thread not exists, try create it
    if (!thread) {
        try {
            createWorkerThread();
        } catch (const std::exception& exception) {
            try {
                result = exception.what();
            } catch (...) {
                result = qs::badAllocErrMsg;
            }
        } catch (...) {
            result = qs::unknownExceptionErrMsg;
        }
    }

    // return error message (if operation success, it will be empty)
    return result;
}

void QsConnectionAsyncWorker::connectTo(QsConnectionWorker* worker)
{
    // connect this signals to worker object slots and save connections to array
    _workerObjConnections.append(
                connect(this, &QsConnectionAsyncWorker::execWithDataSignal,
                        worker, &QsConnectionWorker::execWithData,
                        Qt::QueuedConnection));
    _workerObjConnections.append(
                connect(this, &QsConnectionAsyncWorker::execWithHandlerSignal,
                        worker, &QsConnectionWorker::execWithHandler,
                        Qt::QueuedConnection));
    _workerObjConnections.append(
                connect(this, &QsConnectionAsyncWorker::execStmtWithDataSignal,
                        worker, &QsConnectionWorker::execStatementWithData,
                        Qt::QueuedConnection));
    _workerObjConnections.append(
                connect(this,
                        &QsConnectionAsyncWorker::execStmtWithHandlerSignal,
                        worker, &QsConnectionWorker::execStatementWithHandler,
                        Qt::QueuedConnection));

    // connect worker object signals to this slots
    connect(worker, &QsConnectionWorker::finished,
            this, &QsConnectionAsyncWorker::finished,
            Qt::QueuedConnection);
    connect(worker, &QsConnectionWorker::executed,
            this, &QsConnectionAsyncWorker::onExecuted,
            Qt::QueuedConnection);
    connect(worker, &QsConnectionWorker::error,
            this, &QsConnectionAsyncWorker::error,
            Qt::QueuedConnection);
    connect(worker, &QsConnectionWorker::errorWithData,
            this, &QsConnectionAsyncWorker::errorWithData,
            Qt::QueuedConnection);
}

void QsConnectionAsyncWorker::createWorkerThread()
{
    // lock mutex
    QMutexLocker locker {&_mutex};

    // check if thread not exists
    QsWorkerThread* thread = _activeThread.loadAcquire();
    if (!thread) {

        // create worker object and new thread
        std::unique_ptr<QsConnectionWorker> newWorker =
                std::make_unique<QsConnectionWorker>(_connectionConfig);
        std::unique_ptr<QsWorkerThread> newThread =
                std::make_unique<QsWorkerThread>();

        // save pointers to created object
        thread = newThread.get();
        QsConnectionWorker* worker = newWorker.get();

        // move worker object to new thread
        worker->moveToThread(thread);

        // connect current object with created thread and save connection
        _workerObjConnections.append(
                    connect(thread, &QsWorkerThread::finished,
                    this, &QsConnectionAsyncWorker::onThreadFinished));

        // connect new worker thread signal 'finished' to worker object
        // slot 'deleteLater' (to manage lifetime of the worker object)
        connect(thread, &QsWorkerThread::destroyed,
                worker, &QsConnectionWorker::deleteLater);

        // connect this object with worker object (signals and slots)
        // and save connections into _workerObjConnections
        connectTo(worker);

        // start created thread
        thread->start();

        // delete new created object from std::unique_ptr
        newThread.release();
        newWorker.release();

        // save pointer to thread
        _activeThread.storeRelease(thread);
    }
}

OperationResult
QsConnectionAsyncWorker::disconnectWorkerObject(
        bool          quitThread,
        unsigned long waitMilliseconds) Q_DECL_NOTHROW
{
    OperationResult result;

    // clear pointer to worker thread and save previous pointer
    QsWorkerThread* thread = _activeThread.fetchAndStoreRelease(nullptr);

    result.first = (thread == nullptr);

    // if thread exists, disconnect from it
    if (thread) {
        try {
            // quit thread, if needed
            if (quitThread) {
                thread->quit();
            }

            // disconnect from worker object slots
            QMutexLocker locker {&_mutex};
            for (const auto& conn : _workerObjConnections) {
                disconnect(conn);
            }

            // delete connections
            _workerObjConnections = QVector<QMetaObject::Connection>();

            // wait thread finish, if needed
            if (waitMilliseconds > 0) {
                result.first = thread->wait(waitMilliseconds);
            } else {
                result.first = true;
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

    return result;
}

#include "qsconnectionasyncworker.moc"
