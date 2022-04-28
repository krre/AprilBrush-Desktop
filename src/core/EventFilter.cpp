#include "EventFilter.h"
#include "Context.h"
#include <QtGui>

EventFilter::EventFilter(QObject* parent) : QObject(parent) {

}

bool EventFilter::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        emit Context::instance()->keyPressed(keyEvent);
        return true;
    } else if (event->type() == QEvent::KeyRelease) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        emit Context::instance()->keyReleased(keyEvent);
        return true;
    } else {
        return QObject::eventFilter(watched, event);
    }
}