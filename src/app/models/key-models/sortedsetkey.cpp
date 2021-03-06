#include "sortedsetkey.h"
#include "modules/redisclient/command.h"
#include "modules/redisclient/commandexecutor.h"

SortedSetKeyModel::SortedSetKeyModel(QSharedPointer<RedisClient::Connection> connection, QString fullPath, int dbIndex, int ttl)
    : KeyModel(connection, fullPath, dbIndex, ttl, true,
               "ZCARD", QString(), "ZRANGE WITHSCORES", true)
{    
}

QString SortedSetKeyModel::getType()
{
    return "zset";
}

QStringList SortedSetKeyModel::getColumnNames()
{
    return QStringList() << "row" << "value" << "score";
}

QHash<int, QByteArray> SortedSetKeyModel::getRoles()
{
    QHash<int, QByteArray> roles;
    roles[Roles::RowNumber] = "row";
    roles[Roles::Value] = "value";
    roles[Roles::Score] = "score";
    roles[Roles::BinaryValue] = "binary_value";
    return roles;
}

QVariant SortedSetKeyModel::getData(int rowIndex, int dataRole)
{
    if (!isRowLoaded(rowIndex))
        return QVariant();

    QPair<QByteArray, double> row = m_rowsCache[rowIndex];

    if (dataRole == Roles::Value)
        return row.first;
    else if (dataRole ==Roles::Score)
        return QString::number(row.second);
    else if (dataRole == Roles::RowNumber)
        return QString::number(rowIndex+1);
    else if (dataRole == Roles::BinaryValue)
        return valueToBinary(row.first);

    return QVariant();
}

void SortedSetKeyModel::updateRow(int rowIndex, const QVariantMap &row)
{
    if (!isRowLoaded(rowIndex) || !isRowValid(row))
        throw Exception("Invalid row");

    QPair<QByteArray, double> cachedRow = m_rowsCache[rowIndex];

    bool valueChanged = cachedRow.first != row["value"].toString();
    bool scoreChanged = cachedRow.second != row["score"].toDouble();

    QPair<QByteArray, double> newRow(
                    (valueChanged) ? row["value"].toByteArray() : cachedRow.first,
                    (scoreChanged) ? row["score"].toDouble() : cachedRow.second
                );

    // TODO (uglide): Update only score if value not changed

    deleteSortedSetRow(cachedRow.first);
    addSortedSetRow(newRow.first, newRow.second);
    m_rowsCache.replace(rowIndex, newRow);
}

void SortedSetKeyModel::addRow(const QVariantMap &row)
{
    if (!isRowValid(row))
        throw Exception("Invalid row");

    QPair<QByteArray, double> cachedRow(
                row["value"].toByteArray(),
                row["score"].toDouble());

    if (addSortedSetRow(cachedRow.first, cachedRow.second)) {
        m_rowsCache.push_back(cachedRow);
        m_rowCount++;
    }
}

void SortedSetKeyModel::removeRow(int i)
{
    if (!isRowLoaded(i))
        return;

    QByteArray value = m_rowsCache[i].first;

    using namespace RedisClient;

    Command deleteValues(QStringList() << "ZREM" << m_keyFullPath << value, m_dbIndex);
    try {
        CommandExecutor::execute(m_connection, deleteValues);
    } catch (const RedisClient::CommandExecutor::Exception& e) {
        throw Exception("Connection error: " + QString(e.what()));
    }

    m_rowCount--;
    m_rowsCache.removeAt(i);
    setRemovedIfEmpty();
}

bool SortedSetKeyModel::addSortedSetRow(const QByteArray &value, double score)
{
    using namespace RedisClient;
    Command addCmd(m_dbIndex);
    (addCmd << "ZADD" << m_keyFullPath << QString::number(score)).append(value);

    Response result;
    try {
        result = CommandExecutor::execute(m_connection, addCmd);
    } catch (const RedisClient::CommandExecutor::Exception& e) {
        throw Exception("Connection error: " + QString(e.what()));
    }

    return result.getValue().toInt() == 1;
}

void SortedSetKeyModel::deleteSortedSetRow(const QByteArray &value)
{
    using namespace RedisClient;
    Command addCmd(m_dbIndex);
    (addCmd << "ZREM" << m_keyFullPath).append(value);

    try {
        CommandExecutor::execute(m_connection, addCmd);
    } catch (const RedisClient::CommandExecutor::Exception& e) {
        throw Exception("Connection error: " + QString(e.what()));
    }
}

void SortedSetKeyModel::addLoadedRowsToCache(const QVariantList &rows, int rowStart)
{
    QList<QPair<QByteArray, double>> result;

    for (QVariantList::const_iterator item = rows.begin();
         item != rows.end(); ++item) {

        QPair<QByteArray, double> value;
        value.first = item->toByteArray();
        ++item;

        if (item == rows.end())
            throw Exception("Partial data loaded from server");

        value.second = item->toDouble();
        result.push_back(value);
    }

    m_rowsCache.addLoadedRange({rowStart, rowStart + result.size() - 1},
                               result);
}
