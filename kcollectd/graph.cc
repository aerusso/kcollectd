/* 
 Constructs a font object that uses the application's default font. 
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

#include <qpainter.h>
#include <qpointarray.h>
#include <qrect.h>

#include <klocale.h>

#include "rrd_interface.h"
#include "misc.h"
#include "timeaxis.h"
#include "graph.moc"

/**
 *
 */
Graph::Graph(QWidget *parent, const char *name) :
  QFrame(parent, name), data_is_valid(false), 
  end(time(0)), span(3600*24), step(1), font(QFont()),
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

/** 
 * get average, min and max data
 *
 * set start end to the values get_rrd_data returns
 * don't change span, because it can shrink
 */
bool Graph::fetchAllData ()
{
  if (data_is_valid)
    return (true);
  
  if (dslist.empty())
    return (false);

  time_t copy_start, copy_end;

  std::vector<datasource>::iterator i;

  for(i = dslist.begin(); i != dslist.end(); ++i) {
    const char *file = i->rrd.ascii();
    const char *ds = i->ds.ascii();    
    copy_start = end - span;
    copy_end = end;
    step = 1;
    get_rrd_data (file, ds, &copy_start, &copy_end, &step, "MIN", &i->min_data);
    
    copy_start = end-span;
    copy_end = end;
    step = 1;
    get_rrd_data (file, ds, &copy_start, &copy_end, &step, "MAX", &i->max_data);
    
    copy_start = end - span;
    copy_end = end;
    step = 1;
    get_rrd_data (file, ds, &copy_start, &copy_end, &step, "AVERAGE", &i->avg_data);
  }    
  data_is_valid = true;
  end = copy_end;
  start = copy_start;

  return (true);
}

/**
 * set up graph-widget with a rrd-file and a datasource.
 *
 * optionally a label/name for the Graph can be given.
 */
void Graph::setup(std::vector<datasource> &list)
{
  data_is_valid = false;

  dslist = list;
}

/**
 * determine min and max values for a graph and save it into y_range
 */

void Graph::minmax(const datasource &ds)
{
  if (ds.avg_data.empty()) {
    return;
  }
  
  const std::vector<double> &avg_data = ds.avg_data;
  const std::vector<double> &min_data = ds.min_data;
  const std::vector<double> &max_data = ds.max_data;
  
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

void Graph::drawHeader(int left, int right, int pos, const QString &text)
{
  QPainter paint(&offscreen);
  const QFontMetrics fontmetric(font);

  // header
  paint.drawText((left+right)/2-fontmetric.width(text)/2, pos, text);
}

void Graph::drawFooter(int left, int right)
{
  QPainter paint(&offscreen);
  const QFontMetrics fontmetric(font);

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

  QString buffer_from = Qstrftime(format, localtime(&start));
  QString buffer_to = Qstrftime(format, localtime(&end));
  QString label = QString(i18n("from %1 to %2"))
    .arg(buffer_from)
    .arg(buffer_to);
  
  fontmetric.width(label);
  paint.drawText((left+right)/2-fontmetric.width(label)/2, 
	label_y2, label);
}

void Graph::drawXLines(const QRect &rect, time_iterator i, QColor color)
{
  if (!i.valid()) return;

  // setting up linear mappings
  const linMap xmap(start, rect.left(), end, rect.right());
 
  // if lines are to close draw nothing
  if (i.interval() * xmap.m() < 3) 
    return;

  // draw lines
  QPainter paint(&offscreen);
  paint.setPen(color);
  for(; *i <= end; ++i) {
    int x = xmap(*i);
    paint.drawLine(x, rect.top(), x, rect.bottom());
  }
}

void Graph::drawXLabel(int left, int right, 
      time_iterator i, QString format, bool center)
{
  if (!i.valid()) return;

  const QFontMetrics fontmetric(font);
  QPainter paint(&offscreen);
  
  // setting up linear mappings
  const linMap xmap(start, left, end, right);

  // draw labels
  paint.setPen(foregroundColor());
  if (center) --i;
  for(; *i <= end; ++i) {
    // special handling for localtime/mktime on DST
    time_t t = center ? *i + i.interval() / 2 : *i;
    tm tm;
    localtime_r(&t, &tm);
    if(QString label = Qstrftime(i18n(format), &tm)) {
      const int width = fontmetric.width(label);
      int x = center 
	? xmap(*i + i.interval() / 2) - width / 2
	: xmap(*i) - width / 2;
	  
      if (x > left && x + width < right)
	paint.drawText(x, label_y1, label);
    }
  }
}

void Graph::findXGrid(int width, QString &format, bool &center,
      time_iterator &minor_x, time_iterator &major_x, time_iterator &label_x)
{
  const time_t min = 60;
  const time_t hour = 3600;
  const time_t day = 24*hour;
  const time_t week = 7*day;
  const time_t month = 31*day;
  const time_t year = 365*day;
  
  enum bla { align_tzalign, align_week, align_month };
  struct {
    time_t maxspan;
    time_t major;
    time_t minor;
    const char *format;
    bool center;
    bla align;
  } axis_params[] = {
    {   day,   1*min,     10, "%H:%M",              false, align_tzalign },
    {   day,   2*min,     30, "%H:%M",              false, align_tzalign },
    {   day,   5*min,    min, "%H:%M",              false, align_tzalign },
    {   day,  10*min,    min, "%H:%M",              false, align_tzalign },
    {   day,  30*min, 10*min, "%H:%M",              false, align_tzalign },
    {   day,    hour, 10*min, "%H:%M",              false, align_tzalign },
    {   day,  2*hour, 30*min, "%H:%M",              false, align_tzalign },
    {   day,  3*hour,   hour, "%H:%M",              false, align_tzalign },
    {   day,  6*hour,   hour, "%H:%M",              false, align_tzalign },
    {   day, 12*hour, 3*hour, "%H:%M",              false, align_tzalign },
    {  week, 12*hour, 3*hour, "%a %H:%M",           false, align_tzalign },
    {  week,     day, 3*hour, "%a",                 true,  align_tzalign },
    {  week,   2*day, 6*hour, "%a",                 true,  align_tzalign },
    { month,     day, 6*hour, "%a %d",              true,  align_tzalign },
    { month,     day, 6*hour, "%d",                 true,  align_tzalign },
    {  year,    week,    day, I18N_NOOP("week %V"), true,  align_week    },
    {  year,   month,    day, "%b",                 true,  align_month   },
    {     0,       0,      0, 0,                    true,  align_tzalign },
  };

  const QFontMetrics fontmetric(font);
  const time_t time_span = end - start;
  const time_t now = time(0);

  for(int i=0; axis_params[i].maxspan; ++i) {
    if (time_span < axis_params[i].maxspan) {
      if(QString label = Qstrftime(axis_params[i].format, localtime(&now))) {
	const int textwidth = fontmetric.width(label) 
	  * time_span / axis_params[i].major * 3 / 2;
	if (textwidth < width) {
	  switch(axis_params[i].align) {
	  case align_tzalign:
	    minor_x.set(start, axis_params[i].minor);
	    major_x.set(start, axis_params[i].major);
	    label_x.set(start, axis_params[i].major);
	    format = axis_params[i].format;
	    center = axis_params[i].center;
	    break;
	  case align_week:
	    minor_x.set(start, day);
	    major_x.set(start, 1, time_iterator::weeks);
	    label_x.set(start, 1, time_iterator::weeks);
	    format = axis_params[i].format;
	    center = axis_params[i].center;
	    break;
	  case align_month:
	    minor_x.set(start, axis_params[i].minor);
	    major_x.set(start, 1, time_iterator::month);
	    label_x.set(start, 1, time_iterator::month);
	    format = axis_params[i].format;
	    center = axis_params[i].center;
	    break;
	  }
	  return;
	}
      }
    }
  }
  if(QString label = Qstrftime("%Y", localtime(&now))) {
    const int textwidth = fontmetric.width(label) * 3 / 2;
    // fixed-point calculation with 16 bit fraction.
    int num = (time_span * textwidth * 16) / ( year * width);
    if (num < 16 ) {
      minor_x.set(start, 1, time_iterator::month);
      major_x.set(start, 1, time_iterator::years);
      label_x.set(start, 1, time_iterator::years);
      format = "%Y";
      center = true;
    } else {
      minor_x.set(start, 1, time_iterator::years);
      major_x.set(start, (num+15)/16, time_iterator::years);
      label_x.set(start, (num+15)/16, time_iterator::years);
      format = "%Y";
      center = false;
    }
  }
}

void Graph::drawYLabel(const QRect &rect, const Range &y_range, double base)
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
}

void Graph::drawYLines(const QRect &rect, const Range &y_range, double base, 
      QColor color)
{
  // setting up linear mappings
  const linMap ymap(y_range.min(), rect.bottom(), y_range.max(), rect.top());

  const QFontMetrics fontmetric(font);

  QPainter paint(&offscreen);

  // draw lines
  paint.setPen(color);
  if (base * -ymap.m() < 5) base = base/5;
  if (base * -ymap.m() < 5) base = base/2;
  if (base * -ymap.m() > 5) {
    double min = ceil(y_range.min()/base)*base;
    double max = y_range.max();
    for(double i = min; i < max; i += base) {
      paint.drawLine(rect.left(), ymap(i), rect.right(), ymap(i));
    }
  }
}

/**
 * draw the graph itself
 */
void Graph::drawGraph(const QRect &rect, const datasource &ds, 
      double min, double max)
{
  QPainter paint(&offscreen);

  const std::vector<double> &avg_data = ds.avg_data;
  const std::vector<double> &min_data = ds.min_data;
  const std::vector<double> &max_data = ds.max_data;


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
    paint.setBrush(QBrush(QColor(qRgba(0,100,0,70))));
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
}

/**
 * draw the widgets contents.
 *
 * this covers a grid, the graph istself, x- and y-label and a header
 */
void Graph::drawAll()
{
  const int numgraphs =  dslist.size();
  if (!numgraphs) return;

  if (!data_is_valid)
    fetchAllData ();  

  // resize offscreen-map to widget-size
  offscreen.resize(contentsRect().width(), contentsRect().height());

  // clear
  QPainter paint(&offscreen);
  paint.eraseRect(0, 0, contentsRect().width(), contentsRect().height());

  // margin calculations
  // place for labels at the left and two line labels below
  const QFontMetrics fontmetric(font);
  const int labelheight = fontmetric.lineSpacing();
  const int labelwidth = fontmetric.boundingRect("888.888 M").width();
  const int marg = 4; // distance between labels and graph

  graphrect.setRect(contentsRect().left() + labelwidth + marg,
	contentsRect().top(),
	contentsRect().width() - labelwidth - 2 * marg,
	contentsRect().height() - 2 * labelheight - marg);

  // positions of labels and headers
  label_y1 = graphrect.bottom() + 2 + fontmetric.ascent();
  label_y2 = label_y1 + labelheight;

  time_iterator minor_x, major_x, label_x;
  QString format_x;
  bool center_x;
  findXGrid(graphrect.width(), format_x, center_x, minor_x, major_x, label_x);
  drawFooter(graphrect.left(), graphrect.right());
  drawXLabel(graphrect.left(), graphrect.right(), label_x, format_x, center_x);
 
  const int graphheight = graphrect.height() / numgraphs;
  const int panelheight = graphheight - 2 - marg - labelheight;
  int n = 0;
  for(std::vector<datasource>::iterator i = dslist.begin();
      i != dslist.end(); ++n, ++i) {
    
    // y-scaling
    minmax(*i);
    if (!y_range.isValid())
      continue;

    // 
    QRect panelrect(graphrect.left(), 
	  graphrect.top() + n * graphheight + 2 + labelheight, 
	  graphrect.width(),
	  panelheight);

    // black graph-background
    paint.fillRect(panelrect, color_graph_bg);
    
    // draw minor, major, graph
    drawHeader(panelrect.left(), panelrect.right(), 
	  panelrect.top() - fontmetric.descent() - marg/2, i->label);
    drawXLines(panelrect, minor_x, color_minor);
    drawYLines(panelrect, y_range, base/10, color_minor);
    drawXLines(panelrect, major_x, color_major);
    drawYLines(panelrect, y_range, base, color_major);
    drawYLabel(panelrect, y_range, base);
    drawGraph(panelrect, *i, y_range.min(), y_range.max());
  }
  
  // copy to screen
  QPainter(this).drawPixmap(contentsRect(), offscreen);
}

/**
 * Qt (re)paint event
 */
void Graph::paintEvent(QPaintEvent *e)
{
  QFrame::paintEvent(e);
  drawAll();
}

/**
 * Qt mouse-press-event
 */
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

/**
 * Qt mouse-event
 *
 * handle dragging of mouse in graph
 */
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

/**
 * set the graph to display the last new_span seconds
 */
void Graph::last(time_t new_span)
{
  end = time(0);
  span = new_span;
  start = end - span;
  data_is_valid = false;
  drawAll();
}

/**
 * zoom graph with factor
 */
void Graph::zoom(double factor)
{
  // don't zoom to wide
  if (factor < 1 && span*factor < width()) return;

  time_t time_center = end - span / 2;  
  if (time_center < 0) return;
  span *= factor;
  end = time_center + (span / 2);

  const time_t now = time(0);
  if (end > now + (span * 2) / 3 )
    end = now + (span * 2) / 3;

  data_is_valid = false;
  drawAll();
}
