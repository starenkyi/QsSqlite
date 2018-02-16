#include "../include/qsconnectionconfig.h"

#include <QByteArrayList>

#include "qshelper.h"


bool operator ==(const QsConnectionConfig& lhs,
                 const QsConnectionConfig& rhs)
{
    return lhs._threadMode == lhs._threadMode
            && lhs._openMode == rhs._openMode
            && lhs._cacheMode == rhs._cacheMode
            && lhs._databaseName == rhs._databaseName
            && lhs._createSchemaScript == rhs._createSchemaScript
            && lhs._configConnectionScript == rhs._configConnectionScript
            && lhs._collatorLocales == rhs._collatorLocales;
}

QsConnectionConfig::QsConnectionConfig(const QByteArray& dbName) Q_DECL_NOTHROW
    : _threadMode {QsConnection::defaultThreadMode},
      _openMode {QsConnection::defaultOpenMode},
      _cacheMode {QsConnection::defaultCacheMode},
      _databaseName {dbName}
{}

void QsConnectionConfig::addUtf16Collator(const QByteArray& collationName,
                                          const QLocale&    locale)
{
    _collatorLocales.insert(collationName, locale);
}

QByteArray QsConnectionConfig::databaseName() const Q_DECL_NOTHROW
{
    return _databaseName;
}

void QsConnectionConfig::deleteUtf16Collator(const QByteArray& collationName)
{
    _collatorLocales.remove(collationName);
}

QsConnection::CacheMode QsConnectionConfig::cacheMode() const noexcept
{
    return _cacheMode;
}

QByteArray QsConnectionConfig::configConnectionScript() const Q_DECL_NOTHROW
{
    return _configConnectionScript;
}

QByteArray QsConnectionConfig::createSchemaScript() const Q_DECL_NOTHROW
{
    return _createSchemaScript;
}

QByteArray QsConnectionConfig::lastError() const Q_DECL_NOTHROW
{
    return _lastError;
}

QString QsConnectionConfig::lastError16() const
{
    return QString::fromUtf8(_lastError);
}

QsConnection::OpenMode QsConnectionConfig::openMode() const noexcept
{
    return _openMode;
}

int QsConnectionConfig::openAndConfig(QsConnection& connection)
{
    int result = Ok;
    QByteArrayList errors;

    // try construct and open connection (save error on fail)
    if (!tryOpen(connection)) {
        errors.append(qs::buildConnErrMsg(
                          "Error on open connection", connection));
        result = OpenConnError;
    } else {
        // try add collations for locales (save errors on fail)
        errors.append(createCollations(connection));
        if (!errors.isEmpty()) {
            result = CreateCollationError;
        }

        // try create schema if needed (save error on fail);
        // if create schema success, try execute script for
        // connection configuration (save error on fail)
        if (!tryCreateSchema(connection)) {
            errors.append(qs::buildConnErrMsg(
                              "Error on create database schema", connection));
            result |= CreateSchemaError;
        } else if (!tryConfigureConnection(connection)) {
            errors.append(qs::buildConnErrMsg(
                              "Error on configure connection", connection));
            result |= ConfigureConnError;
        }
    }

    // clear last error on success (otherwise, save new errors)
    if (result == Ok) {
        _lastError.clear();
    } else {
        _lastError = errors.join(' ');
    }

    // return result flags
    return result;
}

void QsConnectionConfig::setDatabaseName(
        const QByteArray& databaseName) Q_DECL_NOTHROW
{
    _databaseName = databaseName;
}

void
QsConnectionConfig::setCacheMode(const QsConnection::CacheMode value) noexcept
{
    _cacheMode = value;
}

void QsConnectionConfig::setConfigConnectionScript(
        const QByteArray& script) Q_DECL_NOTHROW
{
    _configConnectionScript = script;
}

void QsConnectionConfig::setCreateSchemaScript(
        const QByteArray& script) Q_DECL_NOTHROW
{
    _createSchemaScript = script;
}

void
QsConnectionConfig::setOpenMode(const QsConnection::OpenMode value) noexcept
{
    _openMode = value;
}

void
QsConnectionConfig::setThreadMode(const QsConnection::ThreadMode value) noexcept
{
    _threadMode = value;
}

QsConnection::ThreadMode QsConnectionConfig::threadMode() const noexcept
{
    return _threadMode;
}

QHash<QByteArray, QLocale> QsConnectionConfig::utf16Collators() const
{
    return _collatorLocales;
}

bool QsConnectionConfig::tryConfigureConnection(
        QsConnection& connection) const noexcept
{
    // try configure connection and return result
    return _configConnectionScript.isEmpty()
            || connection.execute(_configConnectionScript);
}

QByteArrayList
QsConnectionConfig::createCollations(QsConnection& connection) const
{
    QByteArrayList errorList;

    // try create collation and save errors to list
    // (if create some collations failed)
    for (auto it = _collatorLocales.cbegin(),
         end = _collatorLocales.cend(); it != end; it++) {
        if (!connection.createUtf16Collation(it.key(), it.value())) {
            // build error string and append it to list
            QByteArray error("Error on add collation \'");
            error.append(it.key()).append('\'');
            errorList.append(qs::buildConnErrMsg(
                                 error.constData(), connection));
        }
    }

    // return result string list
    return errorList;
}

bool
QsConnectionConfig::tryCreateSchema(QsConnection& connection) const noexcept
{
    // check if script to create database schema is not empty
    if (!_createSchemaScript.isEmpty()) {
        // read info from sqlite_master
        const std::pair<qint64, int> dbInfo = connection.readInt64(
                    QByteArrayLiteral("select count(*) from sqlite_master"));

        // try create database schema if not exists and return result
        return dbInfo.second == QsConnection::ReadSuccess
                && (dbInfo.first == 0
                    || connection.execute(_createSchemaScript));
    }

    return true;
}

bool QsConnectionConfig::tryOpen(QsConnection& connection) const
{
    // close db (if opened) and set database name,
    connection.close();
    connection.setDatabaseName(_databaseName);

    // try open connection and return result
    return connection.open(_openMode, _threadMode, _cacheMode);
}
