#pragma once
#include <QMainWindow>

class CanvasTabWidget;
class Canvas;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onExport();

    void onClear();

    void onAbout();
    void onOptions();
    void onInputDevice();

private:
    void readSettings();
    void writeSettings();

    void createActions();
    void createUi();
    void createDockWindows();
    void applyHotSettings();

    Canvas* currentCanvas() const;

    CanvasTabWidget* canvasTabWidget;
    QMenu* viewMenu = nullptr;
};
