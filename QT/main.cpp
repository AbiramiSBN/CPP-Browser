#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QWebEngineView>
#include <QToolBar>
#include <QLineEdit>
#include <QAction>
#include <QUrl>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QWebEngineProfile>
#include <QWebEngineDownloadItem>
#include <QFileDialog>
#include <QFileInfo>
#include <QList>
#include <QPair>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineSettings>
#include <QWebEngineHistory>
#include <QPalette>
#include <QColor>

// Custom interceptor to add dark mode preference header
class DarkModeInterceptor : public QWebEngineUrlRequestInterceptor {
public:
    DarkModeInterceptor(QObject* parent = nullptr)
        : QWebEngineUrlRequestInterceptor(parent) {}
    void interceptRequest(QWebEngineUrlRequestInfo &info) override {
        // This header tells websites that the user prefers dark mode
        info.setHttpHeader("Sec-CH-Prefers-Color-Scheme", "dark");
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        // Create a tab widget for tabbed browsing.
        tabWidget = new QTabWidget(this);
        tabWidget->setTabsClosable(true);
        setCentralWidget(tabWidget);
        connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
        connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::updateUrlBar);

        // Create a navigation toolbar.
        QToolBar *navBar = addToolBar("Navigation");

        QAction *backAction = navBar->addAction("Back");
        connect(backAction, &QAction::triggered, this, [this](bool){ goBack(); });

        QAction *forwardAction = navBar->addAction("Forward");
        connect(forwardAction, &QAction::triggered, this, [this](bool){ goForward(); });

        QAction *reloadAction = navBar->addAction("Reload");
        connect(reloadAction, &QAction::triggered, this, [this](bool){ reloadPage(); });

        QAction *newTabAction = navBar->addAction("New Tab");
        connect(newTabAction, &QAction::triggered, this, [this](bool){ addNewTab(); });

        urlBar = new QLineEdit(this);
        urlBar->setPlaceholderText("Enter URL");
        navBar->addWidget(urlBar);
        connect(urlBar, &QLineEdit::returnPressed, this, &MainWindow::navigateToUrl);

        QAction *devToolsAction = navBar->addAction("Dev Tools");
        connect(devToolsAction, &QAction::triggered, this, [this](bool){ openDevTools(); });

        // File menu for opening PDFs.
        QMenu *fileMenu = menuBar()->addMenu("File");
        QAction *openPdfAction = fileMenu->addAction("Open PDF");
        connect(openPdfAction, &QAction::triggered, this, &MainWindow::openPdfFile);

        // Menu for bookmarks.
        QMenu *bookmarksMenu = menuBar()->addMenu("Bookmarks");
        bookmarksMenu->setObjectName("Bookmarks");
        QAction *addBookmarkAction = bookmarksMenu->addAction("Add Bookmark");
        connect(addBookmarkAction, &QAction::triggered, this, &MainWindow::addBookmark);
        // Initialize the bookmarks list with some defaults.
        bookmarksList.append(qMakePair(QString("Google"), QUrl("https://google.com")));
        bookmarksList.append(qMakePair(QString("YouTube"), QUrl("https://youtube.com")));
        updateBookmarksMenu(bookmarksMenu);

        // Menu for history.
        QMenu *historyMenu = menuBar()->addMenu("History");
        QAction *showHistoryAction = historyMenu->addAction("Show History");
        connect(showHistoryAction, &QAction::triggered, this, &MainWindow::showHistory);

        // Add the initial tab.
        addNewTab(QUrl("https://google.com"));
        
        // Connect download signal from the default profile.
        connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested,
                this, &MainWindow::handleDownload);
    }

private slots:
    // Adds a new tab with the given URL (default is Google).
    void addNewTab(const QUrl &url = QUrl("https://google.com")) {
        QWebEngineView *view = new QWebEngineView;
        view->load(url);
        int index = tabWidget->addTab(view, url.toString());
        tabWidget->setCurrentIndex(index);
        connect(view, &QWebEngineView::urlChanged, this, &MainWindow::updateUrlBar);
    }

    void closeTab(int index) {
        if (tabWidget->count() > 1) {
            QWidget *tab = tabWidget->widget(index);
            tabWidget->removeTab(index);
            tab->deleteLater();
        }
    }

    void navigateToUrl() {
        QWebEngineView *view = currentWebView();
        if (view) {
            QUrl url = QUrl::fromUserInput(urlBar->text());
            view->load(url);
        }
    }

    void updateUrlBar() {
        QWebEngineView *view = currentWebView();
        if (view) {
            urlBar->setText(view->url().toString());
            tabWidget->setTabText(tabWidget->currentIndex(), view->title());
        }
    }

    void goBack() {
        QWebEngineView *view = currentWebView();
        if (view)
            view->back();
    }

    void goForward() {
        QWebEngineView *view = currentWebView();
        if (view)
            view->forward();
    }

    void reloadPage() {
        QWebEngineView *view = currentWebView();
        if (view)
            view->reload();
    }

    void openDevTools() {
        QWebEngineView *view = currentWebView();
        if (!view) return;
        QWebEngineView *devTools = new QWebEngineView;
        view->page()->setDevToolsPage(devTools->page());
        devTools->resize(800,600);
        devTools->show();
    }

    // Adds the current page to the bookmarks list.
    void addBookmark() {
        QWebEngineView *view = currentWebView();
        if (!view) return;
        QString title = view->title();
        QUrl url = view->url();
        bookmarksList.append(qMakePair(title, url));
        QMenu *bookmarksMenu = menuBar()->findChild<QMenu*>("Bookmarks");
        if (bookmarksMenu) {
            updateBookmarksMenu(bookmarksMenu);
        }
    }

    void bookmarkTriggered() {
        QAction *action = qobject_cast<QAction*>(sender());
        if (action) {
            QUrl url = action->data().toUrl();
            addNewTab(url);
        }
    }

    // Shows the browsing history of the current tab in a simple message box.
    void showHistory() {
        QWebEngineView *view = currentWebView();
        if (!view) return;
        QString historyStr;
        const auto historyItems = view->history()->items();
        for (const auto &item : historyItems) {
            historyStr += item.title() + "\n" + item.url().toString() + "\n\n";
        }
        QMessageBox::information(this, "History", historyStr.isEmpty() ? "No history" : historyStr);
    }

    // Handles downloads by asking the user where to save the file.
    void handleDownload(QWebEngineDownloadItem* download) {
        QString defaultPath = download->downloadDirectory() + "/" + download->downloadFileName();
        QString path = QFileDialog::getSaveFileName(this, "Save File", defaultPath);
        if (!path.isEmpty()) {
            QFileInfo fileInfo(path);
            download->setDownloadDirectory(fileInfo.absolutePath());
            download->setDownloadFileName(fileInfo.fileName());
            download->accept();
        }
    }

    // Opens a local PDF file in a new tab.
    void openPdfFile() {
        QString fileName = QFileDialog::getOpenFileName(this, "Open PDF", "", "PDF Files (*.pdf)");
        if (!fileName.isEmpty()) {
            QUrl fileUrl = QUrl::fromLocalFile(fileName);
            addNewTab(fileUrl);
        }
    }

private:
    QWebEngineView* currentWebView() {
        return qobject_cast<QWebEngineView*>(tabWidget->currentWidget());
    }

    // Rebuilds the bookmarks menu with the current list.
    void updateBookmarksMenu(QMenu *menu) {
        // Remove existing bookmark actions (except "Add Bookmark").
        QList<QAction*> actions = menu->actions();
        for (QAction *action : actions) {
            if (action->text() != "Add Bookmark")
                menu->removeAction(action);
        }
        // Add bookmarks to the menu.
        for (const auto &bm : bookmarksList) {
            QAction *action = new QAction(bm.first, this);
            action->setData(bm.second);
            connect(action, &QAction::triggered, this, &MainWindow::bookmarkTriggered);
            menu->addAction(action);
        }
    }

    QTabWidget *tabWidget;
    QLineEdit *urlBar;
    QList<QPair<QString, QUrl>> bookmarksList;
};

#include "main.moc"

int main(int argc, char *argv[]){
    QApplication app(argc, argv);

    // Set Fusion style and apply a dark palette to the entire application.
    QApplication::setStyle("Fusion");
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42,130,218));
    darkPalette.setColor(QPalette::Highlight, QColor(42,130,218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);

    // Enable persistent cookies and dark mode interceptor.
    QWebEngineProfile::defaultProfile()->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    QWebEngineProfile::defaultProfile()->setRequestInterceptor(new DarkModeInterceptor());
    // Enable built-in PDF viewer support.
    QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);

    MainWindow window;
    window.resize(1200, 800);
    window.show();
    return app.exec();
}

