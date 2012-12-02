#include "colorpicker.h"
#include "qmath.h"

//ColorPicker::ColorPicker(QWidget *parent) : QWidget(parent)
ColorPicker::ColorPicker(QColor color)
{
    setWindowTitle(tr("Color"));
    setMinimumSize(200, 200);
    QPalette pal;
    backgroundColor = pal.color(QPalette::Window);
    Pi = qAtan2(1, 0) * 2; // 3.14
    hueAngle = Pi / 2;
    hueGrab = false;
    satValGrab = false;
    scaleInnerWheel = 0.8;
    currentPickerColor = color;
    if (color.hue() == -1)
        hueColor = 0;
    else
        hueColor = color.hue();
    //currentPickerColor.setHsv(0, 128, 128); // temporary
    //QPixmapCache::setCacheLimit(100000);
}

void ColorPicker::resizeEvent(QResizeEvent *)
{   
    diameterWheel = qMin(width(), height());
    outerRadius = diameterWheel / 2;
    innerRadius = outerRadius * scaleInnerWheel;
    edgeTriangle = innerRadius * qSqrt(3);

    drawWheel();
    drawTriangle();
}

void ColorPicker::drawWheel()
{
    QPixmap wheelPixmap(diameterWheel, diameterWheel);
    wheelPixmap.fill(backgroundColor);

    QPainter painter(&wheelPixmap);
    // Transfer to a logical coordinates with the center of the color wheel at (0, 0)
    painter.setWindow(-outerRadius, -outerRadius, diameterWheel, diameterWheel);
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw the circle, filled the conical gradient of the color ring
    QConicalGradient circleGradient(0, 0, 0);
    QColor circleColor;
    for (int point = 0; point < 360; point++)
    {
        circleColor.setHsv(point, 255, 255);
        circleGradient.setColorAt((qreal)point / 360, circleColor);
    }
    painter.setBrush(circleGradient);
    painter.drawEllipse(QPointF(0, 0), outerRadius, outerRadius);

    // Delete the inner circle to a background window color
    painter.setBrush(QBrush(backgroundColor));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), innerRadius + 2, innerRadius + 2);

    // Store wheel image to the cache
    QPixmapCache::insert("wheel", wheelPixmap);
}

void ColorPicker::paintWheel()
{
    QPixmap wheelPixmap;
    bool checkWheel = QPixmapCache::find("wheel", &wheelPixmap);
    // FIXME: strange bug with the disappearance of the pixmap wheel from a cache
    if (!checkWheel)
    {
        drawWheel();
        QPixmapCache::find("wheel", &wheelPixmap);
        //qDebug() << "redraw wheel";
    }

    QPainter painter(this);
    painter.drawPixmap(width() / 2 - outerRadius, height() / 2 - outerRadius, outerRadius * 2, outerRadius * 2, wheelPixmap);
}

void ColorPicker::drawWheelSelector()
{
    // Draw the hue selector
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(Qt::white, 2));
    painter.setViewport((width() - outerRadius * 2) / 2, (height() - outerRadius * 2) / 2, outerRadius * 2, outerRadius * 2);
    painter.setWindow(-outerRadius, -outerRadius, outerRadius * 2, outerRadius * 2);
    painter.rotate(360 - hueColor);

    QPointF hueSelectorInner(innerRadius + 3, 0);
    QPointF hueSelectorOuter(outerRadius, 0);

    painter.drawLine(hueSelectorInner, hueSelectorOuter);
}

void ColorPicker::drawTriangle()
{
    QImage triangleImage(innerRadius * 2, innerRadius * 2, QImage::Format_ARGB32);
    // Transparent square
    triangleImage.fill(QColor(0, 0, 0, 0));

    QPolygonF trianglePolygon;
    trianglePolygon << QPointF(innerRadius * 2, innerRadius)
                    << QPointF(innerRadius / 2, innerRadius - edgeTriangle / 2)
                    << QPointF(innerRadius / 2, innerRadius + edgeTriangle / 2);
    // Painting the black opacity triangle on the square
    QPainter painter(&triangleImage);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(Qt::black));
    painter.drawPolygon(trianglePolygon);

    qreal color;
    int roundColor;
    qreal stepColor;
    qreal alpha;
    qreal x, x1, y, y1;
    x = innerRadius * 3 / 2; // median
    // Pixels of the triangle changes by rules:
    // 1. Hue corner of the triangle located to the east,
    //      black corner located to the north and white corner  located to the south
    // 2. In the color area, there is a verical black-white gradient from the north to the south
    // 3. In the alpha area, there is a linear gradient from hue corner to the black-white border
    for (int pointX = 0; pointX < innerRadius * 2; pointX++)
    {
        if (qAlpha(triangleImage.pixel(pointX, innerRadius)) != 0)
        {
            stepColor = qSqrt(3) / (2 * (2 * innerRadius - pointX)) * 255;
            color = 0;
            x1 = innerRadius * 2 - pointX;
            for (int pointY = 0; pointY < innerRadius * 2; pointY++)
            {
                if (qAlpha(triangleImage.pixel(pointX, pointY)) != 0)
                {
                    roundColor = qRound(color);
                    y1 = innerRadius - pointY;
                    y = x * y1 / x1;
                    alpha =  qSqrt((qPow(x1, 2) + qPow(y1, 2)) / (qPow(x, 2) + qPow(y, 2))) * 255;
                    triangleImage.setPixel(pointX, pointY, qRgba(roundColor, roundColor, roundColor, qRound(alpha)));
                    color += stepColor;
                }
            }
        }
    }

    // Convert image to pixmap
    QPixmap trianglePixmap(innerRadius * 2, innerRadius * 2);
    trianglePixmap = trianglePixmap.fromImage(triangleImage);

    // Store triangle image to the cache
    QPixmapCache::insert("triangle", trianglePixmap);
}

void ColorPicker::paintTriangle()
{
    QPixmap trianglePixmap;
    bool checkTriangle = QPixmapCache::find("triangle", &trianglePixmap);

    // FIXME: strange bug with the disappearance of the pixmap triangle from a cache
    if (!checkTriangle)
    {
        drawTriangle();
        QPixmapCache::find("triangle", &trianglePixmap);
        //qDebug() << "redraw triangle";
    }

    QPainter painter(this);
    painter.setViewport(width() / 2 - innerRadius, height() / 2 - innerRadius, innerRadius * 2, innerRadius * 2);
    painter.setWindow(-innerRadius, -innerRadius, innerRadius * 2, innerRadius * 2);
    painter.rotate(360 - hueColor);

    QColor triangleHueColor;
    triangleHueColor.setHsv(hueColor, 255, 255);
    painter.setBrush(QBrush(triangleHueColor));
    painter.setPen(Qt::NoPen);
    QPolygonF trianglePolygon;
    trianglePolygon << QPointF(innerRadius, 0) << QPointF(-innerRadius / 2, -edgeTriangle / 2) << QPointF(-innerRadius / 2, edgeTriangle / 2);
    // Painting the hue-color triangle
    painter.drawPolygon(trianglePolygon);
    // Painting the alpha-mask triangle above hue-color triangle
    painter.drawPixmap(-innerRadius, -innerRadius, innerRadius * 2, innerRadius * 2, trianglePixmap);

    QPointF startPoint;
    QPointF endPoint;
    QLinearGradient borderGradient;
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Antialiasing border from hue to black corner
    startPoint = QPointF(innerRadius, 0);
    endPoint = QPointF(-innerRadius / 2, -edgeTriangle / 2);
    QColor triangleBlackColor;
    triangleBlackColor.setHsv(hueColor, 255, 0);
    borderGradient = QLinearGradient(startPoint, endPoint);
    borderGradient.setColorAt(0, triangleHueColor);
    borderGradient.setColorAt(1, triangleBlackColor);
    painter.setPen(QPen(borderGradient, 2.7));
    painter.drawLine(startPoint, endPoint);

    // Antialiasing border from hue to white corner
    startPoint = QPointF(innerRadius, 0);
    endPoint = QPointF(-innerRadius / 2, edgeTriangle / 2);
    borderGradient = QLinearGradient(startPoint, endPoint);
    borderGradient.setColorAt(0, triangleHueColor);
    borderGradient.setColorAt(1, Qt::white);
    painter.setPen(QPen(borderGradient, 2.7));
    painter.drawLine(startPoint, endPoint);

    // Antialiasing border from black to white corner
    startPoint = QPointF(-innerRadius / 2, -edgeTriangle / 2);
    endPoint = QPointF(-innerRadius / 2, edgeTriangle / 2);
    borderGradient = QLinearGradient(startPoint, endPoint);
    borderGradient.setColorAt(0, Qt::black);
    borderGradient.setColorAt(1, Qt::white);
    painter.setPen(QPen(borderGradient, 2.7));
    painter.drawLine(startPoint, endPoint);
}

void ColorPicker::drawTriangleSelector()
{
    // Drawing the sat/val pointer
    QPainter painter(this);
    painter.setViewport(width() / 2 - innerRadius, height() / 2 - innerRadius, innerRadius * 2, innerRadius * 2);
    painter.setWindow(-innerRadius, -innerRadius, innerRadius * 2, innerRadius * 2);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // In the white corner, the pointer changed color from white to black
    if (currentPickerColor.saturation() < 64 && currentPickerColor.value() > 192)
        painter.setPen(QPen(Qt::black, 2));
    else
       painter.setPen(QPen(Qt::white, 2));

    triangleSelectorPoint = colorToCoord(currentPickerColor);
    QPointF correctPoint = QPointF(triangleSelectorPoint.x(), -triangleSelectorPoint.y());
    painter.drawEllipse(correctPoint, 4, 4);
}

QColor ColorPicker::coordToColor(QPoint coord)
{
    qreal median = edgeTriangle * qSqrt(3) / 2;
    // Position relative to the widget center
    qreal relX = coord.x() - width() / 2;
    qreal relY = height() / 2 - coord.y();
    // Position relative to the widget center with considering rotating of the wheel
    qreal length = qSqrt(qPow(relX, 2) + qPow(relY, 2));
    // Rotating the triangle so that the black corner pointing to the east
    qreal angle = qAtan2(relY, relX) - (hueColor + 120) * Pi / 180;
    // Correcting relative position
    relX = length * qCos(angle);
    relY = length * qSin(angle);

    //Value
    qreal x1 = innerRadius - relX;
    qreal y1 = relY;
    qreal x = median;
    qreal y = y1 * x / x1;
    qreal len1 = qSqrt(qPow(x1, 2) + qPow(y1, 2));
    qreal len = qSqrt(qPow(x, 2) + qPow(y, 2));
    int value = qRound(len1 / len * 255);
    if (value > 255) value = 255;
    if (x1 < 0) value = 0;

    // Saturation
    qreal median1 = innerRadius - relX;
    y = median1 * 2 / qSqrt(3);
    y1 = y / 2 - relY;
    if (y1 < 0) y1 = 0;

    int saturation = qRound(y1 * 255 / y);
    if (saturation > 255) saturation = 255;
    // Step changing the saturation from 0 to 255 (and back) in black corner
    if (saturation < 0 && y1 > 0) saturation = 255;
    if (saturation < 0) saturation = 0;

    QColor outColor;
    outColor.setHsv(qRound(hueColor), saturation, value);

    return outColor;
}

QPoint ColorPicker::colorToCoord(QColor color)
{
    int saturation = color.saturation();
    int value = color.value();

    qreal y1 = value / 255.0 * edgeTriangle;
    qreal relY = edgeTriangle * saturation / 255.0 * value / 255.0;
    relY = y1 / 2.0 - relY;
    qreal median = edgeTriangle * qSqrt(3) / 2;
    qreal relX = innerRadius - qSqrt((qPow(median, 2) + qPow(edgeTriangle / 2 - edgeTriangle * saturation / 255.0, 2)) * qPow(value / 255.0, 2) - qPow(relY, 2));
    qreal length = qSqrt(qPow(relX, 2) + qPow(relY, 2));
    qreal angle = qAtan2(relY, relX) + (hueColor + 120) * Pi / 180;

    relX = length * qCos(angle);
    relY = length * qSin(angle);
    QPoint outPoint = QPoint(qRound(relX), qRound(relY));

    return outPoint;
}

void ColorPicker::paintEvent(QPaintEvent *)
{
    paintWheel();
    paintTriangle();

    drawWheelSelector();
    drawTriangleSelector();
}

void ColorPicker::mousePressEvent(QMouseEvent *event)
{
    QPointF point = event->pos() - rect().center();
    qreal length = qSqrt(qPow(point.x(), 2) + qPow(point.y(), 2));
    if (length >= innerRadius && length <= outerRadius)
    {
        hueAngle = qAtan2(point.x(), point.y());
        hueColor = hueAngle * 180 / Pi - 90;
        if (hueColor < 0) hueColor += 360;
        currentPickerColor.setHsv(qRound(hueColor), currentPickerColor.saturation(), currentPickerColor.value());
        hueGrab = true;
        emit colorSignal();
    }
    if (length < innerRadius)
    {
        satValAngle = qAtan2(point.x(), point.y());
        satValGrab = true;
        currentPickerColor = coordToColor(event->pos());
        emit colorSignal();
    }
    //qDebug() << currentPickerColor.hue() << qRound(currentPickerColor.saturation() / 255.0 * 100.0) << qRound(currentPickerColor.value() / 255.0 * 100.0);
    update();

}

void ColorPicker::mouseMoveEvent(QMouseEvent *event)
{
    QPointF point = event->pos() - rect().center();
    if (hueGrab)
    {
        hueAngle = qAtan2(point.x(), point.y());
        hueColor = hueAngle * 180 / Pi - 90;
        if (hueColor < 0) hueColor += 360;
        currentPickerColor.setHsv(qRound(hueColor), currentPickerColor.saturation(), currentPickerColor.value());
        emit colorSignal();
    }
    if (satValGrab)
    {
        satValAngle = qAtan2(point.x(), point.y());
        currentPickerColor = coordToColor(event->pos());
        emit colorSignal();
    }
    //qDebug() << currentPickerColor.hue() << qRound(currentPickerColor.saturation() / 255.0 * 100.0) << qRound(currentPickerColor.value() / 255.0 * 100.0);
    update();
}

void ColorPicker::mouseReleaseEvent(QMouseEvent *)
{
    hueGrab = false;
    satValGrab = false;
}

void ColorPicker::setColor(QColor color)
{
    currentPickerColor = color;
    hueColor = color.hue();
}
