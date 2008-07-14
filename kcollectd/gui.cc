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

#include <iostream>

#include <qlayout.h>
#include <qlabel.h>
#include <qwidget.h>

#include <kpushbutton.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <klocale.h>

#include "graph.h"
#include "gui.moc"

KCollectdGui::KCollectdGui(QWidget *parent, const char *name)
  : QWidget(parent, name)
{
  QHBoxLayout *hbox = new QHBoxLayout(this, 4, 4);

  listview = new KListView(this);
  listview->addColumn(i18n("Sensordata"));
  listview->setRootIsDecorated(true);
  listview->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  hbox->addWidget(listview);

  QVBoxLayout *vbox = new QVBoxLayout(hbox);
  graph = new Graph(this);
  vbox->addWidget(graph);

  QHBoxLayout *hbox2 = new QHBoxLayout(vbox, 4);
  KPushButton *last_month = new KPushButton(i18n("last month"), this);
  hbox2->addWidget(last_month);
  KPushButton *last_week = new KPushButton(i18n("last week"), this);
  hbox2->addWidget(last_week);
  KPushButton *last_day = new KPushButton(i18n("last day"), this);
  hbox2->addWidget(last_day);
  KPushButton *last_hour = new KPushButton(i18n("last hour"), this);
  hbox2->addWidget(last_hour);
  KPushButton *zoom_in = new KPushButton(BarIcon("viewmag+"), "", this);
  zoom_in->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  hbox2->addWidget(zoom_in);
  KPushButton *zoom_out = new KPushButton(BarIcon("viewmag-"), "", this);
  zoom_out->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  hbox2->addWidget(zoom_out);

  connect(listview, SIGNAL(executed(QListViewItem *)), 
	SLOT(selectionChanged(QListViewItem *)));
  connect(last_month, SIGNAL(clicked()), graph, SLOT(last_month()));
  connect(last_week, SIGNAL(clicked()), graph, SLOT(last_week()));
  connect(last_day, SIGNAL(clicked()), graph, SLOT(last_day()));
  connect(last_hour, SIGNAL(clicked()), graph, SLOT(last_hour()));
  connect(zoom_in, SIGNAL(clicked()), graph, SLOT(zoomIn()));
  connect(zoom_out, SIGNAL(clicked()), graph, SLOT(zoomOut()));

}

void KCollectdGui::selectionChanged(QListViewItem * item)
{
  if (item && item->text(1)) {
    graph->setup(item->text(2), item->text(0), item->text(1));
    graph->update();
  }
}
