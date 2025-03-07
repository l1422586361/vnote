#include "widgetconfig.h"

using namespace vnotex;

#define READINT(key) readInt(appObj, userObj, (key))
#define READBOOL(key) readBool(appObj, userObj, (key))
#define READSTRLIST(key) readStringList(appObj, userObj, (key))

WidgetConfig::WidgetConfig(ConfigMgr *p_mgr, IConfig *p_topConfig)
    : IConfig(p_mgr, p_topConfig)
{
    m_sessionName = QStringLiteral("widget");
}

void WidgetConfig::init(const QJsonObject &p_app,
                        const QJsonObject &p_user)
{
    const auto appObj = p_app.value(m_sessionName).toObject();
    const auto userObj = p_user.value(m_sessionName).toObject();

    {
        m_outlineAutoExpandedLevel = READINT(QStringLiteral("outline_auto_expanded_level"));
        if (m_outlineAutoExpandedLevel < 0 || m_outlineAutoExpandedLevel > 6) {
            m_outlineAutoExpandedLevel = 6;
        }

        m_outlineSectionNumberEnabled = READBOOL(QStringLiteral("outline_section_number_enabled"));
    }

    m_findAndReplaceOptions = static_cast<FindOptions>(READINT(QStringLiteral("find_and_replace_options")));

    {
        m_nodeExplorerViewOrder = READINT(QStringLiteral("node_explorer_view_order"));
        m_nodeExplorerRecycleBinNodeVisible = READBOOL(QStringLiteral("node_explorer_recycle_bin_node_visible"));
        m_nodeExplorerExternalFilesVisible = READBOOL(QStringLiteral("node_explorer_external_files_visible"));
        m_nodeExplorerAutoImportExternalFilesEnabled = READBOOL(QStringLiteral("node_explorer_auto_import_external_files_enabled"));
        m_nodeExplorerCloseBeforeOpenWithEnabled = READBOOL(QStringLiteral("node_explorer_close_before_open_with_enabled"));
    }

    m_searchPanelAdvancedSettingsVisible = READBOOL(QStringLiteral("search_panel_advanced_settings_visible"));

    m_mainWindowKeepDocksExpandingContentArea = READSTRLIST(QStringLiteral("main_window_keep_docks_expanding_content_area"));

    m_snippetPanelBuiltInSnippetsVisible = READBOOL(QStringLiteral("snippet_panel_builtin_snippets_visible"));
}

QJsonObject WidgetConfig::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("outline_auto_expanded_level")] = m_outlineAutoExpandedLevel;
    obj[QStringLiteral("outline_section_number_enabled")] = m_outlineSectionNumberEnabled;

    obj[QStringLiteral("find_and_replace_options")] = static_cast<int>(m_findAndReplaceOptions);

    obj[QStringLiteral("node_explorer_view_order")] = m_nodeExplorerViewOrder;
    obj[QStringLiteral("node_explorer_recycle_bin_node_visible")] = m_nodeExplorerRecycleBinNodeVisible;
    obj[QStringLiteral("node_explorer_external_files_visible")] = m_nodeExplorerExternalFilesVisible;
    obj[QStringLiteral("node_explorer_auto_import_external_files_enabled")] = m_nodeExplorerAutoImportExternalFilesEnabled;
    obj[QStringLiteral("node_explorer_close_before_open_with_enabled")] = m_nodeExplorerCloseBeforeOpenWithEnabled;

    obj[QStringLiteral("search_panel_advanced_settings_visible")] = m_searchPanelAdvancedSettingsVisible;
    obj[QStringLiteral("snippet_panel_builtin_snippets_visible")] = m_snippetPanelBuiltInSnippetsVisible;
    writeStringList(obj,
                    QStringLiteral("main_window_keep_docks_expanding_content_area"),
                    m_mainWindowKeepDocksExpandingContentArea);
    return obj;
}

int WidgetConfig::getOutlineAutoExpandedLevel() const
{
    return m_outlineAutoExpandedLevel;
}

void WidgetConfig::setOutlineAutoExpandedLevel(int p_level)
{
    updateConfig(m_outlineAutoExpandedLevel, p_level, this);
}

bool WidgetConfig::getOutlineSectionNumberEnabled() const
{
    return m_outlineSectionNumberEnabled;
}

void WidgetConfig::setOutlineSectionNumberEnabled(bool p_enabled)
{
    updateConfig(m_outlineSectionNumberEnabled, p_enabled, this);
}

FindOptions WidgetConfig::getFindAndReplaceOptions() const
{
    return m_findAndReplaceOptions;
}

void WidgetConfig::setFindAndReplaceOptions(FindOptions p_options)
{
    updateConfig(m_findAndReplaceOptions, p_options, this);
}

int WidgetConfig::getNodeExplorerViewOrder() const
{
    return m_nodeExplorerViewOrder;
}

void WidgetConfig::setNodeExplorerViewOrder(int p_viewOrder)
{
    updateConfig(m_nodeExplorerViewOrder, p_viewOrder, this);
}

bool WidgetConfig::isNodeExplorerRecycleBinNodeVisible() const
{
    return m_nodeExplorerRecycleBinNodeVisible;
}

void WidgetConfig::setNodeExplorerRecycleBinNodeVisible(bool p_visible)
{
    updateConfig(m_nodeExplorerRecycleBinNodeVisible, p_visible, this);
}

bool WidgetConfig::isNodeExplorerExternalFilesVisible() const
{
    return m_nodeExplorerExternalFilesVisible;
}

void WidgetConfig::setNodeExplorerExternalFilesVisible(bool p_visible)
{
    updateConfig(m_nodeExplorerExternalFilesVisible, p_visible, this);
}

bool WidgetConfig::getNodeExplorerAutoImportExternalFilesEnabled() const
{
    return m_nodeExplorerAutoImportExternalFilesEnabled;
}

void WidgetConfig::setNodeExplorerAutoImportExternalFilesEnabled(bool p_enabled)
{
    updateConfig(m_nodeExplorerAutoImportExternalFilesEnabled, p_enabled, this);
}

bool WidgetConfig::getNodeExplorerCloseBeforeOpenWithEnabled() const
{
    return m_nodeExplorerCloseBeforeOpenWithEnabled;
}

void WidgetConfig::setNodeExplorerCloseBeforeOpenWithEnabled(bool p_enabled)
{
    updateConfig(m_nodeExplorerCloseBeforeOpenWithEnabled, p_enabled, this);
}

bool WidgetConfig::isSearchPanelAdvancedSettingsVisible() const
{
    return m_searchPanelAdvancedSettingsVisible;
}

void WidgetConfig::setSearchPanelAdvancedSettingsVisible(bool p_visible)
{
    updateConfig(m_searchPanelAdvancedSettingsVisible, p_visible, this);
}

const QStringList &WidgetConfig::getMainWindowKeepDocksExpandingContentArea() const
{
    return m_mainWindowKeepDocksExpandingContentArea;
}

void WidgetConfig::setMainWindowKeepDocksExpandingContentArea(const QStringList &p_docks)
{
    updateConfig(m_mainWindowKeepDocksExpandingContentArea, p_docks, this);
}

bool WidgetConfig::isSnippetPanelBuiltInSnippetsVisible() const
{
    return m_snippetPanelBuiltInSnippetsVisible;
}

void WidgetConfig::setSnippetPanelBuiltInSnippetsVisible(bool p_visible)
{
    updateConfig(m_snippetPanelBuiltInSnippetsVisible, p_visible, this);
}

