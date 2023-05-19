#include "MainWindow.h"
#include "CanvasTabWidget.h"
#include "NewImage.h"
#include "Preferences.h"
#include "Canvas.h"
#include "InputDevice.h"
#include "ColorPicker.h"
#include "BrushSettings.h"
#include "core/EventFilter.h"
#include "core/Constants.h"
#include "engine/BrushEngine.h"
#include <QtWidgets>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setAutoFillBackground(true);

    m_eventFilter = new EventFilter(this);
    qApp->installEventFilter(m_eventFilter);

    m_undoGroup = new QUndoGroup(this);
    m_brushEngine = new BrushEngine(this);

    m_canvasTabWidget = new CanvasTabWidget(m_undoGroup);
    setCentralWidget(m_canvasTabWidget);

    createActions();
    createUi();
    readSettings();
    m_canvasTabWidget->addCanvas(m_brushEngine, m_eventFilter);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    writeSettings();
    event->accept();
}

void MainWindow::onNew() {
    NewImage newImage(m_canvasTabWidget->nextName());

    if (newImage.exec() == QDialog::Accepted) {
        m_canvasTabWidget->addCanvas(newImage.name(), newImage.size(), m_brushEngine, m_eventFilter);
    }
}

void MainWindow::onOpen() {
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Image"), QString(), tr("Images (*.ora)"));

    if (!filePath.isEmpty()) {
        Canvas* canvas = m_canvasTabWidget->addCanvas(QString(), QSize(), m_brushEngine, m_eventFilter);
        canvas->open(filePath);
        m_canvasTabWidget->setTabText(m_canvasTabWidget->currentIndex(), canvas->name());
    }
}

void MainWindow::onSave() {
    Canvas* canvas =  currentCanvas();

    if (!canvas->filePath().isEmpty()) {
        canvas->save();
    } else {
        onSaveAs();
    }
}

void MainWindow::onSaveAs() {
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Image As"),  currentCanvas()->name() + ".ora", tr("OpenRaster (*.ora)"));

    if (!filePath.isEmpty()) {
        QString oraPath = filePath.last(4) != ".ora" ? filePath + ".ora" : filePath;
        currentCanvas()->setFilePath(oraPath);
        currentCanvas()->save();
        m_canvasTabWidget->setTabText(m_canvasTabWidget->currentIndex(), currentCanvas()->name());
    }
}

void MainWindow::onExport() {
    QString fileName = currentCanvas()->name() + ".png";
    QString filePath = QFileDialog::getSaveFileName(this, tr("Export Image"), fileName, tr("Images (*.png)"));

    if (!filePath.isEmpty()) {
        currentCanvas()->exportPng(filePath);
    }
}

void MainWindow::onClose() {
    m_canvasTabWidget->closeCanvas(m_canvasTabWidget->currentIndex());
}

void MainWindow::onCloseAll() {
    for (int i = m_canvasTabWidget->count() - 1; i >= 0; i--) {
        m_canvasTabWidget->closeCanvas(i);
    }
}

void MainWindow::onCloseOthers() {
    for (int i = m_canvasTabWidget->count() - 1; i >= 0; i--) {
        if (m_canvasTabWidget->widget(i) != m_canvasTabWidget->currentWidget()) {
            m_canvasTabWidget->closeCanvas(i);
        }
    }
}

void MainWindow::onClear() {
    currentCanvas()->clear();
}

void MainWindow::onAbout() {
    using namespace Const::App;

    QMessageBox::about(this, tr("About %1").arg(Name),
        tr("<h3>%1 %2</h3>"
           "Painting application<br><br>"
           "Based on Qt %3<br>"
           "Build on %4 %5<br><br>"
           "<a href=%6>%6</a><br><br>"
           "Copyright © 2012-%7, Vladimir Zarypov")
           .arg(Name, Version, QT_VERSION_STR, BuildDate, BuildTime, URL, CopyrightLastYear));
}

void MainWindow::onPreferences() {
    Preferences preferences;

    if (preferences.exec() == QDialog::Accepted) {
        applyHotSettings();
    }
}

void MainWindow::onInputDevice() {
    auto inputDevice = new InputDevice(this);
    connect(m_canvasTabWidget, &CanvasTabWidget::inputDeviceDataChanged, inputDevice, &InputDevice::onDataChanged);
    inputDevice->show();
}

void MainWindow::readSettings() {
    QSettings settings;
    QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();

    if (geometry.isEmpty()) {
        QSize screenSize = QGuiApplication::screens().constFirst()->size();
        resize(screenSize.width() * 2 / 3, screenSize.height() * 2 / 3);
        move((screenSize.width() - width()) / 2, (screenSize.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
}

void MainWindow::writeSettings() {
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
}

void MainWindow::createActions() {
    // File
    QMenu* fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(tr("New..."), Qt::CTRL | Qt::Key_N, this, &MainWindow::onNew);
    fileMenu->addAction(tr("Open..."), Qt::CTRL | Qt::Key_O, this, &MainWindow::onOpen);

    QAction* saveAction = fileMenu->addAction(tr("Save"), Qt::CTRL | Qt::Key_S, this, &MainWindow::onSave);
    QAction* saveAsAction = fileMenu->addAction(tr("Save As..."), Qt::CTRL | Qt::SHIFT | Qt::Key_S, this, &MainWindow::onSaveAs);
    QAction* exportAction = fileMenu->addAction(tr("Export..."), Qt::CTRL | Qt::Key_E, this, &MainWindow::onExport);
    fileMenu->addSeparator();

    QAction* closeAction = fileMenu->addAction(tr("Close"), Qt::CTRL | Qt::Key_W, this, &MainWindow::onClose);
    QAction* closeAllAction =fileMenu->addAction(tr("Close All"), Qt::CTRL | Qt::SHIFT | Qt::Key_W, this, &MainWindow::onCloseAll);
    QAction* closeOthersAction = fileMenu->addAction(tr("Close Others"), Qt::CTRL | Qt::ALT | Qt::Key_W, this, &MainWindow::onCloseOthers);

    fileMenu->addSeparator();
    fileMenu->addAction(tr("Preferences..."), this, &MainWindow::onPreferences);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), Qt::CTRL | Qt::Key_Q, this, &QMainWindow::close);

    // Edit
    QMenu* editMenu = menuBar()->addMenu(tr("Edit"));

    auto undoAction = m_undoGroup->createUndoAction(this, tr("Undo"));
    undoAction->setShortcuts(QKeySequence::Undo);
    editMenu->addAction(undoAction);

    auto redoAction = m_undoGroup->createRedoAction(this, tr("Redo"));
    redoAction->setShortcuts(QKeySequence::Redo);
    editMenu->addAction(redoAction);

    QAction* clearAction = editMenu->addAction(tr("Clear"), Qt::Key_Delete, this, &MainWindow::onClear);

    // View
    m_viewMenu = menuBar()->addMenu(tr("View"));

    // Window
    QMenu* windowMenu = menuBar()->addMenu(tr("Window"));
    windowMenu->addAction(tr("Input Device..."), this, &MainWindow::onInputDevice);

    // Help
    QMenu* helpMenu = menuBar()->addMenu(tr("Help"));
    helpMenu->addAction(tr("About %1...").arg(Const::App::Name), this, &MainWindow::onAbout);

    connect(m_canvasTabWidget, &CanvasTabWidget::countChanged, this, [=] (int count) {
        saveAction->setEnabled(count);
        saveAsAction->setEnabled(count);
        exportAction->setEnabled(count);
        closeAction->setEnabled(count);
        closeAllAction->setEnabled(count);
        closeOthersAction->setEnabled(count >= 2);
        undoAction->setEnabled(count);
        redoAction->setEnabled(count);
        clearAction->setEnabled(count);
    });
}

void MainWindow::createUi() {
    createDockWindows();
}

void MainWindow::createDockWindows() {
    m_colorPicker = new ColorPicker;
    connect(m_colorPicker, &ColorPicker::colorChanged, m_brushEngine, &BrushEngine::setColor);
    connect(m_brushEngine, &BrushEngine::colorChanged, m_colorPicker, &ColorPicker::setColor);

    auto dock = new QDockWidget(m_colorPicker->windowTitle(), this);
    dock->setWidget(m_colorPicker);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    m_viewMenu->addAction(dock->toggleViewAction());

    auto brushSettings = new BrushSettings(m_brushEngine);
    dock = new QDockWidget(brushSettings->windowTitle(), this);
    dock->setWidget(brushSettings);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    m_viewMenu->addAction(dock->toggleViewAction());

    auto undoView = new QUndoView(m_undoGroup);
    dock = new QDockWidget(tr("Commands"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setWidget(undoView);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::applyHotSettings() {

}

Canvas* MainWindow::currentCanvas() const {
    return static_cast<Canvas*>(m_canvasTabWidget->currentWidget());
}
