#include "OpenRaster.h"
#include "engine/Layer.h"
#include <QtGui/private/qzipwriter_p.h>
#include <QtGui/private/qzipreader_p.h>
#include <QtGui>

OpenRaster::OpenRaster() {

}

void OpenRaster::write(const QString& filePath, const QSize& size, const Layers& layers) {
    QZipWriter zipWriter(filePath);
    zipWriter.setCompressionPolicy(QZipWriter::AutoCompress);

    QByteArray xmlByteArray;

    // mimetype file
    xmlByteArray.append("image/openraster");
    zipWriter.addFile("mimetype", xmlByteArray);

    // stack.xml file
    QXmlStreamWriter stream(&xmlByteArray);
    stream.setAutoFormatting(true);

    stream.writeStartDocument();

    stream.writeStartElement("image");
    stream.writeAttribute("w", QString::number(size.width()));
    stream.writeAttribute("h",QString::number(size.height()));
    stream.writeStartElement("stack");

    for (const auto& layer : layers) {
        QString src = "data/" + layer->name() + ".png";
        zipWriter.addFile(src, layer->data());

        // layer
        stream.writeStartElement("layer");
        stream.writeAttribute("name", layer->name());
        stream.writeAttribute("composite-op", "svg:src-over");
        stream.writeAttribute("visibility", layer->isVisible() ? "visible" : "hidden");
        stream.writeAttribute("edit-locked", QVariant(layer->isLocked()).toString());
        stream.writeAttribute("selected", QVariant(layer->isSelected()).toString());
        stream.writeAttribute("src", src);
        stream.writeAttribute("x", "0");
        stream.writeAttribute("y", "0");
        stream.writeAttribute("opacity", "1.0");
        stream.writeEndElement(); // layer
    }

    stream.writeEndElement(); // stack
    stream.writeEndElement(); // image
    stream.writeEndDocument(); // document
    zipWriter.addFile("stack.xml", xmlByteArray);

    zipWriter.close();
}

Layers OpenRaster::read(const QString& filePath) {
    QZipReader zipReader(filePath, QIODevice::ReadOnly);

    QByteArray xmlByteArray = zipReader.fileData("stack.xml");
    QXmlStreamReader stream(xmlByteArray);
    Layers result;

    while (!stream.atEnd()) {
        if (stream.isStartElement()) {
            if (stream.name().toString() == "layer") {
                QSharedPointer<Layer> layer(new Layer("", QSize()));

                layer->setName(stream.attributes().value("name").toString());
                layer->setVisible(QVariant(stream.attributes().value("visibility").toString()).toBool());
                layer->setLocked(QVariant(stream.attributes().value("edit-locked").toString()).toBool());
                layer->setSelected(QVariant(stream.attributes().value("selected").toString()).toBool());

                QByteArray data = zipReader.fileData(stream.attributes().value("src").toString());
                layer->setData(data);

                result.append(layer);
            }
        }

        stream.readNextStartElement();
    }

    zipReader.close();

    return result;
}

QVariantMap OpenRaster::readAttr(const QString& filePath) {
    QVariantMap result;

    QZipReader zipReader(filePath, QIODevice::ReadOnly);

    QByteArray xmlByteArray = zipReader.fileData("stack.xml");
    QXmlStreamReader stream(xmlByteArray);

    while (!stream.atEnd()) {
        if (stream.isStartElement()) {
            if (stream.name().toString() == "image") {
                for (int i = 0; i < stream.attributes().size(); i++) {
                    result[stream.attributes().at(i).name().toString()] = stream.attributes().at(i).value().toString();
                }
            }
        }
        stream.readNextStartElement();
    }

    zipReader.close();

    return result;
}
