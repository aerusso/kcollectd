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
#include <vector>

#include <QFrame>
#include <QPixmap>
#include <QRect>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>

#include "misc.h"

class time_iterator;

class GraphInfo
{
public:

  struct datasource {
    QString rrd;
    QString ds;
    QString label;
    std::vector<double> avg_data, min_data, max_data;
  };

  void add(const QString &rrd, const QString &ds, const QString &label);
  void clear() { dslist.clear(); }
  size_t size() const { return dslist.size(); }
  Range minmax();
  Range minmax_adj(double *base);

  

  // iterators pointing to datasources
  typedef std::vector<datasource>::iterator iterator;
  typedef std::vector<datasource>::const_iterator const_iterator;

  iterator begin() { return dslist.begin(); }
  iterator end()   { return dslist.end(); }
  const_iterator begin() const { return dslist.begin(); } 
  const_iterator end() const   { return dslist.end(); }

private:
  
  std::vector<datasource> dslist;
};

/**
 *
 */
class Graph : public QFrame
{
  Q_OBJECT;
 public:

  typedef std::vector<GraphInfo> graph_list;

  Graph(QWidget *parent=0);
  Graph(QWidget *parent, const std::string &rrd, const std::string &ds, 
	const char *name=0);

  void clear();
  GraphInfo &add(const QString &rrd, const QString &ds, const QString &label);

  virtual QSize sizeHint() const;
  virtual void paintEvent(QPaintEvent *ev);

  virtual void last(time_t span);
  virtual void zoom(double factor);
  virtual void mousePressEvent(QMouseEvent *e);
  virtual void mouseReleaseEvent(QMouseEvent *e);
  virtual void mouseDoubleClickEvent(QMouseEvent *e);
  virtual void mouseMoveEvent(QMouseEvent *e);
  virtual void wheelEvent(QWheelEvent *e);
  virtual void timerEvent(QTimerEvent *event);
				    
public slots:
  virtual void last_month();
  virtual void last_week();
  virtual void last_day();
  virtual void last_hour();
  virtual void zoomIn();
  virtual void zoomOut();
  virtual void autoUpdate(bool active);

 private:
  bool fetchAllData();
  void drawAll();
  void drawLabel(QPainter &paint, int left, int right, int pos, 
	const GraphInfo &ginfo);
  void drawFooter(QPainter &paint, int left, int right);
  void drawHeader(QPainter &paint);
  void drawYLines(QPainter &paint, const QRect &rect, 
	const Range &y_range, double base, QColor color);
  void drawYLabel(QPainter &paint, const QRect &rect, 
	const Range &range, double base);
  void drawXLines(QPainter &paint, const QRect &rect, 
	time_iterator it, QColor color);
  void drawXLabel(QPainter &paint, int y, int left, int right, 
	time_iterator it, QString format, bool center);
  void findXGrid(int width, QString &format, bool &center, 
       time_iterator &minor_x, time_iterator &major_x, time_iterator &label_x );
  void drawGraph(QPainter &paint, const QRect &rect, const GraphInfo &gi, 
	double min, double max);

  // rrd-data
  graph_list glist;
  bool data_is_valid;
  time_t start;		// user set start of graph
  time_t span;		// user-set span of graph
  time_t data_start;	// real start of data (from rrd_fetch)
  time_t data_end;	// real end of data (from rrd_fetch)
  time_t tz_off; 	// offset of the local timezone from GMT
  unsigned long step;

  // technical helpers
  int origin_x, origin_y;
  time_t origin_start, origin_end;

  // widget-data
  QFont font, header_font, small_font;
  QPixmap offscreen;
  QRect graphrect;
  int label_y1, label_y2;
  QColor color_major, color_minor, color_graph_bg;
  QColor color_minmax[8], color_line[8];

  // Auto-Update
  int autoUpdateTimer;
  time_t timer_diff;
};

/**
 * add a datasource to the GraphInfo
 */
inline void 
GraphInfo::add(const QString &rrd, const QString &ds, const QString &label)
{
  datasource new_ds;
  new_ds.rrd = rrd;
  new_ds.ds = ds;
  new_ds.label = label;
  dslist.push_back(new_ds);
}

/**
 * 
 */
inline GraphInfo &
Graph::add(const QString &rrd, const QString &ds, const QString &label)
{
  GraphInfo gi;
  gi.add(rrd, ds, label);
  glist.push_back(gi);
  data_is_valid = false;
  return glist.back();
}

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
