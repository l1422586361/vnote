#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSharedPointer>
#include <QBitArray>
#include <QSet>

#include "toolbarhelper.h"
#include "dockwidgethelper.h"
#include "statusbarhelper.h"

class QDockWidget;
class QSystemTrayIcon;
class QTimer;
class QLabel;

namespace vnotex
{
    class ToolBox;
    class NotebookExplorer;
    class ViewArea;
    class Event;
    class OutlineViewer;
    class LocationList;
    class SearchPanel;
    class SnippetPanel;
    class HistoryPanel;

    enum { RESTART_EXIT_CODE = 1000 };

    class MainWindow : public QMainWindow
    {
        Q_OBJECT
    public:
        friend class DockWidgetHelper;

        explicit MainWindow(QWidget *p_parent = nullptr);

        ~MainWindow();

        MainWindow(const MainWindow &) = delete;
        void operator=(const MainWindow &) = delete;

        void kickOffOnStart(const QStringList &p_paths);

        void resetStateAndGeometry();

        const QVector<QDockWidget *> &getDocks() const;

        void setContentAreaExpanded(bool p_expanded);
        // Should be called after MainWindow is shown.
        bool isContentAreaExpanded() const;

        void focusViewArea();

        void setStayOnTop(bool p_enabled);

        void restart();

        void showMainWindow();

        void quitApp();

        void openFiles(const QStringList &p_files);

        LocationList *getLocationList() const;

        void setLocationListVisible(bool p_visible);

        void toggleLocationListVisible();

        void updateDockWidgetTabBar();

    signals:
        void mainWindowStarted();

        // @m_response of @p_event: true to continue the close, false to stop the close.
        void mainWindowClosed(const QSharedPointer<Event> &p_event);

        // No user interaction is available.
        void mainWindowClosedOnQuit();

        void layoutChanged();

    protected:
        void closeEvent(QCloseEvent *p_event) Q_DECL_OVERRIDE;

        void changeEvent(QEvent *p_event) Q_DECL_OVERRIDE;

    private slots:
        void closeOnQuit();

        void exportNotes();

        void showTips(const QString &p_message, int p_timeoutMilliseconds);

    private:
        void setupUI();

        void setupCentralWidget();

        void setupOutlineViewer();

        void setupSearchPanel();

        void setupLocationList();

        void setupSnippetPanel();

        void setupHistoryPanel();

        void setupNotebookExplorer(QWidget *p_parent = nullptr);

        void setupDocks();

        void setupStatusBar();

        void setupTipsArea();

        void createTipsArea();

        void saveStateAndGeometry();

        void loadStateAndGeometry(bool p_stateOnly = false);

        // Used to test widget in development.
        void demoWidget();

        QString getViewAreaTitle() const;

        void setupToolBar();

        void setupShortcuts();

        void setupSystemTray();

        void setTipsAreaVisible(bool p_visible);

        void setupSpellCheck();

        void checkForUpdates();

        void checkNotebooksFailedToLoad();

        ToolBarHelper m_toolBarHelper;

        StatusBarHelper m_statusBarHelper;

        DockWidgetHelper m_dockWidgetHelper;

        ToolBox *m_navigationToolBox = nullptr;

        NotebookExplorer *m_notebookExplorer = nullptr;

        ViewArea *m_viewArea = nullptr;

        QWidget *m_viewAreaStatusWidget = nullptr;

        OutlineViewer *m_outlineViewer = nullptr;

        LocationList *m_locationList = nullptr;

        SearchPanel *m_searchPanel = nullptr;

        SnippetPanel *m_snippetPanel = nullptr;

        HistoryPanel *m_historyPanel = nullptr;

        bool m_layoutReset = false;

        // -1: do not request to quit;
        // 0 and above: exit code.
        int m_requestQuit = -1;

        Qt::WindowStates m_windowOldState = Qt::WindowMinimized;

        QSystemTrayIcon *m_trayIcon = nullptr;

        QLabel *m_tipsLabel = nullptr;

        QTimer *m_tipsTimer = nullptr;

        QStringList m_visibleDocksBeforeExpand;
    };
} // ns vnotex

#endif // MAINWINDOW_H
