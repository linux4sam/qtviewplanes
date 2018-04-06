/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 * Joshua Henderson <joshua.henderson@microchip.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "graphicsplaneview.h"
#include <QDebug>
#include <QPaintEvent>
#include <QGraphicsItem>

GraphicsPlaneView::GraphicsPlaneView(QGraphicsScene *scene)
    : QGraphicsView(scene)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setViewportUpdateMode(ViewportUpdateMode::SmartViewportUpdate);
}

void GraphicsPlaneView::paintEvent(QPaintEvent * event)
{
    qDebug() << "GraphicsPlaneView::paintEvent " << event->region().boundingRect();

    QGraphicsView::paintEvent(event);
}

bool GraphicsPlaneView::eventFilter(QObject* object, QEvent* event)
{
    qDebug() << "GraphicsPlaneView::eventFilter " << event;

    Q_UNUSED(object);

    return false;
}

bool GraphicsPlaneView::event(QEvent *event)
{
    qDebug() << "GraphicsPlaneView::event " << event->type();
    return QGraphicsView::event(event);
}

GraphicsPlaneView::~GraphicsPlaneView()
{}
