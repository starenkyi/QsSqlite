#include "../include/qsstatement.h"

#include <cstring>

#include <QtGlobal>

#include "../include/sqlite3.h"
#include "../include/qsconnection.h"

using BlobData      = QPair<unsigned char*, int>;
using ConstBlobData = QPair<const unsigned char*, int>;

using CStrData      = QPair<char*, int>;
using ConstCStrData = QPair<const char*, int>;

using CStr16Data      = QPair<QChar*, int>;
using ConstCStr16Data = QPair<const QChar*, int>;


QsStatement::QsStatement() noexcept
    : _statement {NULL},
      _db {NULL}
{}

QsStatement::QsStatement(const QsConnection& connection,
                         const QByteArray&   query) noexcept
    : _statement {NULL},
      _db {connection._db}
{
    // try compile statement and check result
    if (!compile(query)) {
        _db = NULL;
    }
}

QsStatement::QsStatement(const QsConnection& connection,
                         const QString&      query) noexcept
    : _statement {NULL},
      _db {connection._db}
{
    // try compile statement and check result
    if (!compile(query)) {
        _db = NULL;
    }
}

QsStatement::QsStatement(QsStatement&& statement) noexcept
    : _statement {statement._statement},
      _db {statement._db}
{
    statement.reset();
}

QsStatement::~QsStatement()
{
    clear();
}

bool QsStatement::bindBlob(const int         index,
                           const void* const value,
                           const int         bytes) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindBlob", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindBlob", "index out of range");
    Q_ASSERT_X(bytes >= 0, "bindBlob", "bytes mustn't be negative number");

    return sqlite3_bind_blob(_statement, index, value,
                             bytes, SQLITE_STATIC) == SQLITE_OK;
}

bool QsStatement::bindBlob(const int         index,
                           const QByteArray& value) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindBlob", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindBlob", "index out of range");

    return sqlite3_bind_blob(_statement, index, value.constData(),
                             value.length(), SQLITE_STATIC) == SQLITE_OK;
}

bool QsStatement::bindBlobCopy(const int         index,
                               const void* const value,
                               const int         bytes) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindBlobCopy", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindBlobCopy", "index out of range");
    Q_ASSERT_X(bytes >= 0, "bindBlob", "bytes mustn't be negative number");

    return sqlite3_bind_blob(_statement, index, value,
                             bytes, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool QsStatement::bindBlobCopy(const int         index,
                               const QByteArray& value) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindBlobCopy", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindBlobCopy", "index out of range");

    return sqlite3_bind_blob(_statement, index, value.constData(),
                             value.length(), SQLITE_TRANSIENT) == SQLITE_OK;
}

bool QsStatement::bindBool(const int  index,
                           const bool value) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindBool", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindBool", "index out of range");

    return sqlite3_bind_int(_statement, index, value) == SQLITE_OK;
}

int QsStatement::bindCount() const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindCount", "Statement is invalid");

    return sqlite3_bind_parameter_count(_statement);
}

bool QsStatement::bindDouble(const int    index,
                             const double value) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindDouble", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindBool", "index out of range");

    return sqlite3_bind_double(_statement, index, value) == SQLITE_OK;
}

bool QsStatement::bindInt(const int index,
                          const int value) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindInt", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindInt", "index out of range");

    return sqlite3_bind_int(_statement, index, value) == SQLITE_OK;
}

bool QsStatement::bindInt64(const int    index,
                            const qint64 value) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindInt64", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindInt64", "index out of range");

    return sqlite3_bind_int64(_statement, index, value) == SQLITE_OK;
}

bool QsStatement::bindNull(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindNull", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindNull", "index out of range");

    return sqlite3_bind_null(_statement, index) == SQLITE_OK;
}

bool QsStatement::bindText(const int         index,
                           const char* const value,
                           const int         bytes) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindText", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindText", "index out of range");

    return sqlite3_bind_text(_statement, index, value,
                             bytes, SQLITE_STATIC) == SQLITE_OK;
}

bool QsStatement::bindText16(const int         index,
                             const void* const value,
                             const int         bytes) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindText16", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindText16", "index out of range");
    Q_ASSERT_X(bytes % 2 == 0, "bindText16", "bytes must be an even number");

    return sqlite3_bind_text16(_statement, index, value,
                               bytes, SQLITE_STATIC) == SQLITE_OK;
}

bool QsStatement::bindText16(const int      index,
                             const QString& value) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindText16", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindText16", "index out of range");

    return sqlite3_bind_text16(_statement, index, value.constData(),
                               value.length() << 1, SQLITE_STATIC) == SQLITE_OK;
}

bool QsStatement::bindText16Copy(const int         index,
                                 const void* const value,
                                 const int         bytes) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindText16Copy", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindText16Copy", "index out of range");
    Q_ASSERT_X(bytes % 2 == 0, "bindText16", "bytes must be an even number");

    return sqlite3_bind_text16(_statement, index, value,
                               bytes, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool QsStatement::bindText16Copy(const int      index,
                                 const QString& value) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindText16Copy", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindText16Copy", "index out of range");

    return sqlite3_bind_text16(_statement, index, value.constData(),
                               value.length() << 1, SQLITE_TRANSIENT)
            == SQLITE_OK;
}

bool QsStatement::bindTextCopy(const int         index,
                               const char* const value,
                               const int         bytes) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "bindTextCopy", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindTextCopy", "index out of range");

    return sqlite3_bind_text(_statement, index, value,
                             bytes, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool QsStatement::bindTextCopy(const int      index,
                               const QString& value) const
{
    Q_ASSERT_X(_statement != NULL, "bindTextCopy", "Statement is invalid");
    Q_ASSERT_X(index > 0 && index <= sqlite3_bind_parameter_count(_statement),
               "bindTextCopy", "index out of range");

    const QByteArray& textUtf8 = value.toUtf8();

    return sqlite3_bind_text(_statement, index, textUtf8.constData(),
                             textUtf8.length(), SQLITE_TRANSIENT) == SQLITE_OK;
}

unsigned QsStatement::byteLength(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "byteLength", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_bind_parameter_count(_statement),
               "byteLength", "index out of range");

    return sqlite3_column_bytes(_statement, index);
}

unsigned QsStatement::byteLength16(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "byteLength16", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_bind_parameter_count(_statement),
               "byteLength16", "index out of range");

    return sqlite3_column_bytes16(_statement, index);
}

void QsStatement::clear() noexcept
{
    if (_statement) {
        sqlite3_finalize(_statement);
        reset();
    }
}

void QsStatement::clearBindings() const noexcept
{
    Q_ASSERT_X(_statement != NULL, "clearBindings", "Statement is invalid");

    sqlite3_clear_bindings(_statement);
}

int QsStatement::columnCount() const noexcept
{
    Q_ASSERT_X(_statement != NULL, "columnCount", "Statement is invalid");

    return sqlite3_column_count(_statement);
}


QsStatement::DataType QsStatement::columnType(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "columnType", "Statement is invalid");

    const int typeId = sqlite3_column_type(_statement, index);
    switch (typeId) {
    case SQLITE_INTEGER:
        return DataType::Integer;
    case SQLITE_TEXT:
        return DataType::Text;
    case SQLITE_FLOAT:
        return DataType::Double;
    case SQLITE_BLOB:
        return DataType::Blob;
    default:
        return DataType::Null;
    };
}

bool QsStatement::execute() const noexcept
{
    Q_ASSERT_X(_statement != NULL, "execute", "Statement is invalid");

    if (sqlite3_step(_statement) == SQLITE_DONE) {
        sqlite3_reset(_statement);
        return true;
    }

    return false;
}

QByteArray QsStatement::expandedQuery() const
{
    Q_ASSERT_X(_statement != NULL, "expandedQuery", "Statement is invalid");

    QByteArray result;
    char* str = sqlite3_expanded_sql(_statement);
    if (str) {
        result = str;
        sqlite3_free(str);
    }

    return result;
}

QString QsStatement::expandedQuery16() const
{
    Q_ASSERT_X(_statement != NULL, "expandedQuery", "Statement is invalid");

    QString result;
    char* str = sqlite3_expanded_sql(_statement);
    if (str) {
        result = std::move(QString::fromUtf8(str));
        sqlite3_free(str);
    }

    return result;
}

ConstBlobData QsStatement::getBlob(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "getBlob", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getBlob", "index out of range");

    return ConstBlobData {reinterpret_cast<const unsigned char*>
                (sqlite3_column_blob(_statement, index)),
                sqlite3_column_bytes(_statement, index)};
}

BlobData QsStatement::getBlobCopy(const int index) const
{
    Q_ASSERT_X(_statement != NULL, "getBlobCopy", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getBlobCopy", "index out of range");

    const void* const temp = sqlite3_column_blob(_statement, index);
    const int bytes = sqlite3_column_bytes(_statement, index);
    unsigned char* result {nullptr};
    if (bytes) {
        result = new unsigned char[bytes];
        memcpy(result, temp, bytes);
    }

    return BlobData {result, bytes};
}

bool QsStatement::getBool(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "getBool", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getBool", "index out of range");

    return sqlite3_column_int(_statement, index);
}

QByteArray QsStatement::getByteArray(const int index) const
{
    Q_ASSERT_X(_statement != NULL, "getByteArray", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getByteArray", "index out of range");

    return QByteArray(reinterpret_cast<const char*>
                      (sqlite3_column_blob(_statement, index)),
                      sqlite3_column_bytes(_statement, index));
}

ConstCStrData QsStatement::getCStr(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "getCStr", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getCStr", "index out of range");

    return ConstCStrData {reinterpret_cast<const char*>
                (sqlite3_column_text(_statement, index)),
                sqlite3_column_bytes(_statement, index)};
}

ConstCStr16Data
QsStatement::getCStr16(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "getCStr16", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getCStr16", "index out of range");

    return ConstCStr16Data {reinterpret_cast<const QChar*>
                (sqlite3_column_text16(_statement, index)),
                sqlite3_column_bytes16(_statement, index)};
}

CStrData QsStatement::getCStrCopy(const int index) const
{
    Q_ASSERT_X(_statement != NULL, "getCStrCopy", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getCStrCopy", "index out of range");

    const void* const from = sqlite3_column_text(_statement, index);
    const int bytes = sqlite3_column_bytes(_statement, index);
    char* result {nullptr};
    if (from) {
        result = new char[bytes + sizeof(char)];
        memcpy(result, from, bytes + sizeof(char));
    }

    return CStrData {result, bytes};
}

CStr16Data QsStatement::getCStr16Copy(const int index) const
{
    Q_ASSERT_X(_statement != NULL, "getCStr16Copy", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getCStr16Copy", "index out of range");

    const void* const from = sqlite3_column_text16(_statement, index);
    const int bytes = sqlite3_column_bytes16(_statement, index);
    QChar* result {nullptr};
    if (from) {
        result = new QChar[bytes + sizeof(QChar)];
        memcpy(result, from, bytes + sizeof(QChar));
    }

    return CStr16Data {result, bytes};
}

double QsStatement::getDouble(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "getDouble", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getDouble", "index out of range");

    return sqlite3_column_double(_statement, index);
}

int QsStatement::getInt(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "getInt", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getInt", "index out of range");

    return sqlite3_column_int(_statement, index);
}

qint64 QsStatement::getInt64(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "getInt64", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getInt64", "index out of range");

    return sqlite3_column_int64(_statement, index);
}

QString QsStatement::getString(const int index) const
{
    Q_ASSERT_X(_statement != NULL, "getString", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getString", "index out of range");

    return QString::fromUtf8(reinterpret_cast<const char*>
                             (sqlite3_column_text(_statement, index)),
                             sqlite3_column_bytes(_statement, index));
}

QString QsStatement::getString16(const int index) const
{
    Q_ASSERT_X(_statement != NULL, "getString16", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "getString16", "index out of range");

    return QString(reinterpret_cast<const QChar*>
                   (sqlite3_column_text16(_statement, index)),
                   sqlite3_column_bytes16(_statement, index));
}

bool QsStatement::isNull(const int index) const noexcept
{
    Q_ASSERT_X(_statement != NULL, "isNull", "Statement is invalid");
    Q_ASSERT_X(index >= 0 && index < sqlite3_column_count(_statement),
               "isNull", "index out of range");

    return sqlite3_column_type(_statement, index) == SQLITE_NULL;
}

QByteArray QsStatement::lastError() const
{
    Q_ASSERT_X(_db != NULL, "lastError", "Statement is invalid");

    return QByteArray(sqlite3_errmsg(_db));
}

QString QsStatement::lastError16() const
{
    Q_ASSERT_X(_db != NULL, "lastError16", "Statement is invalid");

    return QString(reinterpret_cast<const QChar*>(sqlite3_errmsg16(_db)));
}

int QsStatement::lastErrorCode() const noexcept
{
    Q_ASSERT_X(_db != NULL, "lastErrorCode", "Statement is invalid");

    return sqlite3_errcode(_db);
}

qint64 QsStatement::lastInsertRowId() const noexcept
{
    Q_ASSERT_X(_db != NULL, "lastInsertRowId", "Statement is invalid");

    return sqlite3_last_insert_rowid(_db);
}

bool QsStatement::next() const noexcept
{
    Q_ASSERT_X(_statement != NULL, "next", "Statement is invalid");

    return sqlite3_step(_statement) == SQLITE_ROW;
}

bool QsStatement::recompile(const QByteArray& query) noexcept
{
    return compile(query);
}

bool QsStatement::recompile(const QString& query) noexcept
{
    return compile(query);
}

QByteArray QsStatement::query() const
{
    Q_ASSERT_X(_statement != NULL, "query", "Statement is invalid");

    return QByteArray(sqlite3_sql(_statement));
}

QString QsStatement::query16() const
{
    Q_ASSERT_X(_statement != NULL, "query16", "Statement is invalid");

    return QString::fromUtf8(sqlite3_sql(_statement));
}

QsStatement::Type QsStatement::type() const noexcept
{
    if (_statement) {
        return sqlite3_stmt_readonly(_statement)
                ? Type::Select : Type::NonSelect;
    } else {
        return Type::Undefined;
    }
}

QsStatement& QsStatement::operator=(QsStatement&& statement) noexcept
{
    if (this != &statement) {
        // clear this
        clear();

        // move data from statement to this
        _statement = statement._statement;
        _db = statement._db;

        // reset statement
        statement.reset();
    }
    return *this;
}

bool QsStatement::compile(const QByteArray& query) noexcept
{
    return _db && sqlite3_prepare_v2(_db, query.constData(), query.length(),
                                     &_statement, NULL) == SQLITE_OK;
}

bool QsStatement::compile(const QString& query) noexcept
{
    return _db && sqlite3_prepare16_v2(_db, query.constData(),
                                       query.length() << 1,
                                       &_statement, NULL) == SQLITE_OK;
}
