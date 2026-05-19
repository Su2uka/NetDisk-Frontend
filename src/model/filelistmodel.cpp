#include "filelistmodel.h"

FileListModel::FileListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

// 获取当前列表长度（是 QAbstractListModel 要求必须重写的核心接口）
int FileListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_items.count();
}

// 核心数据提供接口：QML 每次渲染或访问一个属性时，都会传入目标行数（index）及需要的角色（role）。此函数负责将内部 C++ 数据精准地装箱转发出去
QVariant FileListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.count())
        return QVariant();

    const FileItem &item = m_items[index.row()];

    switch (role) {
    case FileIdRole:      return item.fileId;
    case FileNameRole:    return item.fileName;
    case FileIconRole:    return item.fileIcon;
    case IsFolderRole:    return item.isFolder;
    case IsSelectedRole:  return item.isSelected;  // 这是控制界面单列勾选状态的关键分发口
    case CreateTimeRole:  return item.createTime;
    case ModifyTimeRole:  return item.modifyTime;
    case FileSizeRole:    return item.fileSize;
    case FileSizeStrRole: return item.fileSizeStr;
    case ParentIdRole:    return item.parentId;
    }

    return QVariant();
}

// 角色映射表：在此定义 C++ 枚举 Role 与 QML 字符串名的绑定。
// 比如设定 "fileName"，在 QML 的 ListView/GridView 内部中就可以直接写 model.fileName 。
QHash<int, QByteArray> FileListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[FileIdRole] = "fileId";
    roles[FileNameRole] = "fileName";
    roles[FileIconRole] = "fileIcon";
    roles[IsFolderRole] = "isFolder";
    roles[IsSelectedRole] = "selected"; // 这里顺应原 QML 中 property 的习惯映射为了 "selected"
    roles[CreateTimeRole] = "createTime";
    roles[ModifyTimeRole] = "modifyTime";
    roles[FileSizeRole] = "fileSize";
    roles[FileSizeStrRole] = "fileSizeStr";
    roles[ParentIdRole] = "parentId";
    return roles;
}

// 重新加载并更新数据（全量刷新数据环境）
void FileListModel::loadData(const QVariantList &data)
{
    // 利用 Qt 模型机制通知视图：模型即将重置（令 ListView 移除或者重建全部的组件）
    beginResetModel();
    m_items.clear();

    // 结构化解析：从纯粹传入的动态变体列表反序装载为 C++ 底层强类型的结构体提升访问性能
    for (const QVariant &var : data) {
        QVariantMap map = var.toMap();
        FileItem item;
        item.fileId = map["fileId"].toString();
        item.fileName = map["fileName"].toString();
        item.fileIcon = map["fileIcon"].toString();
        item.isFolder = map["isFolder"].toBool();
        item.isSelected = map["selected"].toBool();
        item.fileSize = map["fileSize"].toLongLong();
        item.createTime = map["createTime"].toString();
        item.modifyTime = map["modifyTime"].toString();
        item.fileSizeStr = map["fileSizeStr"].toString();
        item.parentId = map["parentId"].toString();
        
        m_items.append(item);
    }
    // 通知视图模型已经完成装填
    endResetModel();
    
    // 通知外部监听者同步刷新计算状态
    emit selectionChanged();
    emit countChanged();
}

// 增量追加数据（无限滚动加载下一页时使用，不重置现有列表）
void FileListModel::appendData(const QVariantList &data)
{
    if (data.isEmpty())
        return;

    int first = m_items.count();
    int last  = first + data.count() - 1;

    beginInsertRows(QModelIndex(), first, last);

    for (const QVariant &var : data) {
        QVariantMap map = var.toMap();
        FileItem item;
        item.fileId      = map["fileId"].toString();
        item.fileName    = map["fileName"].toString();
        item.fileIcon    = map["fileIcon"].toString();
        item.isFolder    = map["isFolder"].toBool();
        item.isSelected  = map["selected"].toBool();
        item.fileSize    = map["fileSize"].toLongLong();
        item.createTime  = map["createTime"].toString();
        item.modifyTime  = map["modifyTime"].toString();
        item.fileSizeStr = map["fileSizeStr"].toString();
        item.parentId    = map["parentId"].toString();

        m_items.append(item);
    }

    endInsertRows();
    emit countChanged();
}

// 反转单行的选中状态 (多选框点击时触发)
void FileListModel::toggleSelection(int row)
{
    if (row < 0 || row >= m_items.count())
        return;

    m_items[row].isSelected = !m_items[row].isSelected;
    
    // 【高性能数据驱动核心实现】：这里发出 dataChanged 局部重绘信号。
    // 这可以让 QML 引擎极其精准地追踪，并且『只重新渲染』指定行的选中状态和色块，无需全列表销毁并创建重排。此方法在数万行数据中同样极速且低开销。
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {IsSelectedRole});
    
    emit selectionChanged();
}

// 选中全部文件
void FileListModel::selectAll()
{
    for (int i = 0; i < m_items.count(); ++i) {
        m_items[i].isSelected = true;
    }
    
    if (!m_items.isEmpty()) {
        // [0, length-1] 的局部刷新信号跨越全长，通知视图引擎这些行的 IsSelectedRole 都发生了变化
        emit dataChanged(index(0, 0), index(m_items.count() - 1, 0), {IsSelectedRole});
        emit selectionChanged();
    }
}

// 取消选中所有文件
void FileListModel::clearSelection()
{
    for (int i = 0; i < m_items.count(); ++i) {
        m_items[i].isSelected = false;
    }
    
    if (!m_items.isEmpty()) {
        // 同样下发区间渲染指令
        emit dataChanged(index(0, 0), index(m_items.count() - 1, 0), {IsSelectedRole});
        emit selectionChanged();
    }
}

// 计算当前列表中共有多少文件被选中
int FileListModel::selectedCount() const
{
    int count = 0;
    for (const auto &item : m_items) {
        if (item.isSelected) {
            count++;
        }
    }
    return count;
}

// 轻量级便利判断函数：是否已有选中的文件存在于模型当中
bool FileListModel::hasSelection() const
{
    return selectedCount() > 0;
}

QVariantMap FileListModel::getFileInfo(int index) const
{
    QVariantMap info;
    if (index < 0 || index >= m_items.count())
        return info;

    const FileItem &item = m_items[index];
    info["fileId"] = item.fileId;
    info["fileName"] = item.fileName;
    info["fileIcon"] = item.fileIcon;
    info["isFolder"] = item.isFolder;
    info["isSelected"] = item.isSelected;
    info["createTime"] = item.createTime;
    info["modifyTime"] = item.modifyTime;
    info["fileSize"] = item.fileSize;
    info["fileSizeStr"] = item.fileSizeStr;
    info["parentId"] = item.parentId;
    return info;
}

void FileListModel::removeFileById(const QString &fileId)
{
    for (int i = 0; i < m_items.count(); ++i) {
        if (m_items[i].fileId == fileId) {
            beginRemoveRows(QModelIndex(), i, i);
            m_items.removeAt(i);
            endRemoveRows();

            emit selectionChanged();
            emit countChanged();
            return;
        }
    }
}
