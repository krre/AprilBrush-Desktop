#include "Canvas.h"
#include "InputDevice.h"
#include "ColorPicker.h"
#include "engine/BrushEngine.h"
#include "engine/Layer.h"
#include "engine/undo/ClearCommand.h"
#include "engine/undo/BrushCommand.h"
#include "core/SignalHub.h"
#include "core/Context.h"
#include "core/OpenRaster.h"
#include <QtGui>

Canvas::Canvas(const QSize& size, BrushEngine* brushEngine) : brushEngine(brushEngine) {
    resize(size);
    m_buffer = QPixmap(size);
    m_buffer.fill(Qt::transparent);

    m_undoStack = new QUndoStack(this);
    m_undoStack->setUndoLimit(50);

    addLayer(nextName());

    connect(Context::instance(), &Context::keyPressed, this, &Canvas::onKeyPressed);
    connect(Context::instance(), &Context::keyReleased, this, &Canvas::onKeyReleased);

    connect(brushEngine, &BrushEngine::sizeChanged, this, &Canvas::drawCursor);
    drawCursor(brushEngine->size());
}

Canvas::~Canvas() {

}

void Canvas::setName(const QString& name) {
    m_name = name;
}

const QString& Canvas::name() const {
    return m_name;
}

void Canvas::setFilePath(const QString& filePath) {
    m_filePath = filePath;
    m_name = filePathToName(filePath);
}

const QString& Canvas::filePath() const {
    return m_filePath;
}

const QPixmap& Canvas::buffer() const {
    return m_buffer;
}

QUndoStack* Canvas::undoStack() const {
    return m_undoStack;
}

void Canvas::save() {
    OpenRaster openRaster;
    openRaster.write(m_filePath, size(), layers);
}

void Canvas::open(const QString& filePath) {
    OpenRaster openRaster;
    layers = openRaster.read(filePath);

    m_filePath = filePath;
    m_name = filePathToName(filePath);
}

void Canvas::exportPng(const QString& filePath) {
    QPixmap pixmap(width(), height());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);

    for (int i = layers.count() - 1; i >= 0; i--) {
        painter.drawPixmap(0, 0, *layers.at(i)->pixmap());
    }

    pixmap.save(filePath);
}

void Canvas::addLayer(const QString& name) {
    auto newLayer = QSharedPointer<Layer>(new Layer(name, size()));
    layers.append(newLayer);
    m_currentLayerIndex = layers.count() - 1;
}

void Canvas::addLayer(const QSharedPointer<Layer>& layer) {

}

void Canvas::select() {

}

void Canvas::unselect() {

}

void Canvas::clear() {
    m_undoStack->push(new ClearCommand(this, currentLayer()));
}

void Canvas::setCurrentLayerIndex(int index) {
    m_currentLayerIndex = index;
}

int Canvas::currentLayerIndex() const {
    return m_currentLayerIndex;
}

Layer* Canvas::currentLayer() const {
    return m_currentLayerIndex >= 0 ? layers.at(m_currentLayerIndex).data() : nullptr;
}

QString Canvas::nextName() {
    return tr("Untitled-%1").arg(maxTabCount++);
}

void Canvas::mouseMoveEvent(QMouseEvent* event) {
    if (pickPressed()) {
        pickColor(event->position());
    } else {
        paintAction(event->position());
    }
}

void Canvas::mousePressEvent(QMouseEvent* event) {
    setFocus();

    if (pickPressed()) {
        pickColor(event->position());
    } else {
        paintArea = QRect(event->position().toPoint(), event->position().toPoint());
        paintAction(event->position());
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent*) {
    double opacity = brushEngine->opacity() / 100.0;
    m_undoStack->push(new BrushCommand(this, currentLayer(), paintArea, opacity));

    QPainter painter(currentLayer()->pixmap());
    painter.setOpacity(opacity);
    painter.drawPixmap(0, 0, m_buffer);

    m_buffer.fill(Qt::transparent);
    brushEngine->finish();
    update();
}

void Canvas::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    QPainter painter(this);

    for (int i = layers.count() - 1; i >= 0; i--) {
        if (m_currentLayerIndex == i) {
            painter.setOpacity(brushEngine->opacity() / 100.0);
            painter.drawPixmap(0, 0, m_buffer);
        }

        painter.setOpacity(1.0);
        painter.drawPixmap(0, 0, *layers.at(i)->pixmap());
    }
}

void Canvas::drawCursor(int size) {
   // Size of the cursor should not be very small
   int sizeBrush = qMax(size, 3);
   QPixmap pixmap(sizeBrush, sizeBrush);
   pixmap.fill(QColor(255, 255, 255, 0));

   QPainter painter(&pixmap);
   painter.setRenderHint(QPainter::Antialiasing, true);
   painter.setBrush(Qt::NoBrush);
   painter.setPen(QColor(0, 0, 0, 200));
   painter.drawEllipse(0, 0, sizeBrush, sizeBrush);
   painter.setPen(QColor(255, 255, 255, 200));
   painter.drawEllipse(1, 1, sizeBrush - 2, sizeBrush - 2);

   setCursor(QCursor(pixmap));
}

void Canvas::onKeyPressed(QKeyEvent* event) {
    if (event->key() == Qt::Key_Alt) {
        setCursor(Qt::CrossCursor);
    }
}

void Canvas::onKeyReleased(QKeyEvent* event) {
    if (event->key() == Qt::Key_Alt) {
        drawCursor(brushEngine->size());
    }
}

void Canvas::paintAction(const QPointF& pos) {
    QRect bound = brushEngine->paint(brushEngine->eraser() < 50 ? &m_buffer : currentLayer()->pixmap(), pos);

    if (bound.isNull()) return;

    QRect clipedBound = clipBound(bound);
    update(clipedBound);

    paintArea.setTopLeft(QPoint(qMin(paintArea.topLeft().x(), clipedBound.topLeft().x()), qMin(paintArea.topLeft().y(), clipedBound.topLeft().y())));
    paintArea.setBottomRight(QPoint(qMax(paintArea.bottomRight().x(), clipedBound.bottomRight().x()), qMax(paintArea.bottomRight().y(), clipedBound.bottomRight().y())));

    InputDevice::Data data{};
    data.type = InputDevice::Type::Mouse;
    data.pos = pos;

    emit SignalHub::instance()->inputDeviceDataChanged(data);
}

QString Canvas::filePathToName(const QString& filePath) const {
    return QFileInfo(filePath).fileName().replace(".ora", "");
}

bool Canvas::pickPressed() const {
    return QGuiApplication::queryKeyboardModifiers().testFlag(Qt::AltModifier);
}

void Canvas::pickColor(const QPointF& pos) {
    QPixmap pixmap(width(), height());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);

    for (int i = layers.count() - 1; i >= 0; i--) {
        painter.drawPixmap(0, 0, *layers.at(i)->pixmap());
    }

    emit colorPicked(QColor(pixmap.toImage().pixel(qRound(pos.x()), qRound(pos.y()))));
}

QRect Canvas::clipBound(const QRect& bound) {
    // Correct corner positions on brush size
    int burhsSize = brushEngine->size();
    QPoint topLeft(bound.topLeft().x() - burhsSize, bound.topLeft().y() - burhsSize);
    QPoint bottomRight(bound.bottomRight().x() + burhsSize, bound.bottomRight().y() + burhsSize);

    // Bound to canvas size
    topLeft.setX(qMax(0, topLeft.x()));
    topLeft.setY(qMax(0, topLeft.y()));
    bottomRight.setX(qMin(size().width(), bottomRight.x()));
    bottomRight.setY(qMin(size().height(), bottomRight.y()));

    return QRect(topLeft, bottomRight);
}
