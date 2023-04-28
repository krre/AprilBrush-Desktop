#pragma once
#include <QTabWidget>

class QUndoGroup;

class CanvasTabWidget : public QTabWidget {
    Q_OBJECT
public:
    CanvasTabWidget(QUndoGroup* undoGroup);

    void addCanvas();
    void addCanvas(const QString& name, const QSize& size);

    QString nextName();

public slots:
    void closeCanvas(int index);

signals:
    void countChanged(int count);

private slots:
    void onCurrentChanged(int index);

private:
    int maxTabCount = 1;
    QUndoGroup* undoGroup;
};
