#ifndef CORECONFIG_H
#define CORECONFIG_H

#include "iconfig.h"

#include <QtGlobal>
#include <QString>
#include <QStringList>

namespace vnotex
{
    class CoreConfig : public IConfig
    {
        Q_GADGET
    public:
        enum Shortcut
        {
            FullScreen,
            StayOnTop,
            ExpandContentArea,
            Settings,
            NewNote,
            CloseTab,
            CloseOtherTabs,
            CloseTabsToTheRight,
            NavigationDock,
            OutlineDock,
            SearchDock,
            SnippetDock,
            LocationListDock,
            HistoryDock,
            Search,
            NavigationMode,
            LocateNode,
            VerticalSplit,
            HorizontalSplit,
            MaximizeSplit,
            DistributeSplits,
            RemoveSplitAndWorkspace,
            NewWorkspace,
            Export,
            Quit,
            FlashPage,
            QuickAccess,
            ActivateTab1,
            ActivateTab2,
            ActivateTab3,
            ActivateTab4,
            ActivateTab5,
            ActivateTab6,
            ActivateTab7,
            ActivateTab8,
            ActivateTab9,
            AlternateTab,
            ActivateNextTab,
            ActivatePreviousTab,
            FocusContentArea,
            OpenWithDefaultProgram,
            OneSplitLeft,
            OneSplitDown,
            OneSplitUp,
            OneSplitRight,
            MoveOneSplitLeft,
            MoveOneSplitDown,
            MoveOneSplitUp,
            MoveOneSplitRight,
            OpenLastClosedFile,
            MaxShortcut
        };
        Q_ENUM(Shortcut)

        CoreConfig(ConfigMgr *p_mgr, IConfig *p_topConfig);

        void init(const QJsonObject &p_app, const QJsonObject &p_user) Q_DECL_OVERRIDE;

        QJsonObject toJson() const Q_DECL_OVERRIDE;

        const QString &getTheme() const;
        void setTheme(const QString &p_name);

        const QString &getLocale() const;
        void setLocale(const QString &p_locale);

        // Should be called after locale is properly set.
        QString getLocaleToUse() const;

        const QString &getShortcut(Shortcut p_shortcut) const;

        int getToolBarIconSize() const;
        void setToolBarIconSize(int p_size);

        int getDocksTabBarIconSize() const;
        void setDocksTabBarIconSize(int p_size);

        const QStringList &getExternalNodeExcludePatterns() const;

        static const QStringList &getAvailableLocales();

        bool isRecoverLastSessionOnStartEnabled() const;
        void setRecoverLastSessionOnStartEnabled(bool p_enabled);

        bool isCheckForUpdatesOnStartEnabled() const;
        void setCheckForUpdatesOnStartEnabled(bool p_enabled);

        int getHistoryMaxCount() const;

        bool isPerNotebookHistoryEnabled() const;
        void setPerNotebookHistoryEnabled(bool p_enabled);

    private:
        friend class MainConfig;

        void loadShortcuts(const QJsonObject &p_app, const QJsonObject &p_user);

        void loadNoteManagement(const QJsonObject &p_app, const QJsonObject &p_user);

        QJsonObject saveShortcuts() const;

        // Theme name.
        QString m_theme;

        // User-specified locale, such as zh_CN, en_US.
        // Empty if not specified.
        QString m_locale;

        QString m_shortcuts[Shortcut::MaxShortcut];

        // Icon size of MainWindow tool bar.
        int m_toolBarIconSize = 16;

        // Icon size of MainWindow QDockWidgets tab bar.
        int m_docksTabBarIconSize = 20;

        QStringList m_externalNodeExcludePatterns;

        // Whether recover last session on start.
        bool m_recoverLastSessionOnStartEnabled = true;

        bool m_checkForUpdatesOnStartEnabled = true;

        // Max count of the history items for each notebook and session config.
        int m_historyMaxCount = 100;

        // Whether store history in each notebook.
        bool m_perNotebookHistoryEnabled = true;

        static QStringList s_availableLocales;
    };
} // ns vnotex

#endif // CORECONFIG_H
