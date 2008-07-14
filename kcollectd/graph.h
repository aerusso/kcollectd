/* -*- c++ -*-
 *
 * (C) 2008 M G Berberich
 *
 */
/*
 * This file is part of the source of kcollectd, a viewer for
 * rrd-databases created by collectd
 * 
 * Copyright (C) 2008 M G Berberich
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <iostream>
#include <vector>
#include <limits>

#include <qframe.h>
#include <qpixmap.h>
#include <qrect.h>

/**
 * small helper holding tow doubles e.g. a point
 */
class Range {
  double x_;
  double y_;
  static const double NaN;
public:
  Range() : x_(NaN), y_(NaN) { }
  Range(double x, double y) : x_(x), y_(y) { }
  double min() const { return x_; }
  double max() const { return y_; }
  void min(double a) { x_ = a; }
  void max(double a) { y_ = a; }
  bool isValid() const { return x_ != NaN; }
};

/**
 * linear mapping from range [x1, x2] to range [y1, y2]
 */
class linMap {
  double m_, t_;
public:
  linMap(double x1, double y1, double x2, double y2) {
    m_ = (y2-y1)/(x2-x1);
    t_ = y1-m_*x1;
  }
  double operator()(double x) const { return m_ * x + t_; }
  double m() const { return m_; }
};

/**
 *
 */
class Graph : public QFrame
{
  Q_OBJECT;
 public:
  Graph(QWidget *parent, const char *name=0);
  Graph(QWidget *parent, const std::string &rrd, const std::string &ds, 
	const char *name=0);

  void setup(const char *rrd, const char *ds, const char* label = 0);

  virtual QSize sizeHint() const;
  virtual void paintEvent(QPaintEvent *ev);

  virtual void last(time_t span);
  virtual void zoom(double factor);
  virtual void mousePressEvent(QMouseEvent *e);
  virtual void mouseReleaseEvent(QMouseEvent *e);
  virtual void mouseDoubleClickEvent(QMouseEvent *e);
  virtual void mouseMoveEvent(QMouseEvent *e);
  virtual void wheelEvent(QWheelEvent *e);
 
public slots:
  virtual void last_month();
  virtual void last_week();
  virtual void last_day();
  virtual void last_hour();
  virtual void zoomIn();
  virtual void zoomOut();

 private:
  bool fetchAllData();
  void drawAll();
  void minmax();
  void drawHeader(const QRect &rect);
  void drawXBase(QPainter &paint, const QRect &rect, time_t off, 
	time_t major, time_t minor, const char *format, bool center);
  void drawXMonth(QPainter &paint, const QRect &rect);
  void drawXYear(QPainter &paint, const QRect &rect);
  void drawXGrid(const QRect &rect);
  void drawYGrid(const QRect &rect, const Range &range, double base);
  void drawGraph(const QRect &rect, double min, double max);

  // rrd-data
  std::vector<double> avg_data, min_data, max_data;
  std::string file;
  std::string ds;
  std::string name;
  bool data_is_valid;
  time_t start;	// real start of data (from rrd_fetch)
  time_t end;	// real end of data (from rrd_fetch)
  time_t span;	// should-be size of data (may differ from end-start)
  time_t tz_off; // offset of the local timezone from GMT
  unsigned long step;

  // technical helpers
  Range y_range;
  double base;
  int origin_x, origin_y;
  time_t origin_start, origin_end;

  // widget-data
  QFont font;
  QPixmap offscreen;
  QRect graphrect;
  int header_y, label_y1, label_y2;
  QColor color_major, color_minor, color_graph_bg;
  QColor color_minmax, color_line;
};

inline void Graph::last_month()
{
  last(3600*24*31);
}

inline void Graph::last_week()
{
  last(3600*24*7);
}

inline void Graph::last_day()
{
  last(3600*24);
}

inline void Graph::last_hour()
{
  last(3600);
}

inline void Graph::zoomIn()
{
  zoom(1.0/1.259921050);
}

inline void Graph::zoomOut()
{
  zoom(1.259921050);
}

inline void Graph::wheelEvent(QWheelEvent *e)
{
  if (e->delta() < 0)
    zoom(1.259921050);
  else
    zoom(1.0/1.259921050);
}

inline QSize Graph::sizeHint() const
{
  return QSize(640, 480);
}

#endif
