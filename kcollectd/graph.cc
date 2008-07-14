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

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

#include <cstring>
#include <vector>
#include <cmath>
#include <limits>

#include <qpainter.h>
#include <qpointarray.h>
#include <qrect.h>

#include <kglobalsettings.h>
#include <klocale.h>

#include "rrd_interface.h"
#include "labeling.h"
#include "graph.moc"

// definition of NaN in Y_Range
const double Range::NaN = std::numeric_limits<double>::quiet_NaN();


/**
 *
 */
Graph::Graph(QWidget *parent, const char *name) :
  QFrame(parent, name), data_is_valid(false), 
  end(time(0)), span(3600*24), step(1), font(KGlobalSettings::generalFont()),
  color_major(140, 115, 60), color_minor(80, 65, 34), color_graph_bg(0, 0, 0),
  color_minmax(0, 100, 0), color_line(0, 255, 0)
{
  setFrameStyle(QFrame::GroupBoxPanel|QFrame::Plain);
  setMinimumWidth(300);
  setMinimumHeight(150);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  struct timezone tz;
  struct timeval tv;
  gettimeofday(&tv, &tz);
  tz_off = tz.tz_minuteswest * 60;
}

Graph::Graph(QWidget *parent, const std::string &rrd, const std::string &dsi,
      const char *name) :
  QFrame(parent, name), file(rrd), ds(dsi), data_is_valid(false), 
  end(time(0)), span(3600*24), step(1), font(KGlobalSettings::generalFont()),
  color_major(140, 115, 60), color_minor(80, 65, 34), color_graph_bg(0, 0, 0),
  color_minmax(0, 100, 0), color_line(0, 255, 0)
{
  setFrameStyle(QFrame::StyledPanel|QFrame::Plain);
  setMinimumWidth(300);
  setMinimumHeight(150);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  struct timezone tz;
  struct timeval tv;
  gettimeofday(&tv, &tz);
  tz_off = tz.tz_minuteswest * 60;

  drawAll();
}

/** 
 * get average, min and max data
 *
 * set start end to the values get_rrd_data returns
 * don't change span, because it can shrink
 */
bool Graph::fetchAllData (void)
{
  if (data_is_valid)
    return (true);
  
  if ((file == 0) || (ds == NULL))
    return (false);

  time_t copy_start = end - span;
  time_t copy_end = end;
  step = 1;
  get_rrd_data (file, ds, &copy_start, &copy_end, &step, "MIN", &min_data);

  copy_start = end-span;
  copy_end = end;
  step = 1;
  get_rrd_data (file, ds, &copy_start, &copy_end, &step, "MAX", &max_data);

  copy_start = end - span;
  copy_end = end;
  step = 1;
  get_rrd_data (file, ds, &copy_start, &copy_end, &step, "AVERAGE", &avg_data);

  data_is_valid = true;
  end = copy_end;
  start = copy_start;

  return (true);
}

void Graph::setup(const char *filei, const char *dsi, const char *labeli)
{
  data_is_valid = false;
  avg_data.clear();
  min_data.clear();
  max_data.clear();

  file = filei;
  ds = dsi;
  if (labeli)
    name = labeli;
  else
    name.clear();

  drawAll();
}

void Graph::minmax()
{
  if (avg_data.empty()) {
    return;
  }
  
  // determine min/max
  bool valid = false;
  const int size = avg_data.size();
  double min(std::numeric_limits<double>::max());
  double max(std::numeric_limits<double>::min());
  if (!avg_data.empty()) {
    for(int i=0; i<size; ++i) {
      if (isnan(avg_data[i])) continue;
      valid = true;
      if (min > avg_data[i]) min = avg_data[i];
      if (max < avg_data[i]) max = avg_data[i];
    }
  }
  if (!min_data.empty() && !max_data.empty()) {  
    for(int i=0; i<size; ++i) {
      if (isnan(min_data[i]) || isnan(max_data[i])) continue;
      valid = true;
      if (min > min_data[i]) min = min_data[i];
      if (max < min_data[i]) max = min_data[i];
      if (min > max_data[i]) min = max_data[i];
      if (max < max_data[i]) max = max_data[i];
    }
  }
  
  // no drawable data found
  if(!valid) return;
  
  if (min == max) {
    max += 1;
    min -=1;
  } else {
    // allign to sensible values
    base = pow(10, floor(log(max-min)/log(10)));
    min = floor(min/base)*base;
    max = ceil(max/base)*base;
    
    // setting some margin at top and bottom
    const double margin = 0.05 * (max-min);
    max += margin;
    min -= margin;
  }

  y_range = Range(min, max);
}

void Graph::drawHeader(const QRect &rect)
{
  QPainter paint(&offscreen);

  QString format;
  time_t time_span = end - start;
  if (time_span > 3600*24*356)
    format = i18n("%Y-%m");
  else if (time_span > 3600*24*31)
    format = i18n("%A %Y-%m-%d");
  else if (time_span > 3600*24)
    format = i18n("%A %Y-%m-%d %H:%M");
  else
    format = i18n("%A %Y-%m-%d %H:%M:%S");

  char buffer_from[50], buffer_to[50];
  strftime(buffer_from, sizeof(buffer_from), format, localtime(&start));
  strftime(buffer_to, sizeof(buffer_to), format, localtime(&end));
  QString label = QString(i18n("from %1 to %2"))
    .arg(buffer_from)
    .arg(buffer_to);
  
  const QFontMetrics fontmetric(font);
  fontmetric.width(label);
  paint.drawText((rect.left()+rect.right())/2-fontmetric.width(label)/2, 
	fontmetric.ascent() + 2 , label);
}

void Graph::drawXBase(QPainter &paint, const QRect &rect, 
      time_t off, time_t major, time_t minor, const char *format, bool center)
{
  const QFontMetrics fontmetric(font);
  int label_y = rect.bottom() + 4 + fontmetric.ascent();
  
  // setting up linear mappings
  const linMap xmap(start, rect.left(), end, rect.right());

  // draw minor lines
  if(minor) {
    time_t mmin = ((start + minor - off) / minor) * minor + off;
    paint.setPen(color_minor);
    for(time_t i = mmin; i <= end; i += minor) {
      paint.drawLine(xmap(i), rect.top(), xmap(i), rect.bottom());
    }
  }

  // draw major lines
  time_t min = ((start + major - off) / major) * major + off;
  paint.setPen(color_major);
  for(time_t i = min; i <= end; i += major) {
    paint.drawLine(xmap(i), rect.top(), xmap(i), rect.bottom());
  }

  // draw labels
  paint.setPen(KGlobalSettings::textColor());
  if (center)
    min = ((start - off) / major) * major + off;
  for(time_t i = min; i <= end; i += major) {
    char label[50];
    if(strftime(label, sizeof(label), i18n(format), localtime(&i))) {
      const int width = fontmetric.width(label);
      int x = center 
	? xmap(i + major / 2) - width / 2
	: xmap(i) - width / 2;
	  
      if (x > rect.left() && x + width < rect.right())
	paint.drawText(x, label_y, label);
    }
  }
}

inline static void next_month(struct tm &bt)
{
  ++bt.tm_mon;
  if (bt.tm_mon == 12) {
    bt.tm_mon = 0;
    ++bt.tm_year;
  }
}

void Graph::drawXMonth(QPainter &paint, const QRect &rect)
{
  const QFontMetrics fontmetric(font);
  int label_y = rect.bottom() + 4 + fontmetric.ascent();
  
  // setting up linear mappings
  const linMap xmap(start, rect.left(), end, rect.right());

  struct tm bt = *localtime(&start);
  bt.tm_sec = bt.tm_min = bt.tm_hour;
  bt.tm_mday = 1;
  
  int i = mktime(&bt);
  while (i <= end) {
    // draw major lines
    int x = xmap(i);
    if (x > rect.left()) {
      paint.setPen(color_major);
      paint.drawLine(x, rect.top(), x, rect.bottom());
    }
    // draw labels
    paint.setPen(KGlobalSettings::textColor());
    char label[50];
    if(strftime(label, sizeof(label), "%b", &bt)) {
      const int width = fontmetric.width(label);
      x =  xmap(i + 3600*24*30 / 2) - width / 2;
	  
      if (x > rect.left() && x + width < rect.right())
	paint.drawText(x, label_y, label);

      next_month(bt);
      i = mktime(&bt);
    }
  }
}

void Graph::drawXYear(QPainter &paint, const QRect &rect)
{
  const QFontMetrics fontmetric(font);
  int label_y = rect.bottom() + 4 + fontmetric.ascent();
  
  // setting up linear mappings
  const linMap xmap(start, rect.left(), end, rect.right());

  // minor lines
  struct tm bt = *localtime(&start);
  bt.tm_sec = bt.tm_min = bt.tm_hour;
  bt.tm_mday = 1;
  
  int i = mktime(&bt);
  while (i <= end) {
    int x = xmap(i);
    if (x > rect.left()) {
      paint.setPen(color_minor);
      paint.drawLine(x, rect.top(), x, rect.bottom());
    }
    next_month(bt);
    i = mktime(&bt);
  }
    
  bt = *localtime(&start);
  bt.tm_sec = bt.tm_min = bt.tm_hour;
  bt.tm_mday = 1;
  bt.tm_mon = 0;
  i = mktime(&bt);
  while (i <= end) {
    // draw major lines
    int x = xmap(i);
    if (x > rect.left()) {
      paint.setPen(color_major);
      paint.drawLine(x, rect.top(), x, rect.bottom());
    }
    // draw labels
    paint.setPen(KGlobalSettings::textColor());
    char label[50];
    if(strftime(label, sizeof(label), "%b", &bt)) {
      const int width = fontmetric.width(label);
      x =  xmap(i + 3600*24*30 / 2) - width / 2;
	  
      if (x > rect.left() && x + width < rect.right())
	paint.drawText(x, label_y, label);

      ++bt.tm_year;
      i = mktime(&bt);
    }
  }
}

static time_t week_align()
{
  time_t w = 7*24*3600;
  struct tm bt = *localtime(&w);
  bt.tm_sec = bt.tm_min = bt.tm_hour;
  bt.tm_mday -= bt.tm_wday - 1;
  return mktime(&bt);
}

void Graph::drawXGrid(const QRect &rect)
{
  const time_t min = 60;
  const time_t hour = 3600;
  const time_t day = 24*hour;
  const time_t week = 7*day;
  const time_t month = 31*day;
  const time_t year = 365*day;
  
  enum bla { align_noalign, align_week, align_month };
  struct {
    time_t maxspan;
    time_t major;
    time_t minor;
    const char *format;
    bool center;
    bla align;
  } axe_params[] = {
    {   day,   1*min,     10, "%H:%M",              false, align_noalign },
    {   day,   2*min,     30, "%H:%M",              false, align_noalign },
    {   day,   5*min,    min, "%H:%M",              false, align_noalign },
    {   day,  10*min,    min, "%H:%M",              false, align_noalign },
    {   day,  30*min, 10*min, "%H:%M",              false, align_noalign },
    {   day,    hour, 10*min, "%H:%M",              false, align_noalign },
    {   day,  2*hour, 30*min, "%H:%M",              false, align_noalign },
    {   day,  3*hour,   hour, "%H:%M",              false, align_noalign },
    {   day,  6*hour,   hour, "%H:%M",              false, align_noalign },
    {   day, 12*hour, 3*hour, "%H:%M",              false, align_noalign },
    {  week, 12*hour, 3*hour, "%a %H:%M",           false, align_noalign },
    {  week,     day, 3*hour, "%a",                 true,  align_noalign },
    {  week,   2*day, 6*hour, "%a",                 true,  align_noalign },
    { month,     day, 6*hour, "%a %d",              true,  align_noalign },
    { month,     day, 6*hour, "%d",                 true,  align_noalign },
    {  year,    week,    day, I18N_NOOP("week %U"), true,  align_week    },
    {  year,   month,    day, "%b",                 true,  align_month   },
    {     0,       0,      0, 0,                    true,  align_noalign },
  };

  QPainter paint(&offscreen);
  const QFontMetrics fontmetric(font);

  const time_t time_span = end-start;
  
  for(int i=0; axe_params[i].maxspan; ++i) {
    if (time_span < axe_params[i].maxspan) {
      char label[50];
      time_t now = time(0);
      if(strftime(label, sizeof(label), axe_params[i].format, 
		  localtime(&now))) {
	const int textwidth = fontmetric.width(label) 
	  * time_span / axe_params[i].major * 3 / 2;
	if (textwidth < rect.width()) {
	  switch(axe_params[i].align) {
	  case align_noalign:
	    drawXBase(paint, rect, tz_off, 
		  axe_params[i].major, axe_params[i].minor, 
		  axe_params[i].format, axe_params[i].center);
	    break;
	  case align_week: 
	    drawXBase(paint, rect, week_align(), 
		  axe_params[i].major, axe_params[i].minor, 
		  axe_params[i].format, axe_params[i].center);	    
	    break;
	  case align_month:
	    drawXMonth(paint, rect); 
	    break;
	  }
	  return;
	}
      }
    }
  }
  drawXBase(paint, rect, tz_off, year, month, "%Y", true);
}

void Graph::drawYGrid(const QRect &rect, const Range &y_range, double base)
{
  // setting up linear mappings
  const linMap ymap(y_range.min(), rect.bottom(), y_range.max(), rect.top());

  const QFontMetrics fontmetric(font);

  QPainter paint(&offscreen);

  // SI Unit for nice display
  std::string SI;
  double mag;
  si_char(y_range.max(), SI, mag);

  // draw labels
  double min = ceil(y_range.min()/base)*base;
  double max = floor(y_range.max()/base)*base;
  for(double i = min; i <= max; i += base) {
    const std::string label = si_number(i, 6, SI, mag);
    const int x = rect.left() - fontmetric.width(label) - 4;
    paint.drawText(x,  ymap(i), label);
  }

  // minor lines
  paint.setPen(color_minor);
  double minbase = base/10;
  if (minbase * -ymap.m() < 5) minbase = base/5;
  if (minbase * -ymap.m() < 5) minbase = base/2;
  if (minbase * -ymap.m() > 5) {
    double minmin = ceil(y_range.min()/minbase)*minbase;
    double minmax = floor(y_range.max()/minbase)*minbase;
    for(double i = minmin; i< minmax; i += minbase) {
      paint.drawLine(rect.left(), ymap(i), rect.right(), ymap(i));
    }
  }

  // major lines
  paint.setPen(color_major);
  if (base * -ymap.m() < 5) base = base/5;
  if (base * -ymap.m() < 5) base = base/2;
  if (base * -ymap.m() > 5) {
    double minmin = ceil(y_range.min()/base)*base;
    double minmax = floor(y_range.max()/base)*base;
    for(double i = minmin; i< minmax; i += base) {
      paint.drawLine(rect.left(), ymap(i), rect.right(), ymap(i));
    }
  }
}

void Graph::drawGraph(const QRect &rect, double min, double max)
{
  QPainter paint(&offscreen);

  if (avg_data.empty())
    return;

  const int size = avg_data.size();

  // setting up linear mappings
  const linMap xmap(0, rect.left(), size-1, rect.right());
  const linMap ymap(min, rect.bottom(), max, rect.top());

  // define once use many
  QPointArray points;

  // draw min/max backshadow
  if (!min_data.empty() && !max_data.empty()) {  
    paint.setPen(color_minmax);
    paint.setBrush(QBrush(color_minmax));
    for(int i=0; i<size; ++i) {
      while (i<size && (isnan(min_data[i]) || isnan(max_data[i]))) ++i;
      int l = i;
      while (i<size && !isnan(min_data[i]) && !isnan(max_data[i])) ++i;
      const int asize = i-l;
      points.resize(asize*2);
      int k;
      for(k=0; k<asize; ++k, ++l) {
	points.setPoint(k, xmap(l), ymap(min_data[l]));
      }
      --l;
      for(; k<2*asize; ++k, --l) {
	points.setPoint(k, xmap(l), ymap(max_data[l]));
      }
      paint.drawPolygon(points);
    }
  }

  // draw average
  if (!avg_data.empty()) {
    paint.setPen(color_line);
    for(int i=0; i<size; ++i) {
      while (i<size && isnan(avg_data[i])) ++i;
      int l = i;
      while (i<size && !isnan(avg_data[i])) ++i;
      const int asize = i-l;
      points.resize(asize);
      for(int k=0; k<asize; ++k, ++l) {
	points.setPoint(k, xmap(l), ymap(avg_data[l]));
      }
      paint.drawPolyline(points);
    }
  }
  paint.end();
}

void Graph::drawAll()
{
  if (!data_is_valid)
    fetchAllData ();  

  // resize offscreen-map to widget-size
  offscreen.resize(contentsRect().width(), contentsRect().height());

  // clear
  QPainter paint(&offscreen);
  paint.eraseRect(0, 0, contentsRect().width(), contentsRect().height());

  // margins
  const QFontMetrics fontmetric(font);
  const int labelheight = fontmetric.lineSpacing();
  const int labelwidth = fontmetric.boundingRect("888.888 M").width();

  // size of actual graph and draw
  // place for labels at the left and two line labels below
  const int marg = 4; // distance between labels and graph
  graphrect.setRect(labelwidth+marg, labelheight+marg, 
	contentsRect().width()-labelwidth-marg, 
	contentsRect().height()-3*labelheight-2*marg);

  // auto y-scale
  minmax();
  if (! y_range.isValid())
    return;

  // black graph-background
  paint.fillRect(graphrect, color_graph_bg);

  drawHeader(graphrect);
  drawYGrid(graphrect, y_range, base);
  drawXGrid(graphrect);

  drawGraph(graphrect, y_range.min(), y_range.max());

  // copy to screen
  QPainter(this).drawPixmap(contentsRect(), offscreen);
}

void Graph::paintEvent(QPaintEvent *e)
{
  QFrame::paintEvent(e);
  drawAll();
}

void Graph::mousePressEvent(QMouseEvent *e)
{
  origin_x = e->x ();
  origin_y = e->y ();

  origin_start = start;
  origin_end = end;
}

void Graph::mouseReleaseEvent(QMouseEvent *)
{
}

void Graph::mouseDoubleClickEvent(QMouseEvent *)
{
}

void Graph::mouseMoveEvent(QMouseEvent *e)
{
  if (e->state() != LeftButton && e->state() != MidButton) {
    e->ignore();
    return;
  }

  int x = e->x();
  int y = e->y();

  if ((x < graphrect.left()) || (x >= graphrect.right()))
    return;
  if ((y < 0) || (y >= height()))
    return;

  int offset = (x - origin_x) * (origin_end - origin_start) 
    / graphrect.width();

  end = origin_end - offset;
  const time_t now = time(0);
  if (end > now + span * 2 / 3 )
    end = now + span * 2 / 3;

  data_is_valid = false;
  drawAll();
}

void Graph::zoom(double factor)
{
  // don't zoom to wide
  if (span*factor < width()) return;

  time_t time_center = end - span / 2;  
  span *= factor;
  end = time_center + (span / 2);

  const time_t now = time(0);
  if (end > now + (span * 2) / 3 )
    end = now + (span * 2) / 3;

  data_is_valid = false;
  drawAll();
}
