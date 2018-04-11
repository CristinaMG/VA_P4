/*
 *    Copyright (C) 2008-2010 by RoboLab - University of Extremadura
 *
 *    This file is part of RoboComp
 *
 *    RoboComp is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    RoboComp is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RoboComp.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef RCDRAW_H
#define RCDRAW_H

#include <QtGui>
#include <stdint.h>
#include <iostream>

#include <QGLWidget>


/**
	@author authorname <authormail>

  The constructor will automatically call show() method.

*/
class RCDraw : public QGLWidget
{
Q_OBJECT
public:
	RCDraw(int _width, int _height, uchar *img=NULL, QWidget *parent = 0);
	RCDraw(int _width, int _height, uchar *img=NULL, QImage::Format format=QImage::Format_Indexed8, QWidget *parent = 0);
	RCDraw(int _width, int _height, QImage *img=NULL, QWidget *parent = 0);
	RCDraw(int _width, int _height, QWidget *parent = 0);
	RCDraw(QImage *img, QWidget *parent = 0);
	RCDraw(QWidget *parent = 0);
	RCDraw(const QRect & win_, QWidget *parent = 0);
	~RCDraw();
	void init();
	void setImage(QImage *img);
	void paintEvent(QPaintEvent *);
	void setWindow(const QRect & win_) { effWin = win = win_; }
	void drawSquare(const QRect &, const QColor &,  bool fill=false, int id= -1, float rads=0, float width=0);
	void drawSquare(const QPoint &, int sideX, int sideY, const QColor &,  bool fill=false , int id= -1, float rads=0, float width=0);
	void drawSquare(const QPointF &, int sideX, int sideY, const QColor &,  bool fill=false , int id= -1, float rads=0, float width=0);
	void drawLine(const QLine &line, const QColor & c, float width=0);
	void drawLine(const QLineF &line, const QColor & c, float width=0);
	void drawLineOnTop(const QLine &line, const QColor & c, float width=0);
	void drawLineOnTop(const QLineF &line, const QColor & c, float width=0);
	void drawLineFOnTop(const QLineF &line, const QColor & c, float width=0);
	void drawPolyLine(const QVector<QPoint> & pline, const QColor & c, int width=1);
	void drawPolyLine(const QVector<int> & xs, const QVector<int> & ys, const QColor & c, int width=1);
	void drawGrad(const QLine &line, const QColor & c, const QColor & c1, float width=0);
	void drawEllipse(const QRect &, const QColor &, bool fill= false, int id =-1, float rads=0);
	void drawEllipse(const QPointF &, int radiusX, int radiusY, const QColor &, bool fill=false, int id =-1, float rads=0);
	void drawEllipse(const QPoint &, int radiusX, int radiusY, const QColor &, bool fill=false, int id =-1, float rads=0);
	void drawAxis(const QColor &, int w);
	void drawPerimeter(const QColor &c, int width, int margin);
	void drawPerimeter(const QColor &c, int width);
	void drawCrossHair(const QColor &c);
	void drawText(const QPoint & pos, const QString & text, int size, const QColor & color);
	void scaleImage(float sscale) { imageScale = sscale; setFixedSize(sscale*width, sscale*height); }

	void setDrawAxis(bool f) { DRAW_AXIS = f;}
	void setDrawPerimeter(bool f) { DRAW_PERIMETER = f;}
	QRectF getWindow() { return win;}

	uint32_t getWidth() { return width; }
	uint32_t getHeight() { return height; }
	void autoResize();
	uchar *imageBuffer() { if (qimg != NULL) return qimg->bits(); return NULL; }
	

protected:
	float imageScale;
	bool invertedVerticalAxis;
	struct TRect
	{
		QRect rect;
		QColor color;
		int id;
		float ang;
		bool fill;
		float width;
	};

	struct TEllipse
	{
		QRect rect;
		QPointF center;
		float rx, ry;
		QColor color;
		int id;
		bool fill;
		float ang;
	};

	struct TLine
	{
		QLineF line;
		QColor color;
		float width;
	};

	struct TGrad
	{
		QLine line;
		QColor color;
		QColor color1;
		float width;
	};

	struct TText
	{
		QPoint pos;
		int size;
		QColor color;
		QString text;
		float width;
	};

	int width, height;
	QRectF win;
	QRectF effWin;
	QQueue<TRect> squareQueue;
	QQueue<TLine> lineQueue;
	QQueue<TLine> lineOnTopQueue;
	QQueue<TEllipse> ellipseQueue;
	QQueue<TGrad> gradQueue;
	QQueue<TText> textQueue;

	QImage *qimg;
	QVector<QRgb> ctable; //For gray conversion
	QPoint inicio, actual;
	bool translating;
	QPointF backPos;
	bool DRAW_AXIS, DRAW_PERIMETER;

	QLinearGradient linGrad;

        QPointF iniCoorSelected, endCoorSelected;
        bool onSelection;

signals:
    void windowSelected(QPointF center, int sizeX, int sizeY);
    void pressEvent();
protected:
	void wheelEvent(QWheelEvent *);
	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
};

#endif
