/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 * Joshua Henderson <joshua.henderson@microchip.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "planemanager.h"
#include "graphicsplaneitem.h"
#include "graphicsplaneview.h"
#include "tools.h"
#include <cmath>

#include <QApplication>
#include <QTimer>
#include <QTimeLine>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QStateMachine>
#include <QSignalTransition>
#include <QDebug>
#include <QGraphicsProxyWidget>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QCommonStyle>
#include <QStyleOption>
#include <QGraphicsSceneMouseEvent>
#include <QCheckBox>
#include <QGridLayout>
#include <QVector2D>
#include <QGesture>

static auto GRIP_SIZE = 50;

static void drawBox(QPainter *painter, bool focus, QRectF& bounding)
{
#ifdef ENABLE_OPACITY
    painter->setOpacity(0.5);
#endif

    // Background
    QColor backColor("#526d74");
    painter->fillRect(bounding, backColor);

    // Grip
    static QImage grip(QImage(":/media/grip.png").scaled(GRIP_SIZE,
                                                         GRIP_SIZE,
                                                         Qt::KeepAspectRatio,
                                                         Qt::SmoothTransformation));
    QRectF rect(bounding.width() - grip.width(),
                bounding.height() - grip.height(),
                grip.width(),
                grip.height());
    painter->drawImage(rect, grip);

    // Arrows
    QImage arrows(QImage(":/media/arrows.png").scaled(std::min(bounding.width()/2,bounding.height()/2),
                                                      std::min(bounding.width()/2,bounding.height()/2),
                                                      Qt::KeepAspectRatio,
                                                      Qt::SmoothTransformation));

    QRectF rect2(bounding.width()/2 - arrows.width()/2,
                 bounding.height()/2 - arrows.height()/2,
                 arrows.width(),
                 arrows.height());
    painter->drawImage(rect2, arrows);

#ifdef ENABLE_OPACITY
    painter->setOpacity(1.0);
#endif

    // Focus in/out border
    QPen pen;
    pen.setWidth(1);
    if (focus)
    {
        pen.setStyle(Qt::DashLine);
        pen.setColor(QColor(Qt::green));
    }
    else
    {
        pen.setStyle(Qt::SolidLine);
        pen.setColor(QColor(Qt::black));
    }
    painter->setPen(pen);
    painter->drawRect(QRectF(bounding.x(),
                             bounding.y(),
                             bounding.width()-1,
                             bounding.height()-1));
}

static void drawText(QPainter *painter, const char* text)
{
    QPen pen;
    pen.setWidth(1);
    pen.setColor(Qt::cyan);
    painter->setPen(pen);
    QFont font = painter->font() ;
    font.setPointSize(8);
    painter->setFont(font);
    painter->drawText(QPointF(10,30), text);
}

class MyGraphicsItem : public QGraphicsObject
{
public:
    MyGraphicsItem(const QRectF& bounding)
        : QGraphicsObject(),
          m_bounding(bounding),
          m_resize(false),
          m_gestureResize(false)
    {
        setFlags(flags() |
                 QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemIsMovable |
                 QGraphicsItem::ItemClipsToShape);

        grabGesture(Qt::PinchGesture);
    }

    void setSize(const QRectF& bounding)
    {
        prepareGeometryChange();
        m_bounding = bounding;
    }

    virtual QRectF boundingRect() const override
    {
        return m_bounding;
    }

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
        drawBox(painter, option->state & QStyle::State_Selected, m_bounding);
        drawText(painter, "Software");

        Q_UNUSED(option);
        Q_UNUSED(widget);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (!m_resize && event->buttons() & Qt::LeftButton)
        {
            QRectF rect(m_bounding.width()-GRIP_SIZE,
                        m_bounding.height()-GRIP_SIZE,
                        GRIP_SIZE,GRIP_SIZE);
            if (rect.contains(event->pos()))
            {
                m_resize = true;
                m_offset = event->scenePos();
                m_boundingOrig = m_bounding;

                /*
                 * Fun with math.  We need to get the distance from center the mouse press is at.  However,
                 * the distance from center needs to be the distance from the center of the bounding rect,
                 * which does not change when scaled.
                 */
                m_distanceFromCenter = sqrt(pow(event->scenePos().x()-mapToScene(m_boundingOrig.center()).x(),2) +
                                            pow(event->scenePos().y()-mapToScene(m_boundingOrig.center()).y(),2));
            }
        }

        QGraphicsItem::mousePressEvent(event);
    }

    bool sceneEvent(QEvent *event) override
    {
        if (event->type() == QEvent::Gesture)
            return gestureEvent(static_cast<QGestureEvent*>(event));
        return QGraphicsObject::sceneEvent(event);
    }

    void pinchTriggered(QPinchGesture * pinch)
    {
        m_resize = false;

        switch (pinch->state())
        {
        case Qt::GestureStarted:
            m_gestureResize = true;
            m_startScale = scale();
            setFlag(QGraphicsItem::ItemIsMovable, false);
            break;
        case Qt::GestureUpdated:
            setScale(pinch->totalScaleFactor() * m_startScale);
            break;
        case Qt::GestureFinished:
        case Qt::GestureCanceled:
            m_gestureResize = false;
            setFlag(QGraphicsItem::ItemIsMovable, true);
            break;
        case Qt::NoGesture:
            break;
        }
    }

    bool gestureEvent(QGestureEvent *event)
    {
        qDebug() << "gestureEvent " << event;

        if (QGesture *pinch = event->gesture(Qt::PinchGesture))
            pinchTriggered(static_cast<QPinchGesture *>(pinch));

        return true;
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (m_resize && (event->buttons() & Qt::LeftButton))
        {
            qreal width = (m_boundingOrig.width()) + (event->scenePos().x() - m_offset.x());
            qreal height = (m_boundingOrig.height()) + (event->scenePos().y() - m_offset.y());

            if (width > 0 && height > 0)
            {
                prepareGeometryChange();
                m_bounding.setRect(m_boundingOrig.x(),
                                   m_boundingOrig.y(),
                                   width,
                                   height);
            }
        }
        else
        {
            QGraphicsItem::mouseMoveEvent(event);
        }
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        m_resize = false;
        QGraphicsItem::mouseReleaseEvent(event);
    }

private:
    QPointF m_offset;
    QRectF m_bounding;
    QRectF m_boundingOrig;
    bool m_resize;
    qreal m_distanceFromCenter;
    bool m_gestureResize;
    qreal m_startScale;
};

class MyGraphicsPlaneItem : public GraphicsPlaneItem
{
public:

    MyGraphicsPlaneItem(struct plane_data* plane, const QRectF& bounding)
        : GraphicsPlaneItem(plane, bounding),
          m_resize(false),
          m_focus(false),
          m_fb(0),
          m_painter(new QPainter),
          m_gestureResize(false)

    {
        setFlag(QGraphicsItem::ItemIsSelectable);
        setFlag(QGraphicsItem::ItemIsMovable);

        m_dirty = QRectF(0,0,plane_width(m_plane), plane_height(m_plane));

        grow(bounding);

        draw(m_painter);

        grabGesture(Qt::PinchGesture);
    }

    void setSize(const QRectF& bounding)
    {
        if (m_dirty.isNull())
            m_dirty = m_bounding;
        else
            m_dirty = m_dirty.united(m_bounding);

        prepareGeometryChange();

        m_bounding = bounding;

        grow(bounding);

        draw(m_painter);
    }

    void update(const QRectF &rect = QRectF())
    {
        qDebug() << "success: aborted update";

        Q_UNUSED(rect);
    }

    void reinit_painter()
    {
        if (m_fb)
            delete m_fb;

        m_fb = new QImage(static_cast<uchar*>(m_plane->buf),
                          plane_width(m_plane), plane_height(m_plane),
                          QImage::Format_ARGB32_Premultiplied);
    }

    void draw(QPainter* painter)
    {
        qDebug() << "MyGraphicsPlaneItem::draw";

        QImage buffer(boundingRect().united(m_dirty).size().toSize(),
                      QImage::Format_ARGB32_Premultiplied);
        buffer.fill(Qt::transparent);

        QPainter painter2(&buffer);

        if (!m_dirty.isNull())
        {
            painter2.fillRect(m_dirty, Qt::transparent);
            m_dirty = QRectF();
        }

        painter2.setClipRect(boundingRect());

        drawBox(&painter2, m_focus, m_bounding);
        drawText(&painter2, "Hardware");

        painter->begin(m_fb);
        painter->setCompositionMode(QPainter::CompositionMode_Source);
        painter->drawImage(0,0,buffer);
        painter->end();
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (!m_resize && event->buttons() & Qt::LeftButton)
        {
            QRectF rect(m_bounding.width()-GRIP_SIZE,
                        m_bounding.height()-GRIP_SIZE,
                        GRIP_SIZE,GRIP_SIZE);
            if (rect.contains(event->pos()))
            {
                m_resize = true;
                m_offset = event->scenePos();
                m_boundingOrig = m_bounding;

                /*
                 * Fun with math.  We need to get the distance from center the mouse press is at.  However,
                 * the distance from center needs to be the distance from the center of the bounding rect,
                 * which does not change when scaled.
                 */
                m_distanceFromCenter = sqrt(pow(event->scenePos().x()-mapToScene(m_boundingOrig.center()).x(),2) +
                                            pow(event->scenePos().y()-mapToScene(m_boundingOrig.center()).y(),2));
            }
        }

        GraphicsPlaneItem::mousePressEvent(event);
    }

    void grow(const QRectF& bounding)
    {
        if ((int)plane_width(m_plane) != bounding.width() || (int)plane_height(m_plane) != bounding.height())
        {
            QRectF bigger(0, 0,
#if 1
                          bounding.width(),
                          bounding.height()
#else
                          bounding.width() + (bounding.width() * 0.25),
                          bounding.height() + (bounding.height() * 0.25)
#endif
                          );

            qDebug() << "resize fb to " << bigger.width() << "," << bigger.height();

            plane_fb_reallocate(m_plane,
                                bigger.width(),
                                bigger.height(),
                                plane_format(m_plane));

            plane_fb_map(m_plane);
            reinit_painter();

            // must reset position after fb reallocate
            moveEvent(pos());

            if (m_dirty.isNull())
                m_dirty = bigger;
            else
                m_dirty = m_dirty.united(bigger);
        }
    }

    bool sceneEvent(QEvent *event) override
    {
        if (event->type() == QEvent::Gesture)
            return gestureEvent(static_cast<QGestureEvent*>(event));
        return QGraphicsObject::sceneEvent(event);
    }

    void pinchTriggered(QPinchGesture * pinch)
    {
        m_resize = false;

        switch (pinch->state())
        {
        case Qt::GestureStarted:
            m_gestureResize = true;
            m_startScale = scale();
            qDebug() << "start scale " << m_startScale;
            setFlag(QGraphicsItem::ItemIsMovable, false);
            break;
        case Qt::GestureUpdated:
            qDebug() << "new scale " << pinch->totalScaleFactor() * m_startScale;
            setScale(pinch->totalScaleFactor() * m_startScale);
            break;
        case Qt::GestureFinished:
        case Qt::GestureCanceled:
            m_gestureResize = false;
            setFlag(QGraphicsItem::ItemIsMovable, true);
            break;
        case Qt::NoGesture:
            break;
        }
    }

    bool gestureEvent(QGestureEvent *event)
    {
        qDebug() << "gestureEvent " << event;

        if (QGesture *pinch = event->gesture(Qt::PinchGesture))
            pinchTriggered(static_cast<QPinchGesture *>(pinch));

        return true;
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (m_resize && (event->buttons() & Qt::LeftButton))
        {
            qreal width = (m_boundingOrig.width()) + (event->scenePos().x() - m_offset.x());
            qreal height = (m_boundingOrig.height()) + (event->scenePos().y() - m_offset.y());

            if (width > 0 && height > 0)
            {
                prepareGeometryChange();
                m_bounding.setRect(m_boundingOrig.x(),
                                   m_boundingOrig.y(),
                                   width,
                                   height);
            }
        }
        else
        {
            GraphicsPlaneItem::mouseMoveEvent(event);
        }
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (m_resize)
        {
            grow(m_bounding);

            if (m_boundingOrig.width() > m_bounding.width() ||
                    m_boundingOrig.height() > m_bounding.height())
            {
                if (m_dirty.isNull())
                    m_dirty = m_boundingOrig;
                else
                    m_dirty = m_dirty.united(m_boundingOrig);
            }

            draw(m_painter);

            m_resize = false;
        }

        GraphicsPlaneItem::mouseReleaseEvent(event);
    }

    virtual ~MyGraphicsPlaneItem()
    {
        if (m_painter)
            delete m_painter;

        if (m_fb)
            delete m_fb;
    }

protected:

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        if (change == GraphicsItemChange::ItemSelectedHasChanged)
        {
            m_focus = value.toBool();
            draw(m_painter);
        }

        return GraphicsPlaneItem::itemChange(change, value);
    }

private:
    QPointF m_offset;
    QRectF m_boundingOrig;
    bool m_resize;
    bool m_focus;
    QRectF m_dirty;
    QImage* m_fb;
    QPainter* m_painter;
    qreal m_distanceFromCenter;
    bool m_gestureResize;
    qreal m_startScale;
};

#ifdef ALL_SOFTWARE
class MyGraphicsView : public QGraphicsView
#else
class MyGraphicsView : public GraphicsPlaneView
#endif
{
public:
    MyGraphicsView(QGraphicsScene *scene, PlaneManager& planes)
#ifdef ALL_SOFTWARE
        : QGraphicsView(scene)
#else
        : GraphicsPlaneView(scene)
#endif
    {
        m_box1 = new MyGraphicsItem(QRectF(0,0,50,50));
        scene->addItem(m_box1);
#ifdef ALL_SOFTWARE
        m_box2 = new MyGraphicsItem(QRectF(0,0,50,50));
        scene->addItem(m_box2);
#else
        m_box2 = new MyGraphicsPlaneItem(planes.get("overlay1"),
                                       QRectF(0,0,50,50));
        scene->addItem(m_box2);
#endif

        viewport()->grabGesture(Qt::TapAndHoldGesture);
    }

    void positionBoxes()
    {
        m_box1->setScale(1.0);
        m_box1->setSize(QRectF(0,0,width() * 0.3,width() * 0.3));
        m_box2->setScale(1.0);
        m_box2->setSize(QRectF(0,0,width() * 0.3,width() * 0.3));

        int space = (width() - m_box1->boundingRect().width() - m_box2->boundingRect().width())/3;
        m_box1->setPos(space,
                       height()/2 - m_box1->boundingRect().height()/2);
        m_box2->setPos(space + space + m_box1->boundingRect().width(),
                       height()/2 - m_box2->boundingRect().height()/2);
    }

    bool tapAndHoldTriggered(QTapAndHoldGesture * tap)
    {
        switch (tap->state())
        {
        case Qt::GestureStarted:
            break;
        case Qt::GestureUpdated:
            break;
        case Qt::GestureFinished:
        {
            positionBoxes();
            break;
        }
        case Qt::GestureCanceled:
        case Qt::NoGesture:
            break;
        }
        return true;
    }

    bool gestureEvent(QGestureEvent *event)
    {
        qDebug() << "gestureEvent " << event;

        if (QGesture *tap = event->gesture(Qt::TapAndHoldGesture))
        {
            if (tapAndHoldTriggered(static_cast<QTapAndHoldGesture *>(tap)))
            {
                event->accept();
                return true;
            }
        }

        return false;
    }

    void keyPressEvent(QKeyEvent* k)
    {
        if(k->key() == 48){
            QApplication::instance()->exit();
        }
    }

protected:

    bool viewportEvent(QEvent *event) override
    {
        qDebug() << "viewportEvent " << event;

        bool ret = false;
        if (event->type() == QEvent::Gesture)
            ret = gestureEvent(static_cast<QGestureEvent*>(event));

        if (!ret)
#ifdef ALL_SOFTWARE
            return QGraphicsView::viewportEvent(event);
#else
            return GraphicsPlaneView::viewportEvent(event);
#endif

        return true;
    }

private:
    MyGraphicsItem* m_box1;
#ifdef ALL_SOFTWARE
    MyGraphicsItem* m_box2;
#else
    MyGraphicsPlaneItem* m_box2;
#endif
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    PlaneManager planes;
#ifndef ALL_SOFTWARE
    if (!planes.load("qtviewplanes.screen"))
    {
        QMessageBox::critical(0, "Failed to Setup Planes",
                              "This demo requires a version of Qt that provides access to the DRI file descriptor,"
                              " a valid planes screen.config file, and using the linuxfb backend with the env var "
                              "QT_QPA_FB_DRM set.\n");
        return -1;
    }
#endif
    QRect screen = QApplication::desktop()->screenGeometry();

    QGraphicsScene scene;

    /*
     * Create scene items.  Some are standard Qt objects, others are custom ones that
     * translate to hardware plane usage behind the scenes.
     */

    QGraphicsPixmapItem* logo = new QGraphicsPixmapItem(QPixmap(":/media/logo.png"));
    logo->setPos(10, 10);
    scene.addItem(logo);

    QGraphicsTextItem* text = new QGraphicsTextItem();
    text->setDefaultTextColor(Qt::white);
    text->setPos(10, screen.height() - 40);
    text->setPlainText("Qt Graphics View Framework + libplanes");
    scene.addItem(text);

    QProgressBar* progress = new QProgressBar();
    progress->setOrientation(Qt::Horizontal);
    progress->setRange(0, 100);
    progress->setTextVisible(true);
    progress->setAlignment(Qt::AlignCenter);
    progress->setFormat("CPU: %p%");
    progress->setValue(0);
    QPalette p = progress->palette();
    p.setColor(QPalette::WindowText, Qt::white);
    p.setColor(QPalette::Highlight, Qt::red);
    p.setBrush(QPalette::Background, Qt::transparent);
    progress->setPalette(p);
    progress->setMaximumWidth(200);
    QGraphicsProxyWidget *proxy = scene.addWidget(progress);
    proxy->setPos(screen.width() - progress->width() - 10, 10);

    /*
     * Setup the view.
     */

    MyGraphicsView view(&scene, planes);
    view.setStyleSheet("QGraphicsView { border-style: none; }");
    QPixmap background(":/media/background.png");
    view.setBackgroundBrush(background.scaled(screen.width(),
                                              screen.height(),
                                              Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation));
    view.setCacheMode(QGraphicsView::CacheBackground);
    view.resize(screen.width(), screen.height());
    view.setSceneRect(0, 0, screen.width(), screen.height());
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.positionBoxes();
    view.show();

    /*
     * Update the progress bar independently.
     */

    Tools tools;
    QTimer cpuTimer;
    QObject::connect(&cpuTimer, &QTimer::timeout, [&tools,&progress]() {
        tools.updateCpuUsage();
        progress->setValue(tools.cpu_usage[0]);
    });
    cpuTimer.start(500);

    return app.exec();
}
