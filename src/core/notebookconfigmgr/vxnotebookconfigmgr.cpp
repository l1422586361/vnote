#include "vxnotebookconfigmgr.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

#include <notebookbackend/inotebookbackend.h>
#include <notebook/notebookparameters.h>
#include <notebook/vxnode.h>
#include <notebook/externalnode.h>
#include <notebook/bundlenotebook.h>
#include <utils/utils.h>
#include <utils/fileutils.h>
#include <utils/pathutils.h>
#include <exception.h>
#include <core/configmgr.h>
#include <core/editorconfig.h>
#include <core/coreconfig.h>

#include <utils/contentmediautils.h>

using namespace vnotex;

const QString VXNotebookConfigMgr::NodeConfig::c_version = "version";

const QString VXNotebookConfigMgr::NodeConfig::c_id = "id";

const QString VXNotebookConfigMgr::NodeConfig::c_createdTimeUtc = "created_time";

const QString VXNotebookConfigMgr::NodeConfig::c_files = "files";

const QString VXNotebookConfigMgr::NodeConfig::c_folders = "folders";

const QString VXNotebookConfigMgr::NodeConfig::c_name = "name";

const QString VXNotebookConfigMgr::NodeConfig::c_modifiedTimeUtc = "modified_time";

const QString VXNotebookConfigMgr::NodeConfig::c_attachmentFolder = "attachment_folder";

const QString VXNotebookConfigMgr::NodeConfig::c_tags = "tags";

QJsonObject VXNotebookConfigMgr::NodeFileConfig::toJson() const
{
    QJsonObject jobj;

    jobj[NodeConfig::c_name] = m_name;
    jobj[NodeConfig::c_id] = QString::number(m_id);
    jobj[NodeConfig::c_createdTimeUtc] = Utils::dateTimeStringUniform(m_createdTimeUtc);
    jobj[NodeConfig::c_modifiedTimeUtc] = Utils::dateTimeStringUniform(m_modifiedTimeUtc);
    jobj[NodeConfig::c_attachmentFolder] = m_attachmentFolder;
    jobj[NodeConfig::c_tags] = QJsonArray::fromStringList(m_tags);

    return jobj;
}

void VXNotebookConfigMgr::NodeFileConfig::fromJson(const QJsonObject &p_jobj)
{
    m_name = p_jobj[NodeConfig::c_name].toString();

    {
        auto idStr = p_jobj[NodeConfig::c_id].toString();
        bool ok;
        m_id = idStr.toULongLong(&ok);
        if (!ok) {
            m_id = Node::InvalidId;
        }
    }

    m_createdTimeUtc = Utils::dateTimeFromStringUniform(p_jobj[NodeConfig::c_createdTimeUtc].toString());
    m_modifiedTimeUtc = Utils::dateTimeFromStringUniform(p_jobj[NodeConfig::c_modifiedTimeUtc].toString());

    m_attachmentFolder = p_jobj[NodeConfig::c_attachmentFolder].toString();

    {
        auto arr = p_jobj[NodeConfig::c_tags].toArray();
        for (int i = 0; i < arr.size(); ++i) {
            m_tags << arr[i].toString();
        }
    }
}

QJsonObject VXNotebookConfigMgr::NodeFolderConfig::toJson() const
{
    QJsonObject jobj;

    jobj[NodeConfig::c_name] = m_name;

    return jobj;
}

void VXNotebookConfigMgr::NodeFolderConfig::fromJson(const QJsonObject &p_jobj)
{
    m_name = p_jobj[NodeConfig::c_name].toString();
}

VXNotebookConfigMgr::NodeConfig::NodeConfig()
{
}

VXNotebookConfigMgr::NodeConfig::NodeConfig(const QString &p_version,
                                            ID p_id,
                                            const QDateTime &p_createdTimeUtc,
                                            const QDateTime &p_modifiedTimeUtc)
    : m_version(p_version),
      m_id(p_id),
      m_createdTimeUtc(p_createdTimeUtc),
      m_modifiedTimeUtc(p_modifiedTimeUtc)
{
}

QJsonObject VXNotebookConfigMgr::NodeConfig::toJson() const
{
    QJsonObject jobj;

    jobj[NodeConfig::c_version] = m_version;
    jobj[NodeConfig::c_id] = QString::number(m_id);
    jobj[NodeConfig::c_createdTimeUtc] = Utils::dateTimeStringUniform(m_createdTimeUtc);
    jobj[NodeConfig::c_modifiedTimeUtc] = Utils::dateTimeStringUniform(m_modifiedTimeUtc);

    QJsonArray files;
    for (const auto &file : m_files) {
        files.append(file.toJson());
    }
    jobj[NodeConfig::c_files] = files;

    QJsonArray folders;
    for (const auto& folder : m_folders) {
        folders.append(folder.toJson());
    }
    jobj[NodeConfig::c_folders] = folders;

    return jobj;
}

void VXNotebookConfigMgr::NodeConfig::fromJson(const QJsonObject &p_jobj)
{
    m_version = p_jobj[NodeConfig::c_version].toString();

    {
    auto idStr = p_jobj[NodeConfig::c_id].toString();
    bool ok;
    m_id = idStr.toULongLong(&ok);
    if (!ok) {
        m_id = Node::InvalidId;
    }
    }

    m_createdTimeUtc = Utils::dateTimeFromStringUniform(p_jobj[NodeConfig::c_createdTimeUtc].toString());
    m_modifiedTimeUtc = Utils::dateTimeFromStringUniform(p_jobj[NodeConfig::c_modifiedTimeUtc].toString());

    auto filesJson = p_jobj[NodeConfig::c_files].toArray();
    m_files.resize(filesJson.size());
    for (int i = 0; i < filesJson.size(); ++i) {
        m_files[i].fromJson(filesJson[i].toObject());
    }

    auto foldersJson = p_jobj[NodeConfig::c_folders].toArray();
    m_folders.resize(foldersJson.size());
    for (int i = 0; i < foldersJson.size(); ++i) {
        m_folders[i].fromJson(foldersJson[i].toObject());
    }
}


const QString VXNotebookConfigMgr::c_nodeConfigName = "vx.json";

const QString VXNotebookConfigMgr::c_recycleBinFolderName = "vx_recycle_bin";

bool VXNotebookConfigMgr::s_initialized = false;

QVector<QRegExp> VXNotebookConfigMgr::s_externalNodeExcludePatterns;

VXNotebookConfigMgr::VXNotebookConfigMgr(const QString &p_name,
                                         const QString &p_displayName,
                                         const QString &p_description,
                                         const QSharedPointer<INotebookBackend> &p_backend,
                                         QObject *p_parent)
    : BundleNotebookConfigMgr(p_backend, p_parent),
      m_info(p_name, p_displayName, p_description)
{
    if (!s_initialized) {
        s_initialized = true;

        const auto &patterns = ConfigMgr::getInst().getCoreConfig().getExternalNodeExcludePatterns();
        s_externalNodeExcludePatterns.reserve(patterns.size());
        for (const auto &pat : patterns) {
            if (!pat.isEmpty()) {
                s_externalNodeExcludePatterns.push_back(QRegExp(pat, Qt::CaseInsensitive, QRegExp::Wildcard));
            }
        }
    }
}

QString VXNotebookConfigMgr::getName() const
{
    return m_info.m_name;
}

QString VXNotebookConfigMgr::getDisplayName() const
{
    return m_info.m_displayName;
}

QString VXNotebookConfigMgr::getDescription() const
{
    return m_info.m_description;
}

void VXNotebookConfigMgr::createEmptySkeleton(const NotebookParameters &p_paras)
{
    BundleNotebookConfigMgr::createEmptySkeleton(p_paras);

    createEmptyRootNode();
}

void VXNotebookConfigMgr::createEmptyRootNode()
{
    auto currentTime = QDateTime::currentDateTimeUtc();
    NodeConfig node(getCodeVersion(),
                    BundleNotebookConfigMgr::RootNodeId,
                    currentTime,
                    currentTime);
    writeNodeConfig(c_nodeConfigName, node);
}

QSharedPointer<Node> VXNotebookConfigMgr::loadRootNode()
{
    auto nodeConfig = readNodeConfig("");
    QSharedPointer<Node> root = nodeConfigToNode(*nodeConfig, "", nullptr);
    root->setUse(Node::Use::Root);
    root->setExists(true);
    Q_ASSERT(root->isLoaded());

    if (!markRecycleBinNode(root)) {
        const_cast<VXNotebookConfigMgr *>(this)->createRecycleBinNode(root);
    }

    return root;
}

bool VXNotebookConfigMgr::markRecycleBinNode(const QSharedPointer<Node> &p_root)
{
    auto node = p_root->findChild(c_recycleBinFolderName,
                                  FileUtils::isPlatformNameCaseSensitive());
    if (node) {
        if (!node->exists()) {
            removeNode(node, true, true);
            return false;
        }

        node->setUse(Node::Use::RecycleBin);
        markNodeReadOnly(node.data());
        return true;
    }

    return false;
}

void VXNotebookConfigMgr::markNodeReadOnly(Node *p_node) const
{
    if (p_node->isReadOnly()) {
        return;
    }

    p_node->setReadOnly(true);
    for (const auto &child : p_node->getChildrenRef()) {
        markNodeReadOnly(child.data());
    }
}

void VXNotebookConfigMgr::createRecycleBinNode(const QSharedPointer<Node> &p_root)
{
    Q_ASSERT(p_root->isRoot());

    auto node = newNode(p_root.data(), Node::Flag::Container, c_recycleBinFolderName, "");
    node->setUse(Node::Use::RecycleBin);
    markNodeReadOnly(node.data());
}

QSharedPointer<VXNotebookConfigMgr::NodeConfig> VXNotebookConfigMgr::readNodeConfig(const QString &p_path) const
{
    auto backend = getBackend();
    if (!backend->exists(p_path)) {
        Exception::throwOne(Exception::Type::InvalidArgument,
                            QString("node path (%1) does not exist").arg(p_path));
    }

    if (backend->isFile(p_path)) {
        Exception::throwOne(Exception::Type::InvalidArgument,
                            QString("node (%1) is a file node without config").arg(p_path));
    } else {
        auto configPath = PathUtils::concatenateFilePath(p_path, c_nodeConfigName);
        auto data = backend->readFile(configPath);
        auto nodeConfig = QSharedPointer<NodeConfig>::create();
        nodeConfig->fromJson(QJsonDocument::fromJson(data).object());
        return nodeConfig;
    }

    return nullptr;
}

QString VXNotebookConfigMgr::getNodeConfigFilePath(const Node *p_node) const
{
    Q_ASSERT(p_node->isContainer());
    return PathUtils::concatenateFilePath(p_node->fetchPath(), c_nodeConfigName);
}

void VXNotebookConfigMgr::writeNodeConfig(const QString &p_path, const NodeConfig &p_config) const
{
    getBackend()->writeFile(p_path, p_config.toJson());
}

void VXNotebookConfigMgr::writeNodeConfig(const Node *p_node)
{
    auto config = nodeToNodeConfig(p_node);
    writeNodeConfig(getNodeConfigFilePath(p_node), *config);
}

QSharedPointer<Node> VXNotebookConfigMgr::nodeConfigToNode(const NodeConfig &p_config,
                                                           const QString &p_name,
                                                           Node *p_parent) const
{
    auto node = QSharedPointer<VXNode>::create(p_name, getNotebook(), p_parent);
    loadFolderNode(node.data(), p_config);
    return node;
}

void VXNotebookConfigMgr::loadFolderNode(Node *p_node, const NodeConfig &p_config) const
{
    QVector<QSharedPointer<Node>> children;
    children.reserve(p_config.m_files.size() + p_config.m_folders.size());
    const auto basePath = p_node->fetchPath();

    for (const auto &folder : p_config.m_folders) {
        if (folder.m_name.isEmpty()) {
            // Skip empty name node.
            qWarning() << "skipped loading node with empty name under" << p_node->fetchPath();
            continue;
        }

        auto folderNode = QSharedPointer<VXNode>::create(folder.m_name,
                                                         getNotebook(),
                                                         p_node);
        inheritNodeFlags(p_node, folderNode.data());
        folderNode->setExists(getBackend()->existsDir(PathUtils::concatenateFilePath(basePath, folder.m_name)));
        children.push_back(folderNode);
    }

    for (const auto &file : p_config.m_files) {
        if (file.m_name.isEmpty()) {
            // Skip empty name node.
            qWarning() << "skipped loading node with empty name under" << p_node->fetchPath();
            continue;
        }

        auto fileNode = QSharedPointer<VXNode>::create(file.m_id,
                                                       file.m_name,
                                                       file.m_createdTimeUtc,
                                                       file.m_modifiedTimeUtc,
                                                       file.m_tags,
                                                       file.m_attachmentFolder,
                                                       getNotebook(),
                                                       p_node);
        inheritNodeFlags(p_node, fileNode.data());
        fileNode->setExists(getBackend()->existsFile(PathUtils::concatenateFilePath(basePath, file.m_name)));
        children.push_back(fileNode);
    }

    p_node->loadCompleteInfo(p_config.m_id,
                             p_config.m_createdTimeUtc,
                             p_config.m_modifiedTimeUtc,
                             QStringList(),
                             children);
}

QSharedPointer<Node> VXNotebookConfigMgr::newNode(Node *p_parent,
                                                  Node::Flags p_flags,
                                                  const QString &p_name,
                                                  const QString &p_content)
{
    Q_ASSERT(p_parent && p_parent->isContainer() && !p_name.isEmpty());

    QSharedPointer<Node> node;

    if (p_flags & Node::Flag::Content) {
        Q_ASSERT(!(p_flags & Node::Flag::Container));
        node = newFileNode(p_parent, p_name, p_content, true, NodeParameters());
    } else {
        node = newFolderNode(p_parent, p_name, true, NodeParameters());
    }

    return node;
}

QSharedPointer<Node> VXNotebookConfigMgr::addAsNode(Node *p_parent,
                                                    Node::Flags p_flags,
                                                    const QString &p_name,
                                                    const NodeParameters &p_paras)
{
    Q_ASSERT(p_parent && p_parent->isContainer());

    // TODO: reuse the config if available.
    QSharedPointer<Node> node;
    if (p_flags & Node::Flag::Content) {
        Q_ASSERT(!(p_flags & Node::Flag::Container));
        node = newFileNode(p_parent, p_name, "", false, p_paras);
    } else {
        node = newFolderNode(p_parent, p_name, false, p_paras);
    }

    return node;
}

QSharedPointer<Node> VXNotebookConfigMgr::copyAsNode(Node *p_parent,
                                                     Node::Flags p_flags,
                                                     const QString &p_path)
{
    Q_ASSERT(p_parent && p_parent->isContainer());

    QSharedPointer<Node> node;
    if (p_flags & Node::Flag::Content) {
        Q_ASSERT(!(p_flags & Node::Flag::Container));
        node = copyFileAsChildOf(p_path, p_parent);
    } else {
        node = copyFolderAsChildOf(p_path, p_parent);
    }

    return node;
}

QSharedPointer<Node> VXNotebookConfigMgr::newFileNode(Node *p_parent,
                                                      const QString &p_name,
                                                      const QString &p_content,
                                                      bool p_create,
                                                      const NodeParameters &p_paras)
{
    auto notebook = getNotebook();

    // Create file node.
    auto node = QSharedPointer<VXNode>::create(Node::InvalidId,
                                               p_name,
                                               p_paras.m_createdTimeUtc,
                                               p_paras.m_modifiedTimeUtc,
                                               p_paras.m_tags,
                                               p_paras.m_attachmentFolder,
                                               notebook,
                                               p_parent);

    // Write empty file.
    if (p_create) {
        getBackend()->writeFile(node->fetchPath(), p_content);
        node->setExists(true);
    } else {
        node->setExists(getBackend()->existsFile(node->fetchPath()));
    }

    addChildNode(p_parent, node);
    writeNodeConfig(p_parent);

    return node;
}

QSharedPointer<Node> VXNotebookConfigMgr::newFolderNode(Node *p_parent,
                                                        const QString &p_name,
                                                        bool p_create,
                                                        const NodeParameters &p_paras)
{
    auto notebook = getNotebook();

    // Create folder node.
    auto node = QSharedPointer<VXNode>::create(p_name, notebook, p_parent);
    node->loadCompleteInfo(Node::InvalidId,
                           p_paras.m_createdTimeUtc,
                           p_paras.m_modifiedTimeUtc,
                           QStringList(),
                           QVector<QSharedPointer<Node>>());

    // Make folder.
    if (p_create) {
        getBackend()->makePath(node->fetchPath());
        node->setExists(true);
    } else {
        node->setExists(getBackend()->existsDir(node->fetchPath()));
    }

    writeNodeConfig(node.data());

    addChildNode(p_parent, node);
    writeNodeConfig(p_parent);

    return node;
}

QSharedPointer<VXNotebookConfigMgr::NodeConfig> VXNotebookConfigMgr::nodeToNodeConfig(const Node *p_node) const
{
    Q_ASSERT(p_node->isContainer());

    auto config = QSharedPointer<NodeConfig>::create(getCodeVersion(),
                                                     p_node->getId(),
                                                     p_node->getCreatedTimeUtc(),
                                                     p_node->getModifiedTimeUtc());

    for (const auto &child : p_node->getChildrenRef()) {
        if (child->hasContent()) {
            NodeFileConfig fileConfig;
            fileConfig.m_name = child->getName();
            fileConfig.m_id = child->getId();
            fileConfig.m_createdTimeUtc = child->getCreatedTimeUtc();
            fileConfig.m_modifiedTimeUtc = child->getModifiedTimeUtc();
            fileConfig.m_attachmentFolder = child->getAttachmentFolder();
            fileConfig.m_tags = child->getTags();

            config->m_files.push_back(fileConfig);
        } else {
            Q_ASSERT(child->isContainer());
            NodeFolderConfig folderConfig;
            folderConfig.m_name = child->getName();

            config->m_folders.push_back(folderConfig);
        }
    }

    return config;
}

void VXNotebookConfigMgr::loadNode(Node *p_node) const
{
    if (p_node->isLoaded() || !p_node->exists()) {
        return;
    }

    auto config = readNodeConfig(p_node->fetchPath());
    Q_ASSERT(p_node->isContainer());
    loadFolderNode(p_node, *config);
}

void VXNotebookConfigMgr::saveNode(const Node *p_node)
{
    if (p_node->isContainer()) {
        writeNodeConfig(p_node);
    } else {
        Q_ASSERT(!p_node->isRoot());
        writeNodeConfig(p_node->getParent());
    }
}

void VXNotebookConfigMgr::renameNode(Node *p_node, const QString &p_name)
{
    Q_ASSERT(!p_node->isRoot());
    if (p_node->isContainer()) {
        getBackend()->renameDir(p_node->fetchPath(), p_name);
    } else {
        getBackend()->renameFile(p_node->fetchPath(), p_name);
    }

    p_node->setName(p_name);
    writeNodeConfig(p_node->getParent());
}

void VXNotebookConfigMgr::addChildNode(Node *p_parent, const QSharedPointer<Node> &p_child) const
{
    if (p_child->isContainer()) {
        int idx = 0;
        const auto &children = p_parent->getChildrenRef();
        for (; idx < children.size(); ++idx) {
            if (!children[idx]->isContainer()) {
                break;
            }
        }

        p_parent->insertChild(idx, p_child);
    } else {
        p_parent->addChild(p_child);
    }

    inheritNodeFlags(p_parent, p_child.data());
}

QSharedPointer<Node> VXNotebookConfigMgr::loadNodeByPath(const QSharedPointer<Node> &p_root, const QString &p_relativePath)
{
    auto p = PathUtils::cleanPath(p_relativePath);
    auto paths = p.split('/', QString::SkipEmptyParts);
    auto node = p_root;
    for (auto &pa : paths) {
        // Find child @pa in @node.
        if (!node->isLoaded()) {
            loadNode(node.data());
        }

        auto child = node->findChild(pa, FileUtils::isPlatformNameCaseSensitive());
        if (!child) {
            return nullptr;
        }

        node = child;
    }

    return node;
}

// @p_src may belong to different notebook or different kind of configmgr.
// TODO: we could constrain @p_src within the same configrmgr?
QSharedPointer<Node> VXNotebookConfigMgr::copyNodeAsChildOf(const QSharedPointer<Node> &p_src,
                                                            Node *p_dest,
                                                            bool p_move)
{
    Q_ASSERT(p_dest->isContainer());

    if (!p_src->exists()) {
        if (p_move) {
            p_src->getNotebook()->removeNode(p_src);
        }
        return nullptr;
    }

    QSharedPointer<Node> node;
    if (p_src->isContainer()) {
        node = copyFolderNodeAsChildOf(p_src, p_dest, p_move);
    } else {
        node = copyFileNodeAsChildOf(p_src, p_dest, p_move);
    }

    return node;
}

QSharedPointer<Node> VXNotebookConfigMgr::copyFileNodeAsChildOf(const QSharedPointer<Node> &p_src,
                                                                Node *p_dest,
                                                                bool p_move)
{
    // Copy source file itself.
    auto srcFilePath = p_src->fetchAbsolutePath();
    auto destFilePath = PathUtils::concatenateFilePath(p_dest->fetchPath(),
                                                       PathUtils::fileName(srcFilePath));
    destFilePath = getBackend()->renameIfExistsCaseInsensitive(destFilePath);
    getBackend()->copyFile(srcFilePath, destFilePath);

    // Copy media files fetched from content.
    ContentMediaUtils::copyMediaFiles(p_src.data(), getBackend().data(), destFilePath);

    // Copy attachment folder. Rename attachment folder if conflicts.
    QString attachmentFolder = p_src->getAttachmentFolder();
    if (!attachmentFolder.isEmpty()) {
        auto destAttachmentFolderPath = fetchNodeAttachmentFolder(destFilePath, attachmentFolder);
        ContentMediaUtils::copyAttachment(p_src.data(), getBackend().data(), destFilePath, destAttachmentFolderPath);
    }

    // Create a file node.
    auto notebook = getNotebook();
    auto id = p_src->getId();
    if (!p_move || p_src->getNotebook() != notebook) {
        // Use a new id.
        id = notebook->getAndUpdateNextNodeId();
    }

    auto destNode = QSharedPointer<VXNode>::create(id,
                                                   PathUtils::fileName(destFilePath),
                                                   p_src->getCreatedTimeUtc(),
                                                   p_src->getModifiedTimeUtc(),
                                                   p_src->getTags(),
                                                   attachmentFolder,
                                                   notebook,
                                                   p_dest);
    destNode->setExists(true);
    addChildNode(p_dest, destNode);
    writeNodeConfig(p_dest);

    if (p_move) {
        // Delete src node.
        p_src->getNotebook()->removeNode(p_src);
    }

    return destNode;
}

QSharedPointer<Node> VXNotebookConfigMgr::copyFolderNodeAsChildOf(const QSharedPointer<Node> &p_src,
                                                                  Node *p_dest,
                                                                  bool p_move)
{
    auto srcFolderPath = p_src->fetchAbsolutePath();
    auto destFolderPath = PathUtils::concatenateFilePath(p_dest->fetchPath(),
                                                         PathUtils::fileName(srcFolderPath));
    destFolderPath = getBackend()->renameIfExistsCaseInsensitive(destFolderPath);

    // Make folder.
    getBackend()->makePath(destFolderPath);

    // Create a folder node.
    auto notebook = getNotebook();
    auto id = p_src->getId();
    if (!p_move || p_src->getNotebook() != notebook) {
        // Use a new id.
        id = notebook->getAndUpdateNextNodeId();
    }
    auto destNode = QSharedPointer<VXNode>::create(PathUtils::fileName(destFolderPath),
                                                   notebook,
                                                   p_dest);
    destNode->loadCompleteInfo(id,
                               p_src->getCreatedTimeUtc(),
                               p_src->getModifiedTimeUtc(),
                               QStringList(),
                               QVector<QSharedPointer<Node>>());
    destNode->setExists(true);

    writeNodeConfig(destNode.data());

    addChildNode(p_dest, destNode);
    writeNodeConfig(p_dest);

    // Copy children node.
    auto children = p_src->getChildren();
    for (const auto &childNode : children) {
        copyNodeAsChildOf(childNode, destNode.data(), p_move);
    }

    if (p_move) {
        p_src->getNotebook()->removeNode(p_src);
    }

    return destNode;
}

void VXNotebookConfigMgr::removeNode(const QSharedPointer<Node> &p_node, bool p_force, bool p_configOnly)
{
    auto parentNode = p_node->getParent();
    if (!p_configOnly && p_node->exists()) {
        // Remove all children.
        auto children = p_node->getChildren();
        for (const auto &childNode : children) {
            removeNode(childNode, p_force, p_configOnly);
        }

        try {
            removeFilesOfNode(p_node.data(), p_force);
        } catch (Exception &p_e) {
            qWarning() << "failed to remove files of node" << p_node->fetchPath() << p_e.what();
        }
    }

    if (parentNode) {
        parentNode->removeChild(p_node);
        writeNodeConfig(parentNode);
    }
}

void VXNotebookConfigMgr::removeFilesOfNode(Node *p_node, bool p_force)
{
    Q_ASSERT(p_node->getNotebook() == getNotebook());
    if (!p_node->isContainer()) {
        // Delete attachment.
        if (!p_node->getAttachmentFolder().isEmpty()) {
            getBackend()->removeDir(p_node->fetchAttachmentFolderPath());
        }

        // Delete media files fetched from content.
        ContentMediaUtils::removeMediaFiles(p_node);

        // Delete node file itself.
        auto filePath = p_node->fetchPath();
        getBackend()->removeFile(filePath);
    } else {
        Q_ASSERT(p_node->getChildrenCount() == 0);
        // Delete node config file and the dir if it is empty.
        auto configFilePath = getNodeConfigFilePath(p_node);
        getBackend()->removeFile(configFilePath);
        auto folderPath = p_node->fetchPath();
        if (p_force) {
            getBackend()->removeDir(folderPath);
        } else {
            getBackend()->removeEmptyDir(folderPath);
            bool deleted = getBackend()->removeDirIfEmpty(folderPath);
            if (!deleted) {
                qWarning() << "folder is not deleted since it is not empty" << folderPath;
            }
        }
    }
}

QString VXNotebookConfigMgr::fetchNodeImageFolderPath(Node *p_node)
{
    auto pa = PathUtils::concatenateFilePath(PathUtils::parentDirPath(p_node->fetchAbsolutePath()),
                                             getNotebook()->getImageFolder());
    // Do not make the folder when it is a folder node request.
    if (p_node->hasContent()) {
        getBackend()->makePath(pa);
    }
    return pa;
}

QString VXNotebookConfigMgr::fetchNodeAttachmentFolderPath(Node *p_node)
{
    auto notebookFolder = PathUtils::concatenateFilePath(PathUtils::parentDirPath(p_node->fetchAbsolutePath()),
                                                         getNotebook()->getAttachmentFolder());
    if (p_node->hasContent()) {
        auto nodeFolder = p_node->getAttachmentFolder();
        if (nodeFolder.isEmpty()) {
            auto folderPath = fetchNodeAttachmentFolder(p_node->fetchAbsolutePath(), nodeFolder);
            p_node->setAttachmentFolder(nodeFolder);
            saveNode(p_node);

            getBackend()->makePath(folderPath);
            return folderPath;
        } else {
            return PathUtils::concatenateFilePath(notebookFolder, nodeFolder);
        }
    } else {
        // Do not make the folder when it is a folder node request.
        return notebookFolder;
    }
}

QString VXNotebookConfigMgr::fetchNodeAttachmentFolder(const QString &p_nodePath, QString &p_folderName)
{
    auto notebookFolder = PathUtils::concatenateFilePath(PathUtils::parentDirPath(p_nodePath),
                                                         getNotebook()->getAttachmentFolder());
    if (p_folderName.isEmpty()) {
        p_folderName = FileUtils::generateUniqueFileName(notebookFolder, QString(), QString());
    } else if (FileUtils::childExistsCaseInsensitive(notebookFolder, p_folderName)) {
        p_folderName = FileUtils::generateFileNameWithSequence(notebookFolder, p_folderName, QString());
    }
    return PathUtils::concatenateFilePath(notebookFolder, p_folderName);
}

bool VXNotebookConfigMgr::isBuiltInFile(const Node *p_node, const QString &p_name) const
{
    static const QString backupFileExtension = ConfigMgr::getInst().getEditorConfig().getBackupFileExtension().toLower();

    const auto name = p_name.toLower();
    if (name == c_nodeConfigName
        || name == QStringLiteral("_vnote.json")
        || name.endsWith(backupFileExtension)) {
        return true;
    }
    return BundleNotebookConfigMgr::isBuiltInFile(p_node, p_name);
}

bool VXNotebookConfigMgr::isBuiltInFolder(const Node *p_node, const QString &p_name) const
{
    const auto name = p_name.toLower();
    if (name == c_recycleBinFolderName
        || name == getNotebook()->getImageFolder().toLower()
        || name == getNotebook()->getAttachmentFolder().toLower()
        || name == QStringLiteral("_v_images")
        || name == QStringLiteral("_v_attachments")) {
        return true;
    }
    return BundleNotebookConfigMgr::isBuiltInFolder(p_node, p_name);
}

QSharedPointer<Node> VXNotebookConfigMgr::copyFileAsChildOf(const QString &p_srcPath, Node *p_dest)
{
    // Skip copy if it already locates in dest folder.
    auto destFilePath = PathUtils::concatenateFilePath(p_dest->fetchAbsolutePath(),
                                                       PathUtils::fileName(p_srcPath));
    if (!PathUtils::areSamePaths(p_srcPath, destFilePath)) {
        // Copy source file itself.
        destFilePath = getBackend()->renameIfExistsCaseInsensitive(destFilePath);
        getBackend()->copyFile(p_srcPath, destFilePath);

        // Copy media files fetched from content.
        ContentMediaUtils::copyMediaFiles(p_srcPath, getBackend().data(), destFilePath);
    }

    const auto name = PathUtils::fileName(destFilePath);
    auto destNode = p_dest->findChild(name, true);
    if (destNode) {
        // Already have the node.
        return destNode;
    }

    // Create a file node.
    auto currentTime = QDateTime::currentDateTimeUtc();
    destNode = QSharedPointer<VXNode>::create(getNotebook()->getAndUpdateNextNodeId(),
                                              name,
                                              currentTime,
                                              currentTime,
                                              QStringList(),
                                              QString(),
                                              getNotebook(),
                                              p_dest);
    destNode->setExists(true);
    addChildNode(p_dest, destNode);
    writeNodeConfig(p_dest);

    return destNode;
}

QSharedPointer<Node> VXNotebookConfigMgr::copyFolderAsChildOf(const QString &p_srcPath, Node *p_dest)
{
    // Skip copy if it already locates in dest folder.
    auto destFolderPath = PathUtils::concatenateFilePath(p_dest->fetchAbsolutePath(),
                                                         PathUtils::fileName(p_srcPath));
    if (!PathUtils::areSamePaths(p_srcPath, destFolderPath)) {
        destFolderPath = getBackend()->renameIfExistsCaseInsensitive(destFolderPath);

        // Copy folder.
        getBackend()->copyDir(p_srcPath, destFolderPath);
    }

    const auto name = PathUtils::fileName(destFolderPath);
    auto destNode = p_dest->findChild(name, true);
    if (destNode) {
        // Already have the node.
        return destNode;
    }

    // Create a folder node.
    auto notebook = getNotebook();
    destNode = QSharedPointer<VXNode>::create(name, notebook, p_dest);
    auto currentTime = QDateTime::currentDateTimeUtc();
    destNode->loadCompleteInfo(notebook->getAndUpdateNextNodeId(),
                               currentTime,
                               currentTime,
                               QStringList(),
                               QVector<QSharedPointer<Node>>());
    destNode->setExists(true);

    writeNodeConfig(destNode.data());

    addChildNode(p_dest, destNode);
    writeNodeConfig(p_dest);

    return destNode;
}

void VXNotebookConfigMgr::inheritNodeFlags(const Node *p_node, Node *p_child) const
{
    if (p_node->isReadOnly()) {
        markNodeReadOnly(p_child);
    }
}

QVector<QSharedPointer<ExternalNode>> VXNotebookConfigMgr::fetchExternalChildren(Node *p_node) const
{
    Q_ASSERT(p_node->isContainer());
    QVector<QSharedPointer<ExternalNode>> externalNodes;

    auto dir = p_node->toDir();

    // Folders.
    {
        const auto folders = dir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
        for (const auto &folder : folders) {
            if (isBuiltInFolder(p_node, folder)) {
                continue;
            }

            if (isExcludedFromExternalNode(folder)) {
                continue;
            }

            if (p_node->containsContainerChild(folder)) {
                continue;
            }

            externalNodes.push_back(QSharedPointer<ExternalNode>::create(p_node, folder, ExternalNode::Type::Folder));
        }
    }

    // Files.
    {
        const auto files = dir.entryList(QDir::Files);
        for (const auto &file : files) {
            if (isBuiltInFile(p_node, file)) {
                continue;
            }

            if (isExcludedFromExternalNode(file)) {
                continue;
            }

            if (p_node->containsContentChild(file)) {
                continue;
            }

            externalNodes.push_back(QSharedPointer<ExternalNode>::create(p_node, file, ExternalNode::Type::File));
        }
    }

    return externalNodes;
}

bool VXNotebookConfigMgr::isExcludedFromExternalNode(const QString &p_name) const
{
    for (const auto &regExp : s_externalNodeExcludePatterns) {
        if (regExp.exactMatch(p_name)) {
            return true;
        }
    }
    return false;
}

bool VXNotebookConfigMgr::checkNodeExists(Node *p_node)
{
    bool exists = getBackend()->exists(p_node->fetchPath());
    p_node->setExists(exists);
    return exists;
}

QStringList VXNotebookConfigMgr::scanAndImportExternalFiles(Node *p_node)
{
    QStringList files;
    if (!p_node->isContainer() || p_node->getUse() == Node::Use::RecycleBin) {
        return files;
    }

    // External nodes.
    auto dir = p_node->toDir();
    auto externalNodes = fetchExternalChildren(p_node);
    for (const auto &node : externalNodes) {
        Node::Flags flags = Node::Flag::Content;
        if (node->isFolder()) {
            if (isLikelyImageFolder(dir.filePath(node->getName()))) {
                qWarning() << "skip importing folder containing only images" << node->getName();
                continue;
            }
            flags = Node::Flag::Container;
        }

        addAsNode(p_node, flags, node->getName(), NodeParameters());
        files << dir.filePath(node->getName());
    }

    // Children folders (including newly-added external nodes).
    for (const auto &child : p_node->getChildrenRef()) {
        if (child->isContainer()) {
            files << scanAndImportExternalFiles(child.data());
        }
    }

    return files;
}

bool VXNotebookConfigMgr::isLikelyImageFolder(const QString &p_dirPath)
{
    QDir dir(p_dirPath);
    const auto folders = dir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    if (!folders.isEmpty()) {
        return false;
    }

    const auto files = dir.entryList(QDir::Files);
    if (files.isEmpty()) {
        return false;
    }

    for (const auto &file : files) {
        if (!FileUtils::isImage(dir.filePath(file))) {
            return false;
        }
    }

    return true;
}
