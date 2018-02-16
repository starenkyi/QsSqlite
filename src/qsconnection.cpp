#include "../include/qsconnection.h"

#include <cstring>
#include <atomic>

#include <QReadLocker>
#include <QWriteLocker>

#include "../include/sqlite3.h"
#include "../include/qsstatement.h"

namespace {

int getOpenFlags(const QsConnection::OpenMode   openMode,
                 const QsConnection::ThreadMode threadMode,
                 const QsConnection::CacheMode  cacheMode) noexcept
{
    int resFlags = 0;

    // set uri open mode
    switch (openMode) {
    case QsConnection::OpenMode::ReadWriteCreate:
    case QsConnection::OpenMode::InMemory:
        resFlags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
        break;
    case QsConnection::OpenMode::ReadWrite:
        resFlags |= SQLITE_OPEN_READWRITE;
        break;
    case QsConnection::OpenMode::ReadOnly:
        resFlags |= SQLITE_OPEN_READONLY;
        break;
    }

    // set thread mode
    switch (threadMode) {
    case QsConnection::ThreadMode::MultiThread:
        resFlags |= SQLITE_OPEN_NOMUTEX;
        break;
    case QsConnection::ThreadMode::Serialized:
        resFlags |= SQLITE_OPEN_FULLMUTEX;
        break;
    default:
        break;
    }

    // set cache mode
    switch (cacheMode) {
    case QsConnection::CacheMode::PrivateCache:
        resFlags |= SQLITE_OPEN_PRIVATECACHE;
        break;
    case QsConnection::CacheMode::SharedCache:
        resFlags |= SQLITE_OPEN_SHAREDCACHE;
        break;
    }

    return resFlags;
}

// function to compare UTF16 string by locale
// (collator is a pointer to some QCollator object)
int localeCompareUtf16(void* collator,
                       int firstLength,
                       const void* firstStr,
                       int secondLength,
                       const void* secondStr)
{
    return reinterpret_cast<QCollator*>(collator)->compare(
                reinterpret_cast<const QChar*>(firstStr), firstLength / 2,
                reinterpret_cast<const QChar*>(secondStr), secondLength / 2);
}

}


using DoubleResult   = std::pair<double,     int>;
using Int64Result    = std::pair<qint64,     int>;
using StringResult   = std::pair<QByteArray, int>;
using String16Result = std::pair<QString,    int>;

using CollatorContainer = QHash<QByteArray, std::shared_ptr<QCollator> >;


QsConnection::QsConnection(const QByteArray& dbName) Q_DECL_NOTHROW
    : _db {NULL},
      _dbName {dbName}
{}

QsConnection::QsConnection(QsConnection&& connection) Q_DECL_NOTHROW
    : _db {connection._db},
      _dbName {std::move(connection._dbName)},
      _openErrorMsg {std::move(connection._openErrorMsg)},
      _collators {std::move(connection._collators)}
{
    connection.reset();
}

QsConnection::~QsConnection()
{
    close();
}

void QsConnection::close() Q_DECL_NOTHROW
{
    // check if connection is opened
    if (_db) {
        // close connection and reset
        sqlite3_close_v2(_db);
        _db = NULL;

        // clear open error message and collators list
        _openErrorMsg.clear();
        _collators = CollatorContainer();
    }
}

bool QsConnection::commit() Q_DECL_NOTHROW
{
    return execute(QByteArrayLiteral("commit"));
}

bool QsConnection::createUtf16Collation(const QByteArray& collationName,
                                        const QLocale&    locale)
{
    // check if collation not exists
    if (_db && !_collators.contains(collationName)) {
        // create collator object for locale
        auto it = _collators.insert(collationName,
                                    std::make_shared<QCollator>(locale));
        QCollator* ptr = it.value().get();

        // try register collator and return true on success
        if (sqlite3_create_collation_v2(
                    _db, collationName.constData(), SQLITE_UTF16, ptr,
                    localeCompareUtf16, NULL) == SQLITE_OK) {
            return true;
        }

        // since create collation fail, delete collator object and return false
        _collators.erase(it);
        return false;
    }

    // return false if connection is closed or collation exists
    return false;
}

bool QsConnection::deleteUtf16Collation(const QByteArray& collationName)
{
    // check if connection is opened and try delete collation for it
    if (_db && sqlite3_create_collation_v2(
                _db, collationName.constData(),
                SQLITE_UTF16, NULL, NULL, NULL) == SQLITE_OK) {

        // delete collator object
        _collators.remove(collationName);
    }

    // return false if connection is closed or delete collation fail
    return false;
}

bool QsConnection::execute(const QByteArray& query) const noexcept
{
    // execute query if connection is opened
    return _db && sqlite3_exec(_db, query.constData(),
                               NULL, NULL, NULL) == SQLITE_OK;
}

bool QsConnection::execute(const QString& query) const
{
    return execute(query.toUtf8());
}

int QsConnection::lastErrorCode() const noexcept
{
    return (_db) ? sqlite3_errcode(_db) : ReadResult::ConnectionIsClosed;
}

QByteArray QsConnection::lastError() const
{
    QByteArray result;

    // try get error
    if (_db) {
        result = sqlite3_errmsg(_db);
    } else {
        result = _openErrorMsg;
    }

    return result;
}

QString QsConnection::lastError16() const
{
    QString result;

    // try get error (if database is opened)
    if (_db) {
        result = QString(reinterpret_cast<const QChar*>(sqlite3_errmsg16(_db)));
    } else {
        result = _openErrorMsg;
    }

    return result;
}

qint64 QsConnection::lastInsertRowId() const noexcept
{
    return (_db) ? sqlite3_last_insert_rowid(_db) : 0;
}

bool QsConnection::open(OpenMode   openMode,
                        ThreadMode threadMode,
                        CacheMode  cacheMode)
{
    // check connection is not opened
    if (!_db) {

        // try open database
        const int code = (openMode != OpenMode::InMemory)
                ? openRegularDb(getOpenFlags(openMode, threadMode, cacheMode))
                : openInMemoryDb(cacheMode);

        // check result
        if (code != SQLITE_OK) {
            // read and save last error
            _openErrorMsg = sqlite3_errmsg(_db);

            // release sqlite3 pointer and return false
            sqlite3_close_v2(_db);
            _db = NULL;
            return false;
        } else {
            _openErrorMsg.clear();
        }
    }

    // return true (connection is opened, or connection was opened before)
    return true;
}

DoubleResult QsConnection::readDouble(const QByteArray& query)
{
    DoubleResult result;

    // try read double and save read result code
    result.second = readValue(query,
                              [&result] (sqlite3_stmt* stmt) -> void {
        result.first = sqlite3_column_double(stmt, 0);
    });

    // return result
    return result;
}

std::pair<double, int> QsConnection::readDouble(const QString& query)
{
    return readDouble(query.toUtf8());
}

Int64Result QsConnection::readInt64(const QByteArray& query)
{
    Int64Result result;

    // try read int64 and save read result code
    result.second = readValue(query,
                              [&result] (sqlite3_stmt* stmt) -> void {
        result.first = sqlite3_column_int64(stmt, 0);
    });

    // return result
    return result;
}

std::pair<qint64, int> QsConnection::readInt64(const QString& query)
{
    return readInt64(query.toUtf8());
}

StringResult QsConnection::readString(const QByteArray& query)
{
    StringResult result;

    // try read string and save read result code
    result.second = readValue(query, [&result] (sqlite3_stmt* stmt) -> void {
        result.first = QByteArray(reinterpret_cast<const char*>
                                  (sqlite3_column_text(stmt, 0)),
                                  sqlite3_column_bytes(stmt, 0));
    });

    // return result
    return result;
}

std::pair<QByteArray, int> QsConnection::readString(const QString& query)
{
    return readString(query.toUtf8());
}

String16Result QsConnection::readString16(const QByteArray& query)
{
    String16Result result;

    // try read string and save read result code
    result.second = readValue(query, [&result] (sqlite3_stmt* stmt) -> void {
        result.first = QString(reinterpret_cast<const QChar*>
                               (sqlite3_column_text16(stmt, 0)),
                               sqlite3_column_bytes16(stmt,0) / 2);
    });

    // return result
    return result;
}

std::pair<QString, int> QsConnection::readString16(const QString& query)
{
    return readString16(query.toUtf8());
}

bool QsConnection::rollback() Q_DECL_NOTHROW
{
    return execute(QByteArrayLiteral("rollback"));
}

void QsConnection::setDatabaseName(const QByteArray& dbName) Q_DECL_NOTHROW
{
    // check if connection is open and assign value
    if (!_db) {
        _dbName = dbName;
    }
}

bool QsConnection::transaction() Q_DECL_NOTHROW
{
    return execute(QByteArrayLiteral("begin"));
}

QsConnection& QsConnection::operator =(QsConnection&& connection) Q_DECL_NOTHROW
{
    if (this != &connection) {
        // close current connection
        close();

        // move assign object vars
        _db = connection._db;
        _dbName = std::move(connection._dbName);
        _openErrorMsg = std::move(connection._openErrorMsg);
        _collators = std::move(connection._collators);

        // reset moved object
        connection.reset();
    }

    return *this;
}

int QsConnection::openInMemoryDb(CacheMode cacheMode)
{
    // build URI string
    QByteArray uriStr("file:");
    if (_dbName.isEmpty()) {
        uriStr += ":memory:?cache=";
    } else {
        uriStr += _dbName;
        uriStr += "?mode=memory&cache=";
    }

    uriStr += (cacheMode == CacheMode::PrivateCache) ? "private" : "shared";

    // try open in-memory db and return result code
    return sqlite3_open_v2(uriStr.constData(), &_db, SQLITE_OPEN_URI, NULL);
}

int QsConnection::openRegularDb(const int flags) noexcept
{
    return sqlite3_open_v2(_dbName.constData(), &_db, flags, NULL);
}

int
QsConnection::readValue(const QByteArray&                          query,
                        const std::function<void (sqlite3_stmt*)>& readLambda)
{
    // check connection
    if (_db) {
        // try prepare statement
        sqlite3_stmt *stmt;
        int resultCode = sqlite3_prepare_v2(_db, query.constData(),
                                            query.length(), &stmt, NULL);

        // if success, try read data (or set the error code)
        if (resultCode == SQLITE_OK) {

            // check if statement return any data
            if (sqlite3_column_count(stmt)) {

                // check if statement has prepared row
                if (sqlite3_step(stmt) == SQLITE_ROW) {

                    // check value type is not NULL
                    if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {

                        // catch potential exception of std::function object
                        try {
                            // read value
                            readLambda(stmt);
                            resultCode = ReadSuccess;
                        } catch (...) {
                            // delete prepared statement and re-throw
                            sqlite3_finalize(stmt);
                            throw;
                        }

                    } else {
                        resultCode = NullValue;
                    }
                } else {
                    resultCode = EmptyData;
                }
            } else {
                resultCode = NoData;
            }

            // delete prepared statement and return result code
            sqlite3_finalize(stmt);
        }

        // return code of sqlite error
        return resultCode;
    } else {
        // return ConnectionIsClosed error code
        return ConnectionIsClosed;
    }
}

void QsConnection::reset() Q_DECL_NOTHROW
{
    // reset all fields to default values
    _db = NULL;
    _dbName = QByteArray();
    _openErrorMsg = QByteArray();
    _collators = CollatorContainer();
}

