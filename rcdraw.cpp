/*
 *    Copyright (C) 2008-2010 by RoboLab - University of Extremadura
 *
 *    This file is part of RoboComp
 *
 *    RoboComp is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License aas published by
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
#include "rcdraw.h"

RCDraw::RCDraw( int _width, int _height, uchar *img, QWidget *parent) : QGLWidget(parent), width(_width), height(_height)
{
	resize ( width, height );
	win.setRect ( 0, 0, width, height );

	if ( img != NULL )
		qimg = new QImage ( img, width, height, QImage::Format_Indexed8 );

	init();
}

RCDraw::RCDraw( int _width, int _height, uchar *img, QImage::Format format , QWidget *parent) : QGLWidget(parent), width(_width), height(_height)
{
	resize ( width, height );
	win.setRect ( 0, 0, width, height );

	if ( img != NULL )
		qimg = new QImage ( img, width, height, format );
	init();
}

RCDraw::RCDraw(int _width, int _height, QImage *img, QWidget *parent) : QGLWidget(parent), width (_width), height (_height)
{
	resize (width,height );
	win.setRect ( 0, 0, width, height );

	if (img)
		qimg = img;

	init();
}

RCDraw::RCDraw(QImage * img, QWidget *parent) : QGLWidget ( parent )
{
	if (parent)
	{
		width = parent->width();
		height = parent->height();
	}
	win.setRect ( 0, 0, width, height );

	if (img)
		qimg = img;

	init();
}



RCDraw::RCDraw(int _width, int _height, QWidget * parent): QGLWidget (parent), width (_width), height (_height)
{
	resize ( _width, _height );
	win.setRect ( 0,0,width,height );

	linGrad.setStart ( width, height );
	linGrad.setFinalStop ( width, height-150 );
	linGrad.setSpread ( QGradient::PadSpread );
	qimg = NULL;
	init();
}

RCDraw::RCDraw(QWidget * parent): QGLWidget (parent)
{
	if (parent)
	{
		setParent ( parent );
		resize ( parent->width(), parent->height() );
		win.setRect ( 0,0,parent->width(),parent->height() );
	}
	qimg=NULL;
	init();
}


RCDraw::RCDraw(const QRect &win_, QWidget *parent) : QGLWidget(parent)
{
	if (parent)
	{
		setParent ( parent );
		width = parent->width();
		height = parent->height();
		resize ( parent->width(), parent->height() );
	}
	setWindow ( win_ );
	qimg=NULL;
	init();
}

RCDraw::~RCDraw()
{
}

void RCDraw::autoResize()
{ 
	if (parent())
	{
		setFixedSize(parentWidget()->width(), parentWidget()->height()); 
		width = parentWidget()->width(); 
		height = parentWidget()->height();
		qimg->scaled(width,height);
	}
}

void RCDraw::init( )
{
	invertedVerticalAxis=false;
	DRAW_AXIS = false;
	DRAW_PERIMETER = false;
	imageScale = 1.;
	
	if (qimg!=NULL)
	{
		imageScale = width/qimg->width();
	}
	else
	{
		qimg = new QImage ( width, height, QImage::Format_Indexed8 );
		qimg->fill ( 240 );
	}
	//Gray color table
	ctable.resize ( 256 );
	for ( int i=0; i < 256; i++ )
		ctable[i] = qRgb ( i,i,i );
	qimg->setColorTable ( ctable );
	translating = false;
	effWin = win;
	QGLFormat f = format();
	if (f.sampleBuffers())
	{
		f.setSampleBuffers( true );
		setFormat( f );
		std::cout << "Sample Buffers On in QGLWidget" << std::endl;
	}
	else
		std::cout << "Sample Buffers Off in QGLWidget" << std::endl;

        onSelection = false;
	show();


}


void RCDraw::setImage(QImage* img)
{
	if (qimg != NULL) 
		delete qimg;
	
	qimg = img;
}

void RCDraw::paintEvent ( QPaintEvent * )
{
	QString s;
	QPainter painter ( this );
	painter.setRenderHint(QPainter::HighQualityAntialiasing);

        if(onSelection)
                drawSquare((iniCoorSelected+endCoorSelected)/2, abs(endCoorSelected.x()-iniCoorSelected.x()),abs(endCoorSelected.y()-iniCoorSelected.y()), Qt::green );

        if ( qimg != NULL )
        {
                painter.drawImage ( QRectF(0., 0., imageScale*width, imageScale*height), *qimg, QRectF(0, 0, width, height) );
        }

	painter.setWindow (effWin.toRect() );

	if ( DRAW_PERIMETER )
	{
		painter.setPen ( Qt::blue );
		painter.drawRect ( 0,0,width-1,height-1 );
	}
	if ( DRAW_AXIS )
	{
		drawAxis(Qt::blue, 2);
	}

	//Draw lines
	while ( !lineQueue.isEmpty() )
	{
		TLine l = lineQueue.dequeue();
		painter.setPen ( QPen ( QBrush ( l.color ),l.width ) );
		painter.drawLine ( l.line );
	}

	//Draw gradient
	while ( !gradQueue.isEmpty() )
	{
		TGrad g = gradQueue.dequeue();
		linGrad.setColorAt ( 0, g.color );
		linGrad.setColorAt ( 1, g.color1 );
		painter.setBrush ( linGrad );
		painter.setPen ( QPen ( linGrad, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
		painter.drawLine ( g.line );
	}

	//Draw ellipses
	while ( !ellipseQueue.isEmpty() )
	{
		TEllipse e = ellipseQueue.dequeue();
		if ( e.fill == true )
			painter.setBrush ( e.color );
		else
			painter.setBrush ( Qt::transparent );
		painter.setPen ( e.color );
		if (fabs(e.ang) > 0.1)
		{
			painter.setPen ( e.color );
			painter.translate( e.center );
			painter.rotate( e.ang );
			painter.drawEllipse ( QPointF(0,0), e.rx, e.ry );
			painter.rotate( -e.ang );
			painter.translate( -e.center );
		}
		else
			painter.drawEllipse( e.center, e.rx, e.ry);
		if ( e.id>=0 )
		{
			painter.drawText ( e.rect, Qt::AlignCenter, s.setNum ( e.id ) );
		}
	}

	//Draw squares
	{
		QPen pen = painter.pen();
		int penwidth = pen.width();
		while ( !squareQueue.isEmpty() )
		{
			TRect r = squareQueue.dequeue();
			if ( r.fill == true )
				painter.setBrush ( r.color );
			else
				painter.setBrush ( Qt::transparent );
			pen.setColor(r.color);
			pen.setWidth(r.width);
			painter.setPen(pen);
			if (fabs(r.ang) > 0.01 )
			{
				QPoint center = r.rect.center();
				painter.translate( center );
				painter.rotate( r.ang );
				painter.drawRoundedRect ( QRect( r.rect.topLeft() - center, r.rect.size() ) , 40 , 40 );
				painter.rotate( -r.ang );
				painter.translate( -center );
			}
			else
				painter.drawRect( r.rect );
			if ( r.id>=0 )
				painter.drawText ( QPointF ( r.rect.x(), r.rect.y() ),  s.setNum ( r.id ) );
		}
		pen.setWidth(penwidth);
		painter.setPen(pen);
	}


	while ( !lineOnTopQueue.isEmpty() )
	{
		TLine l = lineOnTopQueue.dequeue();
		painter.setPen ( QPen ( QBrush ( l.color ),l.width ) );
		painter.drawLine ( l.line );
	}


	//Draw text
	while ( !textQueue.isEmpty() )
	{
		TText t = textQueue.dequeue();
		painter.setBrush ( Qt::transparent );
		painter.setPen ( t.color );
		painter.setWindow ( effWin.normalized().toRect() );
		QFont ant = painter.font();
		QFont temp ( "Helvetica", t.size );
		painter.setFont ( temp );
		painter.drawText( QRect(t.pos.x(), t.pos.y(), 0.82*t.text.size()*t.size, 1.2*t.size), Qt::AlignCenter, t.text);

		painter.setFont ( ant );
		painter.setWindow ( effWin.toRect() );
	}


}


void RCDraw::drawSquare ( const QRect &rect, const QColor & col, bool fill, int id, float rot, float width)
{
	TRect r;
	r.rect = rect;
	r.color= col;
	r.id = id;
	r.ang = rot;
	r.fill = fill;
	r.width = width;
	squareQueue.enqueue ( r );
}


void RCDraw::drawSquare ( const QPoint & center, int sideX, int sideY, const QColor & col, bool fill, int id, float rads, float width)
{
	TRect r;
	r.rect = QRect ( center.x()-sideX/2, (center.y()-sideY/2), sideX, sideY );
	r.rect.moveCenter(center);
	r.color= col;
	r.id = id;
	r.fill = fill;
	r.ang = rads*180./M_PI;
	r.width = width;
	squareQueue.enqueue ( r );
}

void RCDraw::drawSquare ( const QPointF & center, int sideX, int sideY, const QColor & col, bool fill, int id, float rads, float width)
{
	TRect r;
	r.rect = QRect ( center.x()-sideX/2, center.y()-sideY/2, sideX, sideY );
	r.color= col;
	r.id = id;
	r.fill = fill;
	r.ang = rads*180./M_PI;
	r.width = width;
	squareQueue.enqueue ( r );
}


void RCDraw::drawLine ( const QLine & line, const QColor & c, float width )
{
	TLine l;
	l.line=line;
	l.color=c;
	l.width = width;
	lineQueue.enqueue ( l );
}

void RCDraw::drawLine ( const QLineF & line, const QColor & c, float width )
{
	TLine l;
	l.line=line;
	l.color=c;
	l.width = width;
	lineQueue.enqueue ( l );
}

void RCDraw::drawLineOnTop ( const QLine & line, const QColor & c, float width )
{
	TLine l;
	l.line=line;
	l.color=c;
	l.width = width;
	lineOnTopQueue.enqueue ( l );
}

void RCDraw::drawLineOnTop ( const QLineF & line, const QColor & c, float width )
{
	TLine l;
	l.line=line;
	l.color=c;
	l.width = width;
	lineOnTopQueue.enqueue ( l );
}

void RCDraw::drawLineFOnTop ( const QLineF & line, const QColor & c, float width )
{
	TLine l;
	l.line=line;
	l.color=c;
	l.width = width;
	lineOnTopQueue.enqueue ( l );
	qDebug() << "Deprecated. Use overloaded version (RCDraw::drawline())";
}

void RCDraw::drawPolyLine ( const QVector< QPoint > & pline, const QColor & c, int width )
{
	TLine l;
	if ( pline.size() > 1 )
	{
		for ( int i=1; i< pline.size(); i++ )
		{
			l.line.setPoints ( pline[i-1],pline[i] );
			l.color=c;
			l.width = width;
			lineQueue.enqueue ( l );
		}
	}
}

void RCDraw::drawPolyLine ( const QVector< int > & xs, const QVector< int > & ys, const QColor & c, int width )
{
	TLine l;
	QPoint pant;
	if ( xs.size() > 1 )
	{
		pant.setX ( xs[0] );
		pant.setY ( ys[0] );
		for ( int i=1; i< xs.size(); i++ )
		{
			l.line.setPoints ( pant,QPoint ( xs[i],ys[i] ) );
			l.color=c;
			l.width = width;
			lineQueue.enqueue ( l );
			pant.setX ( xs[i] );
			pant.setY ( ys[i] );
		}
	}
}

void RCDraw::drawGrad ( const QLine & line, const QColor & c, const QColor & c1, float width )
{
	TGrad g;
	g.line=line;
	g.color=c;
	g.color1=c1;
	g.width = width;
	gradQueue.enqueue ( g );
}

void RCDraw::drawEllipse ( const QRect & rect, const QColor &col, bool fill, int id , float rads)
{
	TEllipse e;
        e.center = QPoint(rect.x()+rect.width()/2, rect.y()+rect.height()/2);
        e.rx = rect.width();
        e.ry = rect.height();
	e.rect = rect;
	e.color= col;
	e.id = id;
	e.fill = fill;
	e.ang = rads;
	ellipseQueue.enqueue ( e );
}

void RCDraw::drawEllipse ( const QPoint & centro, int radiusX,  int radiusY, const QColor & col, bool fill, int id ,  float rads)
{
	TEllipse e;
	e.center = centro;
	e.rx = radiusX;
	e.ry = radiusY;
	e.rect = QRect(centro.x()-radiusX, centro.y()-radiusY, radiusX*2, radiusY*2);
	e.color= col;
	e.id = id;
	e.fill = fill;
	e.ang = rads*180./M_PI;
	ellipseQueue.enqueue ( e );

}

void RCDraw::drawEllipse ( const QPointF & centro, int radiusX,  int radiusY, const QColor & col, bool fill, int id ,  float rads)
{
	TEllipse e;
	e.center = centro;
	e.rx = radiusX;
	e.ry = radiusY;
	e.color= col;
	e.id = id;
	e.fill = fill;
	e.ang = rads*180./M_PI;
	ellipseQueue.enqueue ( e );
}


void RCDraw::drawText ( const QPoint & pos, const QString & text, int size, const QColor & color )
{
	TText t;
	t.pos = pos;
	t.text = text;
	t.size = size;
	t.color = color;
	textQueue.enqueue ( t );
}



///Mouse events

void RCDraw::mousePressEvent ( QMouseEvent *e )
{
    if ( e->button() == Qt::LeftButton )
	{
            iniCoorSelected.setX(e->x());
            iniCoorSelected.setY(e->y());
            endCoorSelected.setX(e->x());
            endCoorSelected.setY(e->y());

            onSelection = true;
            emit pressEvent();
	}
}

void RCDraw::mouseMoveEvent(QMouseEvent *e)
{
          endCoorSelected.setX(e->x());
          endCoorSelected.setY(e->y());
}

void RCDraw::mouseReleaseEvent ( QMouseEvent *e )
{
        if (e->button() == Qt::LeftButton)
                emit windowSelected((iniCoorSelected+endCoorSelected)/2, abs(endCoorSelected.x()-iniCoorSelected.x()),abs(endCoorSelected.y()-iniCoorSelected.y()));
        onSelection = false;
}

void RCDraw::drawPerimeter ( const QColor &c, int width, int margin )
{
	QRect perim ( effWin.x(), effWin.y(), effWin.width()-margin, effWin.height() +margin );
	drawSquare(effWin.toRect(), c);
	width=width;
}
void RCDraw::drawPerimeter ( const QColor &c, int width )
{
	/*QRect perim(effWin.x(),effWin.y(),effWin.width()-10,effWin.height()+10);*/
	QRect perim (effWin.x() +1, effWin.y()-1, effWin.width()-2, effWin.height() +1 );
	drawSquare ( perim , c );
	width=width;
}

void RCDraw::drawAxis ( const QColor &c, int w )
{
	static int step = 310; //ancho baldosa beta

        for ( int i=step; i< effWin.width(); i+=step )
	{
                drawLine ( QLineF ( i, effWin.y(), i, effWin.height()), c, w );
	}
        for ( int i=-step; i> effWin.x(); i-=step )
	{
                drawLine ( QLineF ( i, effWin.y(), i, effWin.height() ), c, w  );
	}
        for ( int i=step; i< effWin.y(); i+=step )
	{
                drawLine ( QLineF ( effWin.x(), i, effWin.width(), i ), c, w  );
	}
        for ( int i=-step; i> effWin.height(); i-=step )
	{
                drawLine ( QLineF ( effWin.x(), i, effWin.width(), i ), c, w  );
	}

        drawLine ( QLine ( rint(effWin.x()), 0, rint(effWin.x() + effWin.width()), 0 ), Qt::red, w*2 );
        drawLine ( QLine ( 0, effWin.y(), 0,effWin.y() + effWin.height()), Qt::red, w*2 );
}

void RCDraw::drawCrossHair ( const QColor & c )
{
	drawLine ( QLine ( effWin.x(), effWin.height() /2, effWin.x() +effWin.width(), effWin.height() /2 ), c, 1 );
	drawLine ( QLine ( effWin.width() /2, effWin.y(), effWin.width() /2,effWin.y() +effWin.height() ), c, 1 );
}

void RCDraw::wheelEvent(QWheelEvent *event)
{
	if (event->delta()>0)
	{
		effWin.setWidth(0.5*effWin.width());
		effWin.setHeight(0.5*effWin.height());
		effWin.translate(0.5*effWin.width(), 0.5*effWin.height());
	}
	else
	{
		effWin.translate(-0.5*effWin.width(), -0.5*effWin.height());
		effWin.setWidth(2*effWin.width());
		effWin.setHeight(2*effWin.height());
	}
}


