#ifndef QS_STATEMENT_H
#define QS_STATEMENT_H

#include <QByteArray>
#include <QChar>
#include <QPair>
#include <QString>

class  QsConnection;
struct sqlite3_stmt;
struct sqlite3;

class QsStatement
{

public:

    enum DataType {
        Integer = 0,
        Double,
        Text,
        Blob,
        Null
    };

    enum Type {
        NonSelect = 0,
        Select,
        Undefined
    };

    QsStatement() noexcept;

    QsStatement(const QsConnection& connection,
                const QByteArray&   query) noexcept;

    QsStatement(const QsConnection& connection,
                const QString&      query) noexcept;

    QsStatement(QsStatement&& statement) noexcept;

    ~QsStatement();

    bool bindBlob(int         index,
                  const void* value,
                  int         bytes) const noexcept;

    bool bindBlob(int               index,
                  const QByteArray& value) const noexcept;

    bool bindBlobCopy(int         index,
                      const void* value,
                      int         bytes) const noexcept;

    bool bindBlobCopy(int               index,
                      const QByteArray& value) const noexcept;

    bool bindBool(int  index,
                  bool value) const noexcept;

    int bindCount() const noexcept;

    bool bindDouble(int    index,
                    double value) const noexcept;

    bool bindInt(int index,
                 int value) const noexcept;

    bool bindInt64(int    index,
                   qint64 value) const noexcept;

    bool bindNull(int index) const noexcept;

    bool bindText(int         index,
                  const char* value,
                  int         length = -1) const noexcept;

    bool bindText16(int         index,
                    const void* value,
                    int         bytes = -1) const noexcept;

    bool bindText16(int            index,
                    const QString& value) const noexcept;

    bool bindText16Copy(int         index,
                        const void* value,
                        int         bytes = -1) const noexcept;

    bool bindText16Copy(int            index,
                        const QString& value) const noexcept;

    bool bindTextCopy(int         index,
                      const char* value,
                      int         bytes = -1) const noexcept;

    bool bindTextCopy(int            index,
                      const QString& value) const;

    unsigned byteLength(int index) const noexcept;

    unsigned byteLength16(int index) const noexcept;

    void clear() noexcept;

    void clearBindings() const noexcept;

    int columnCount() const noexcept;

    DataType columnType(int index) const noexcept;

    bool execute() const noexcept;

    QByteArray expandedQuery() const;

    QString expandedQuery16() const;

    QPair<const unsigned char*, int> getBlob(int index) const noexcept;

    QPair<unsigned char*, int> getBlobCopy(int index) const;

    bool getBool(int index) const noexcept;

    QByteArray getByteArray(int index) const;

    QPair<const char*, int> getCStr(int index) const noexcept;

    QPair<const QChar*, int> getCStr16(int index) const noexcept;

    QPair<char*, int> getCStrCopy(int index) const;

    QPair<QChar*, int> getCStr16Copy(int index) const;

    double getDouble(int index) const noexcept;

    int getInt(int index) const noexcept;

    qint64 getInt64(int index) const noexcept;

    QString getString(int index) const;

    QString getString16(int index) const;

    bool isNull(int index) const noexcept;

    inline bool isValid() const noexcept
    {
        return _statement != NULL;
    }

    QByteArray lastError() const;

    QString lastError16() const;

    int lastErrorCode() const noexcept;

    qint64 lastInsertRowId() const noexcept;

    bool next() const noexcept;

    bool recompile(const QByteArray& query) noexcept;

    bool recompile(const QString& query) noexcept;

    QByteArray query() const;

    QString query16() const;

    Type type() const noexcept;

    QsStatement& operator =(QsStatement&& statement) noexcept;

    QsStatement(const QsStatement& statement) = delete;
    QsStatement& operator =(const QsStatement& statement) = delete;

private:

    sqlite3_stmt* _statement;
    sqlite3*      _db;

    inline void reset() noexcept
    {
        _statement = NULL;
        _db = NULL;
    }

    bool compile(const QByteArray& query) noexcept;

    bool compile(const QString& query) noexcept;

};

#endif
