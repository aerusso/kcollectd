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
#include <sstream>
#include <iomanip>

#include <rrd.h>

#include <qpainter.h>
#include <qpointarray.h>
#include <qrect.h>

#include <kglobalsettings.h>
#include <klocale.h>

#include "graph.moc"

// definition of NaN in Y_Range
const double Range::NaN = std::numeric_limits<double>::quiet_NaN();


static void get_rrd_data (const std::string &file, const std::string &ds, 
      time_t *start, time_t *end, unsigned long *step, const char *type, 
      std::vector<double> *result)
{
  int argc = 9;
  char *argv[argc];
  unsigned long ds_cnt = 0;
  char **ds_name;
  rrd_value_t *data;
  char buffer[64];
  int status;

  argv[0] = strdup("fetch");
  argv[1] = strdup("--start");
  sprintf(buffer, "%ld", *start);
  argv[2] = strdup(buffer);
  argv[3] = strdup("--end");
  sprintf(buffer, "%ld", *end);
  argv[4] = strdup(buffer);
  argv[5] = strdup("--resolution");
  sprintf(buffer, "%ld", *step);
  argv[6] = strdup(buffer);
  argv[7] = strdup(file.c_str());
  argv[8] = strdup(type);
  
  status = rrd_fetch(argc, argv, start, end, step, &ds_cnt, &ds_name, &data);
  if (status != 0) {
    std::cerr << "get_rrd_data: rrd_fetch failed.\n";
    return;
  }

  result->clear();

  const unsigned long length = (*end - *start) / *step;

  for(unsigned int i=0; i<ds_cnt; ++i) {
    if (ds != ds_name[i])
      continue;
    
    for (unsigned int n = 0; n < length; ++n) 
      result->push_back (data[n * ds_cnt + i]);
    break;
  }

  for(int i=0; i<argc; ++i) 
    free(argv[i]);
}

static bool si_char(double d, std::string &s, double &m)
{
  const struct {
    double factor;
    const char *si_char;
  } si_table[] = {
    { 1e-18, "a" },
    { 1e-15, "f" },
    { 1e-12, "p" },
    { 1e-9,  "n" },
    { 1e-6,  "µ" },
    { 1e-3,  "m" },
    { 1,     ""  },
    { 1e3,   "k" },
    { 1e6,   "M" },
    { 1e9,   "G" },
    { 1e12,  "T" },
    { 1e15,  "P" },
    { 1e18,  "E" },
    { 1e21,  0   },
  };
  const int tablesize = sizeof(si_table)/sizeof(*si_table);

  int i;
  for(i=0; i < tablesize; ++i) {
    if (d < si_table[i].factor) break;
  }
  if (i == 0 || i == tablesize) {
    m = 1.0;
    s = "";
    return false;
  } else {
    --i;
    m = si_table[i].factor;
    s = si_table[i].si_char;
    return true;
  }
}

static std::string si_number(double d, int p, const std::string &s, double m)
{
  std::ostringstream os;
  os << std::setprecision(p) << d/m;
  if (!s.empty())
    os << " " << s;
  return os.str();
}

/**
 *
 */
Graph::Graph(QWidget *parent, const char *name) :
  QFrame(parent, name), data_is_valid(false), 
  end(time(0)), span(3600*48), step(1), font(KGlobalSettings::generalFont())
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
  end(time(0)), span(3600*48), step(1), font(KGlobalSettings::generalFont())
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

void Graph::setup(const char *filei, const char *dsi)
{
  data_is_valid = false;
  avg_data.clear();
  min_data.clear();
  max_data.clear();

  end = time(0);
  span = 86400;

  file = filei;
  ds = dsi;

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

  std::ostringstream os;

  char buffer[50];
  strftime(buffer, sizeof(buffer)-1, 
	i18n("%Y-%m-%d %H:%M:%S"), localtime(&start));
  os << "from " << buffer << " to ";
  strftime(buffer, sizeof(buffer)-1, 
	i18n("%Y-%m-%d %H:%M:%S"), localtime(&end));
  os << buffer;
  
  const QFontMetrics fontmetric(font);
  fontmetric.width(os.str());
  paint.drawText((rect.left()+rect.right())/2-fontmetric.width(os.str())/2, 
	fontmetric.ascent() + 2 , os.str());
}

void Graph::drawXBaseGrid(QPainter &paint, const QRect &rect, 
      time_t major, time_t minor, const char *format, bool center)
{
  const QFontMetrics fontmetric(font);
  int label_y = rect.bottom() + 4 + fontmetric.ascent();
  
  // setting up linear mappings
  const linMap xmap(start, rect.left(), end, rect.right());

  // draw minor lines
  if(minor) {
    time_t mmin = ((start + minor - tz_off) / minor) * minor + tz_off;
    time_t mmax = ((end - tz_off) / minor) * minor + tz_off;
    paint.setPen(QColor(80, 65, 34));
    for(time_t i = mmin; i <= mmax; i += minor) {
      paint.drawLine(xmap(i), rect.top(), xmap(i), rect.bottom());
    }
  }

  // draw major lines
  time_t min = ((start + major - tz_off) / major) * major + tz_off;
  time_t max = ((end - tz_off) / major) * major + tz_off;
  paint.setPen(QColor(140, 115, 60));
  for(time_t i = min; i <= max; i += major) {
    paint.drawLine(xmap(i), rect.top(), xmap(i), rect.bottom());
  }

  // draw labels
  paint.setPen(QColor(0, 0, 0));
  for(time_t i = min; i <= max; i += major) {
    char label[50];
    if(strftime(label, sizeof(label), i18n(format), localtime(&i))) { 
      if (center) {
	paint.drawText(xmap(i + major / 2) - fontmetric.width(label) / 2, 
	      label_y, label);
      } else {
	paint.drawText(xmap(i) - fontmetric.width(label) / 2, label_y, label);
      } 
    }
  }
}

void Graph::drawXGrid(const QRect &rect)
{
  const time_t min = 60;
  const time_t hour = 3600;
  const time_t day = 24*hour;
  const time_t week = 7*day;
  const time_t month = 31*day;
  const time_t year = 365*day;

  struct {
    time_t maxspan;
    time_t major;
    time_t minor;
    const char *format;
    bool center;
  } axe_params[] = {
    {    day,      1*min,       10,      "%H:%M",    false  },
    {    day,      2*min,       30,      "%H:%M",    false  },
    {    day,      5*min,      min,      "%H:%M",    false  },
    {    day,     10*min,      min,      "%H:%M",    false  },
    {    day,     30*min,   10*min,      "%H:%M",    false  },
    {    day,       hour,   10*min,      "%H:%M",    false  },
    {    day,     2*hour,   30*min,      "%H:%M",    false  },
    {    day,     3*hour,     hour,      "%H:%M",    false  },
    {    day,     6*hour,     hour,      "%H:%M",    false  },
    {    day,    12*hour,   3*hour,      "%H:%M",    false  },
    {   week,    12*hour,   3*hour,      "%a %H:%M", false  },
    {   week,        day,   3*hour,      "%a",       true   },
    {   week,      2*day,   6*hour,      "%a",       true   },
    {  month,        day,        0,      "%a %d",    true   },
    {  month,        day,        0,      "%d",       true   },
    {      0,          0,        0,      0,          false  }
  };

  QPainter paint(&offscreen);
  const QFontMetrics fontmetric(font);

  time_t time_span = end-start;
  
  if (time_span < month) {
    for(int i=0; axe_params[i].maxspan; ++i) {
      if ((end-start) < axe_params[i].maxspan) {
	char label[50];
	time_t now = time(0);
	if(strftime(label, sizeof(label), axe_params[i].format, gmtime(&now))) {
	  const int textwidth = fontmetric.width(label) 
	    * (end-start)/axe_params[i].major * 3 / 2;
	  if (textwidth < rect.width()) {
	    drawXBaseGrid(paint, rect, 
		  axe_params[i].major, axe_params[i].minor, 
		  axe_params[i].format, axe_params[i].center);
	    break;
	  }
	}
      }
    }
  } else if (time_span < 6*month) {
  } else if (time_span < year) {
  } else {
  }
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
  paint.setPen(QColor(80, 65, 34));
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
  paint.setPen(QColor(140, 115, 60));
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
    const QColor rc(0, 100, 0);
    paint.setPen(rc);
    paint.setBrush(QBrush(rc));
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
    paint.setPen(Qt::green);
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
  paint.fillRect(graphrect, Qt::black);

  drawHeader(graphrect);
  drawYGrid(graphrect, y_range, base);
  drawXGrid(graphrect);

  drawGraph(graphrect, y_range.min(), y_range.max());

  // copy to screen
  bitBlt(this, contentsRect().left(), contentsRect().top(), &offscreen, 0, 0);
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
  time_t time_center = end - span / 2;  
  span *= factor;
  end = time_center + (span / 2);

  const time_t now = time(0);
  if (end > now + (span * 2) / 3 )
    end = now + (span * 2) / 3;

  data_is_valid = false;
  drawAll();
}
