#pragma once
#include <QtCore>
#include <QtGui>

class CoreLib : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QWindow* window READ getWindow WRITE setWindow)
    Q_PROPERTY(QString buildDate READ buildDate CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)

public:
    explicit CoreLib() {}
    Q_INVOKABLE bool isFileExists(const QString& filePath);
    //Q_INVOKABLE void buildDate() { qDebug() << QLocale(QLocale::C).toDate(QString(__DATE__).simplified(), QLatin1String("MMM d yyyy")); }
    Q_INVOKABLE void writeOra(QString oraPath, const QSize imageSize, const QVariantList layerList);
    Q_INVOKABLE QVariantList readOra(QString oraPath);
    Q_INVOKABLE void writePng(const QString& pngPath, const QVariantList& canvasItems);
    Q_INVOKABLE void setCursorShape(QString type, int size=0);
    Q_INVOKABLE QString urlToPath(const QUrl& url) { return url.toLocalFile(); }
    Q_INVOKABLE QString pathToBaseName(const QString& path);
    Q_INVOKABLE QString pathToFileName(const QString& path);
    Q_INVOKABLE QColor hsvToColor(qreal h, qreal s, qreal v);
    Q_INVOKABLE QColor pickColor(const QPointF& point, const QVariantList& canvasItems);
    Q_INVOKABLE QVariantMap colorToHsv(const QColor& color);
    Q_INVOKABLE QString byteArrayToBase64(const QByteArray& value);
    Q_INVOKABLE QByteArray base64ToByteArray(const QString& value);
    void setWindow(QWindow* window) { this->window = window; }
    QWindow* getWindow() { return this->window; }
    QString buildDate() { return QString(__DATE__); }
    QString qtVersion() { return QT_VERSION_STR; }

private:
    QPointer<QWindow> window;

};
