/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 * Joshua Henderson <joshua.henderson@microchip.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef GRAPHICSPLANEVIEW_H
#define GRAPHICSPLANEVIEW_H

#include <QGraphicsView>

/**
 * @brief The GraphicsPlaneView class
 *
 * An optimized GraphicsView for suporting a view containing a GraphicsPlaneItem.
 */
class GraphicsPlaneView : public QGraphicsView
{
public:
    GraphicsPlaneView(QGraphicsScene *scene);

    virtual bool eventFilter(QObject* object, QEvent* event) override;

    virtual ~GraphicsPlaneView();

protected:
    virtual void paintEvent(QPaintEvent * event) override;
    virtual bool event(QEvent *event) override;
};

#endif // GRAPHICSPLANEVIEW_H
