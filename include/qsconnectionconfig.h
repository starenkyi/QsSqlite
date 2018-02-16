#ifndef QS_CONNECTION_CONFIG_H
#define QS_CONNECTION_CONFIG_H

#include <QByteArray>
#include <QHash>
#include <QLocale>
#include <QString>

#include "qsconnection.h"

class QsConnectionConfig
{

public:

    friend bool operator ==(const QsConnectionConfig& lhs,
                            const QsConnectionConfig& rhs);

    enum ResultCode {
        Ok = 0,
        OpenConnError = 1,
        CreateCollationError = 2,
        CreateSchemaError = 4,
        ConfigureConnError = 8
    };

    QsConnectionConfig(const QByteArray& dbName = QByteArray()) Q_DECL_NOTHROW;

    QsConnectionConfig(const QsConnectionConfig& config) = default;

    QsConnectionConfig(QsConnectionConfig&& config) Q_DECL_NOTHROW = default;

    ~QsConnectionConfig() noexcept = default;

    void addUtf16Collator(const QByteArray& collationName,
                          const QLocale&    locale);

    QByteArray databaseName() const Q_DECL_NOTHROW;

    void deleteUtf16Collator(const QByteArray& collationName);

    QsConnection::CacheMode cacheMode() const noexcept;

    QByteArray configConnectionScript() const Q_DECL_NOTHROW;

    QByteArray createSchemaScript() const Q_DECL_NOTHROW;

    QByteArray lastError() const Q_DECL_NOTHROW;

    QString lastError16() const;

    QsConnection::OpenMode openMode() const noexcept;

    int  openAndConfig(QsConnection& connection);

    void setDatabaseName(const QByteArray& databaseName) Q_DECL_NOTHROW;

    void setCacheMode(QsConnection::CacheMode value) noexcept;

    void setConfigConnectionScript(const QByteArray& script) Q_DECL_NOTHROW;

    void setCreateSchemaScript(const QByteArray& script) Q_DECL_NOTHROW;

    void setOpenMode(QsConnection::OpenMode value) noexcept;

    void setThreadMode(QsConnection::ThreadMode value) noexcept;

    QsConnection::ThreadMode threadMode() const noexcept;

    QHash<QByteArray, QLocale> utf16Collators() const;

    QsConnectionConfig& operator =(const QsConnectionConfig& config) = default;

    QsConnectionConfig&
    operator=(QsConnectionConfig&& config) Q_DECL_NOTHROW = default;

private:

    QsConnection::ThreadMode _threadMode;
    QsConnection::OpenMode   _openMode;
    QsConnection::CacheMode  _cacheMode;

    QByteArray _databaseName;
    QByteArray _createSchemaScript;
    QByteArray _configConnectionScript;
    QByteArray _lastError;

    QHash<QByteArray, QLocale> _collatorLocales;

    QByteArrayList createCollations(QsConnection& connection) const;

    bool tryConfigureConnection(QsConnection& connection) const noexcept;

    bool tryOpen(QsConnection& connection) const;

    bool tryCreateSchema(QsConnection& connection) const noexcept;

};

#endif
