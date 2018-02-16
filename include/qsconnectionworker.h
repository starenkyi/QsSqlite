#ifndef QS_CONNECTION_WORKER_H
#define QS_CONNECTION_WORKER_H

#include <functional>
#include <memory>
#include <utility>

#include <QByteArray>
#include <QObject>
#include <QVariant>

#include "qsconnection.h"
#include "qsconnectionconfig.h"
#include "qsstatement.h"


class QsConnectionWorker : public QObject
{
    Q_OBJECT

public:

    using Task          = std::function<QVariant (QsConnection& connection)>;
    using TaskPtr       = std::shared_ptr<Task>;

    using StmtTask    = std::function<QVariant (QsStatement statement,
                                                bool&       commitChanges)>;
    using StmtTaskPtr = std::shared_ptr<StmtTask>;

    using ExecResult    = std::pair<QVariant,QByteArray>;
    using ExecResultPtr = std::shared_ptr<ExecResult>;

    using OnSuccess       = std::function<void (QVariant   result)>;
    using OnError         = std::function<void (QByteArray errorMessage)>;
    using Handler         = std::pair<OnSuccess, OnError>;
    using HandlerPtr      = std::shared_ptr<Handler>;


    QsConnectionWorker(const QsConnectionConfig& config,
                       QObject*                  parent = nullptr);

    QsConnectionWorker(QsConnectionConfig&& config,
                       QObject*             parent = nullptr);

    virtual ~QsConnectionWorker() = default;

    inline void closeConnection() Q_DECL_NOTHROW
    {
        _connection.close();
    }

    inline bool isConnectionOpen() const noexcept
    {
        return _connection.isOpen();
    }

    ExecResult exec(const Task& task) Q_DECL_NOTHROW;

    ExecResult exec(const StmtTask& task,
                    const QByteArray&    query,
                    bool                 inTransaction = true) Q_DECL_NOTHROW;

    inline QByteArray lastError() const Q_DECL_NOTHROW
    {
        return _connectionConfig.lastError();
    }

    bool openConnection();

    QsConnectionWorker() = delete;
    QsConnectionWorker(const QsConnectionWorker&) = delete;
    QsConnectionWorker(QsConnectionWorker&&) = delete;
    QsConnectionWorker& operator =(const QsConnectionWorker&) = delete;
    QsConnectionWorker& operator =(QsConnectionWorker&&) = delete;

public slots:

    void execWithHandler(TaskPtr    taskPtr,
                         HandlerPtr handlerPtr,
                         bool       runHandler) Q_DECL_NOTHROW;

    void execWithData(TaskPtr  taskPtr,
                      QVariant data) Q_DECL_NOTHROW;

    void execStatementWithHandler(StmtTaskPtr stmtPtr,
                                  QByteArray  query,
                                  bool        inTransaction,
                                  HandlerPtr  handlerPtr,
                                  bool        runHandler) Q_DECL_NOTHROW;

    void execStatementWithData(StmtTaskPtr stmtPtr,
                               QByteArray  query,
                               bool        inTransaction,
                               QVariant    data) Q_DECL_NOTHROW;

signals:

    void error(QByteArray errorMessage);

    void errorWithData(QByteArray errorMessage,
                       QVariant   data);

    void executed(ExecResultPtr resultPtr,
                  HandlerPtr    handlerPtr);

    void finished(QVariant result,
                  QVariant data);

private:

    QsConnection       _connection;
    QsConnectionConfig _connectionConfig;

    void processExecResultWithHandler(ExecResult& result,
                                      HandlerPtr& handlerPtr,
                                      const bool  runCallback) Q_DECL_NOTHROW;

    void processExecResultWithData(ExecResult& result,
                                   QVariant&   data) Q_DECL_NOTHROW;

    void tryRunStmtTask(const StmtTask&    stmtTask,
                        const QByteArray&  query,
                        ExecResult&        result,
                        bool               inTransaction) Q_DECL_NOTHROW;

    void tryRunTask(const Task& task,
                    ExecResult& result) Q_DECL_NOTHROW;

};

Q_DECLARE_METATYPE(QsConnectionWorker::TaskPtr)
Q_DECLARE_METATYPE(QsConnectionWorker::StmtTaskPtr)
Q_DECLARE_METATYPE(QsConnectionWorker::ExecResultPtr)
Q_DECLARE_METATYPE(QsConnectionWorker::HandlerPtr)

#endif
