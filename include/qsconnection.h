#ifndef QS_CONNECTION_H
#define QS_CONNECTION_H

#include <functional>
#include <memory>
#include <utility>

#include <QByteArray>
#include <QCollator>
#include <QHash>
#include <QLocale>
#include <QString>

#include "sqlite3.h"
#include "qsstatement.h"


struct sqlite3;

class QsConnection
{

public:

    friend QsStatement::QsStatement(const QsConnection& connection,
                                    const QByteArray&   query) noexcept;

    friend QsStatement::QsStatement(const QsConnection& connection,
                                    const QString&      query) noexcept;

    /*
    friend void swap(QsConnection& first, QsConnection& second) noexcept
    {
        using std::swap;

        swap(first.mSize, second.mSize);
        swap(first.mArray, second.mArray);
    }
    */

    enum QueryResult {
        Ok = 0
    };

    enum ReadResult : int {
        ReadSuccess = 0,
        ConnectionIsClosed = -1,
        NoData = -2,
        EmptyData = -3,
        NullValue = -4
    };

    enum CacheMode {
        PrivateCache = 0,
        SharedCache
    };

    enum OpenMode {
        ReadWriteCreate = 0,
        ReadWrite,
        ReadOnly,
        InMemory
    };

    enum ThreadMode {
        Default,
        Serialized,
        MultiThread,
        SingleThread
    };

    // default CacheMode value for initializing object in constructor
    static const CacheMode defaultCacheMode { CacheMode::PrivateCache };

    // default OpenMode value for initializing object in constructor
    static const OpenMode  defaultOpenMode { OpenMode::ReadWriteCreate };

    // default ThreadMode value for initializing object in constructor
    static const ThreadMode defaultThreadMode { ThreadMode::Default };

    QsConnection(const QByteArray& dbName = QByteArray()) Q_DECL_NOTHROW;

    QsConnection(QsConnection&& connection) Q_DECL_NOTHROW;

    virtual ~QsConnection();

    void close() Q_DECL_NOTHROW;

    bool commit() Q_DECL_NOTHROW;

    bool createUtf16Collation(const QByteArray& collationName,
                              const QLocale&    locale);

    bool deleteUtf16Collation(const QByteArray& collationName);

    bool execute(const QByteArray& query) const noexcept;

    inline bool execute(const QString& query) const;

    inline QByteArray databaseName() const Q_DECL_NOTHROW
    {
        return _dbName;
    }

    inline bool isOpen() const noexcept
    {
        return _db != NULL;
    }

    int lastErrorCode() const noexcept;

    QByteArray lastError() const;

    QString lastError16() const;

    qint64 lastInsertRowId() const noexcept;

    bool open(OpenMode   openMode   = defaultOpenMode,
              ThreadMode threadMode = defaultThreadMode,
              CacheMode  cacheMode  = defaultCacheMode);

    inline QsStatement prepare(const QByteArray& query) Q_DECL_NOTHROW
    {
        return QsStatement(*this, query);
    }

    inline QsStatement prepare(const QString& query) Q_DECL_NOTHROW
    {
        return QsStatement(*this, query);
    }

    std::pair<double, int> readDouble(const QByteArray& query);

    std::pair<double, int> readDouble(const QString& query);

    std::pair<qint64, int> readInt64(const QByteArray& query);

    std::pair<qint64, int> readInt64(const QString& query);

    std::pair<QByteArray, int> readString(const QByteArray& query);

    std::pair<QByteArray, int> readString(const QString& query);

    std::pair<QString, int> readString16(const QByteArray& query);

    std::pair<QString, int> readString16(const QString& query);

    bool rollback() Q_DECL_NOTHROW;

    void setDatabaseName(const QByteArray& dbName) Q_DECL_NOTHROW;

    bool transaction() Q_DECL_NOTHROW;

    QsConnection& operator =(QsConnection&& connection) Q_DECL_NOTHROW;

    QsConnection(const QsConnection&) = delete;
    QsConnection& operator =(const QsConnection&) = delete;

private:

    sqlite3*   _db;
    QByteArray _dbName;
    QByteArray _openErrorMsg;

    QHash<QByteArray, std::shared_ptr<QCollator> > _collators;

    int openInMemoryDb(CacheMode cacheMode);

    int openRegularDb(const int flags) noexcept;

    int readValue(const QByteArray&                          query,
                  const std::function<void (sqlite3_stmt*)>& readLambda);

    void reset() noexcept;

};

#endif
