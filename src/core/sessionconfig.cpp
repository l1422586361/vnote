#include "sessionconfig.h"

#include <QDir>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>

#include <utils/fileutils.h>

#include "configmgr.h"
#include "mainconfig.h"
#include "historymgr.h"

using namespace vnotex;

bool SessionConfig::NotebookItem::operator==(const NotebookItem &p_other) const
{
    return m_type == p_other.m_type
           && m_rootFolderPath == p_other.m_rootFolderPath
           && m_backend == p_other.m_backend;
}

void SessionConfig::NotebookItem::fromJson(const QJsonObject &p_jobj)
{
    m_type = p_jobj[QStringLiteral("type")].toString();
    m_rootFolderPath = p_jobj[QStringLiteral("root_folder")].toString();
    m_backend = p_jobj[QStringLiteral("backend")].toString();
}

QJsonObject SessionConfig::NotebookItem::toJson() const
{
    QJsonObject jobj;

    jobj[QStringLiteral("type")] = m_type;
    jobj[QStringLiteral("root_folder")] = m_rootFolderPath;
    jobj[QStringLiteral("backend")] = m_backend;

    return jobj;
}

void SessionConfig::ExternalProgram::fromJson(const QJsonObject &p_jobj)
{
    m_name = p_jobj[QStringLiteral("name")].toString();
    m_command = p_jobj[QStringLiteral("command")].toString();
    m_shortcut = p_jobj[QStringLiteral("shortcut")].toString();
}

QJsonObject SessionConfig::ExternalProgram::toJson() const
{
    QJsonObject jobj;

    jobj[QStringLiteral("name")] = m_name;
    jobj[QStringLiteral("command")] = m_command;
    jobj[QStringLiteral("shortcut")] = m_shortcut;

    return jobj;
}

SessionConfig::SessionConfig(ConfigMgr *p_mgr)
    : IConfig(p_mgr, nullptr)
{
}

SessionConfig::~SessionConfig()
{

}

void SessionConfig::init()
{
    auto mgr = getMgr();
    auto sessionSettings = mgr->getSettings(ConfigMgr::Source::Session);
    const auto &sessionJobj = sessionSettings->getJson();

    loadCore(sessionJobj);

    loadStateAndGeometry(sessionJobj);

    loadExportOption(sessionJobj);

    m_searchOption.fromJson(sessionJobj[QStringLiteral("search_option")].toObject());

    m_viewAreaSession = readByteArray(sessionJobj, QStringLiteral("viewarea_session"));

    m_notebookExplorerSession = readByteArray(sessionJobj, QStringLiteral("notebook_explorer_session"));

    loadExternalPrograms(sessionJobj);

    loadNotebooks(sessionJobj);

    loadHistory(sessionJobj);

    if (MainConfig::isVersionChanged()) {
        doVersionSpecificOverride();
    }
}

void SessionConfig::loadCore(const QJsonObject &p_session)
{
    const auto coreObj = p_session.value(QStringLiteral("core")).toObject();
    m_newNotebookDefaultRootFolderPath = readString(coreObj,
        QStringLiteral("new_notebook_default_root_folder_path"));
    if (m_newNotebookDefaultRootFolderPath.isEmpty()) {
        m_newNotebookDefaultRootFolderPath = QDir::homePath();
    }

    m_currentNotebookRootFolderPath = readString(coreObj, QStringLiteral("current_notebook_root_folder_path"));

    {
        auto option = readString(coreObj, QStringLiteral("opengl"));
        m_openGL = stringToOpenGL(option);
    }

    if (!isUndefinedKey(coreObj, QStringLiteral("system_title_bar"))) {
        m_systemTitleBarEnabled = readBool(coreObj, QStringLiteral("system_title_bar"));
    } else {
        m_systemTitleBarEnabled = true;
    }

    if (!isUndefinedKey(coreObj, QStringLiteral("minimize_to_system_tray"))) {
        m_minimizeToSystemTray = readBool(coreObj, QStringLiteral("minimize_to_system_tray")) ? 1 : 0;
    }

    m_flashPage = readString(coreObj, QStringLiteral("flash_page"));

    m_quickAccessFiles = readStringList(coreObj, QStringLiteral("quick_access"));
}

QJsonObject SessionConfig::saveCore() const
{
    QJsonObject coreObj;
    coreObj[QStringLiteral("new_notebook_default_root_folder_path")] = m_newNotebookDefaultRootFolderPath;
    coreObj[QStringLiteral("current_notebook_root_folder_path")] = m_currentNotebookRootFolderPath;
    coreObj[QStringLiteral("opengl")] = openGLToString(m_openGL);
    coreObj[QStringLiteral("system_title_bar")] = m_systemTitleBarEnabled;
    if (m_minimizeToSystemTray != -1) {
        coreObj[QStringLiteral("minimize_to_system_tray")] = m_minimizeToSystemTray > 0;
    }
    coreObj[QStringLiteral("flash_page")] = m_flashPage;
    writeStringList(coreObj, QStringLiteral("quick_access"), m_quickAccessFiles);
    return coreObj;
}

const QString &SessionConfig::getNewNotebookDefaultRootFolderPath() const
{
    return m_newNotebookDefaultRootFolderPath;
}

void SessionConfig::setNewNotebookDefaultRootFolderPath(const QString &p_path)
{
    updateConfig(m_newNotebookDefaultRootFolderPath,
                 p_path,
                 this);
}

const QVector<SessionConfig::NotebookItem> &SessionConfig::getNotebooks() const
{
    return m_notebooks;
}

void SessionConfig::setNotebooks(const QVector<SessionConfig::NotebookItem> &p_notebooks)
{
    updateConfig(m_notebooks,
                 p_notebooks,
                 this);
}

void SessionConfig::loadNotebooks(const QJsonObject &p_session)
{
    const auto notebooksJson = p_session.value(QStringLiteral("notebooks")).toArray();
    m_notebooks.resize(notebooksJson.size());
    for (int i = 0; i < notebooksJson.size(); ++i) {
        m_notebooks[i].fromJson(notebooksJson[i].toObject());
    }
}

QJsonArray SessionConfig::saveNotebooks() const
{
    QJsonArray nbArray;
    for (const auto &nb : m_notebooks) {
        nbArray.append(nb.toJson());
    }
    return nbArray;
}

const QString &SessionConfig::getCurrentNotebookRootFolderPath() const
{
    return m_currentNotebookRootFolderPath;
}

void SessionConfig::setCurrentNotebookRootFolderPath(const QString &p_path)
{
    updateConfig(m_currentNotebookRootFolderPath,
                 p_path,
                 this);
}

void SessionConfig::writeToSettings() const
{
    getMgr()->writeSessionSettings(toJson());
}

QJsonObject SessionConfig::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("core")] = saveCore();
    obj[QStringLiteral("notebooks")] = saveNotebooks();
    obj[QStringLiteral("state_geometry")] = saveStateAndGeometry();
    obj[QStringLiteral("export")] = saveExportOption();
    obj[QStringLiteral("search_option")] = m_searchOption.toJson();
    writeByteArray(obj, QStringLiteral("viewarea_session"), m_viewAreaSession);
    writeByteArray(obj, QStringLiteral("notebook_explorer_session"), m_notebookExplorerSession);
    obj[QStringLiteral("external_programs")] = saveExternalPrograms();
    obj[QStringLiteral("history")] = saveHistory();
    return obj;
}

QJsonObject SessionConfig::saveStateAndGeometry() const
{
    QJsonObject obj;
    writeByteArray(obj, QStringLiteral("main_window_state"), m_mainWindowStateGeometry.m_mainState);
    writeByteArray(obj, QStringLiteral("main_window_geometry"), m_mainWindowStateGeometry.m_mainGeometry);
    writeStringList(obj, QStringLiteral("visible_docks_before_expand"), m_mainWindowStateGeometry.m_visibleDocksBeforeExpand);
    return obj;
}

SessionConfig::MainWindowStateGeometry SessionConfig::getMainWindowStateGeometry() const
{
    return m_mainWindowStateGeometry;
}

void SessionConfig::setMainWindowStateGeometry(const SessionConfig::MainWindowStateGeometry &p_state)
{
    updateConfig(m_mainWindowStateGeometry, p_state, this);
}

SessionConfig::OpenGL SessionConfig::getOpenGLAtBootstrap()
{
    auto userConfigFile = ConfigMgr::locateSessionConfigFilePathAtBootstrap();
    if (!userConfigFile.isEmpty()) {
        auto bytes = FileUtils::readFile(userConfigFile);
        auto obj = QJsonDocument::fromJson(bytes).object();
        auto coreObj = obj.value(QStringLiteral("core")).toObject();
        auto str = coreObj.value(QStringLiteral("opengl")).toString();
        return stringToOpenGL(str);
    }

    return OpenGL::None;
}

SessionConfig::OpenGL SessionConfig::getOpenGL() const
{
    return m_openGL;
}

void SessionConfig::setOpenGL(OpenGL p_option)
{
    updateConfig(m_openGL, p_option, this);
}

QString SessionConfig::openGLToString(OpenGL p_option)
{
    switch (p_option) {
    case OpenGL::Desktop:
        return QStringLiteral("desktop");

    case OpenGL::Angle:
        return QStringLiteral("angle");

    case OpenGL::Software:
        return QStringLiteral("software");

    default:
        return QStringLiteral("none");
    }
}

SessionConfig::OpenGL SessionConfig::stringToOpenGL(const QString &p_str)
{
    auto option = p_str.toLower();
    if (option == QStringLiteral("software")) {
        return OpenGL::Software;
    } else if (option == QStringLiteral("desktop")) {
        return OpenGL::Desktop;
    } else if (option == QStringLiteral("angle")) {
        return OpenGL::Angle;
    } else {
        return OpenGL::None;
    }
}

bool SessionConfig::getSystemTitleBarEnabled() const
{
    return m_systemTitleBarEnabled;
}

void SessionConfig::setSystemTitleBarEnabled(bool p_enabled)
{
    updateConfig(m_systemTitleBarEnabled, p_enabled, this);
}

int SessionConfig::getMinimizeToSystemTray() const
{
    return m_minimizeToSystemTray;
}

void SessionConfig::setMinimizeToSystemTray(bool p_enabled)
{
    updateConfig(m_minimizeToSystemTray, p_enabled ? 1 : 0, this);
}

void SessionConfig::doVersionSpecificOverride()
{
    // In a new version, we may want to change one value by force.
    // SHOULD set the in memory variable only, or will override the notebook list.
}

const ExportOption &SessionConfig::getExportOption() const
{
    return m_exportOption;
}

void SessionConfig::setExportOption(const ExportOption &p_option)
{
    updateConfig(m_exportOption, p_option, this);
}

const QVector<ExportCustomOption> &SessionConfig::getCustomExportOptions() const
{
    return m_customExportOptions;
}

void SessionConfig::setCustomExportOptions(const QVector<ExportCustomOption> &p_options)
{
    updateConfig(m_customExportOptions, p_options, this);
}

const SearchOption &SessionConfig::getSearchOption() const
{
    return m_searchOption;
}

void SessionConfig::setSearchOption(const SearchOption &p_option)
{
    updateConfig(m_searchOption, p_option, this);
}

void SessionConfig::loadStateAndGeometry(const QJsonObject &p_session)
{
    const auto obj = p_session.value(QStringLiteral("state_geometry")).toObject();
    m_mainWindowStateGeometry.m_mainState = readByteArray(obj, QStringLiteral("main_window_state"));
    m_mainWindowStateGeometry.m_mainGeometry = readByteArray(obj, QStringLiteral("main_window_geometry"));
    m_mainWindowStateGeometry.m_visibleDocksBeforeExpand = readStringList(obj, QStringLiteral("visible_docks_before_expand"));
}

QByteArray SessionConfig::getViewAreaSessionAndClear()
{
    QByteArray bytes;
    m_viewAreaSession.swap(bytes);
    return bytes;
}

void SessionConfig::setViewAreaSession(const QByteArray &p_bytes)
{
    updateConfigWithoutCheck(m_viewAreaSession, p_bytes, this);
}

QByteArray SessionConfig::getNotebookExplorerSessionAndClear()
{
    QByteArray bytes;
    m_notebookExplorerSession.swap(bytes);
    return bytes;
}

void SessionConfig::setNotebookExplorerSession(const QByteArray &p_bytes)
{
    updateConfigWithoutCheck(m_notebookExplorerSession, p_bytes, this);
}

const QString &SessionConfig::getFlashPage() const
{
    return m_flashPage;
}

void SessionConfig::setFlashPage(const QString &p_file)
{
    updateConfig(m_flashPage, p_file, this);
}

const QStringList &SessionConfig::getQuickAccessFiles() const
{
    return m_quickAccessFiles;
}

void SessionConfig::setQuickAccessFiles(const QStringList &p_files)
{
    updateConfig(m_quickAccessFiles, p_files, this);
}

void SessionConfig::loadExternalPrograms(const QJsonObject &p_session)
{
    const auto arr = p_session.value(QStringLiteral("external_programs")).toArray();
    m_externalPrograms.resize(arr.size());
    for (int i = 0; i < arr.size(); ++i) {
        m_externalPrograms[i].fromJson(arr[i].toObject());
    }
}

QJsonArray SessionConfig::saveExternalPrograms() const
{
    QJsonArray arr;
    for (const auto &pro : m_externalPrograms) {
        arr.append(pro.toJson());
    }
    return arr;
}

const QVector<SessionConfig::ExternalProgram> &SessionConfig::getExternalPrograms() const
{
    return m_externalPrograms;
}

const QVector<HistoryItem> &SessionConfig::getHistory() const
{
    return m_history;
}

void SessionConfig::addHistory(const HistoryItem &p_item)
{
    HistoryMgr::insertHistoryItem(m_history, p_item);
    update();
}

void SessionConfig::clearHistory()
{
    m_history.clear();
    update();
}

void SessionConfig::loadHistory(const QJsonObject &p_session)
{
    auto arr = p_session[QStringLiteral("history")].toArray();
    m_history.resize(arr.size());
    for (int i = 0; i < arr.size(); ++i) {
        m_history[i].fromJson(arr[i].toObject());
    }
}

QJsonArray SessionConfig::saveHistory() const
{
    QJsonArray arr;
    for (const auto &item : m_history) {
        arr.append(item.toJson());
    }
    return arr;
}

void SessionConfig::loadExportOption(const QJsonObject &p_session)
{
    auto exportObj = p_session[QStringLiteral("export")].toObject();

    m_exportOption.fromJson(exportObj[QStringLiteral("export_option")].toObject());

    auto customArr = exportObj[QStringLiteral("custom_options")].toArray();
    m_customExportOptions.resize(customArr.size());
    for (int i = 0; i < customArr.size(); ++i) {
        m_customExportOptions[i].fromJson(customArr[i].toObject());
    }
}

QJsonObject SessionConfig::saveExportOption() const
{
    QJsonObject obj;

    obj[QStringLiteral("export_option")] = m_exportOption.toJson();

    QJsonArray customArr;
    for (int i = 0; i < m_customExportOptions.size(); ++i) {
        customArr.push_back(m_customExportOptions[i].toJson());
    }
    obj[QStringLiteral("custom_options")] = customArr;

    return obj;
}
