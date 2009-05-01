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

#include <QLayout>
#include <QLabel>
#include <QWidget>
#include <QTreeWidget>

#include <KPushButton>
#include <KIconLoader>
#include <KGlobal>
#include <KLocale>
#include <QWhatsThis>

#include "graph.h"
#include "gui.moc"

KCollectdGui::KCollectdGui(QWidget *parent)
  : QWidget(parent)
{
  QHBoxLayout *hbox = new QHBoxLayout(this);

  listview = new QTreeWidget();
  listview->setColumnCount(1);
  listview->setHeaderLabels(QStringList(i18n("Sensordata")));
  listview->setRootIsDecorated(true);
  //QT4 listview->setShowSortIndicator(true);
  listview->setSelectionMode(QAbstractItemView::MultiSelection);
  listview->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  hbox->addWidget(listview);

  QVBoxLayout *vbox = new QVBoxLayout();
  hbox->addLayout(vbox);
  graph = new Graph();
  vbox->addWidget(graph);

  QHBoxLayout *hbox2 = new QHBoxLayout();
  vbox->addLayout(hbox2);
  KPushButton *last_month = new KPushButton(i18n("last month"));
  hbox2->addWidget(last_month);
  KPushButton *last_week = new KPushButton(i18n("last week"));
  hbox2->addWidget(last_week);
  KPushButton *last_day = new KPushButton(i18n("last day"));
  hbox2->addWidget(last_day);
  KPushButton *last_hour = new KPushButton(i18n("last hour"));
  hbox2->addWidget(last_hour);
  KPushButton *zoom_in = new KPushButton(KIcon("zoom-in"), QString());
  zoom_in->setToolTip(i18n("increases magnification"));
  zoom_in->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  hbox2->addWidget(zoom_in);
  KPushButton *zoom_out = new KPushButton(KIcon("zoom-out"), QString());
  zoom_out->setToolTip(i18n("reduces magnification"));
  zoom_out->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  hbox2->addWidget(zoom_out);
  KPushButton *auto_update = new KPushButton(KIcon("chronometer"), QString());
  auto_update->setToolTip(i18n("toggle automatic update-and-follow mode."));
  auto_update->setWhatsThis(i18n("<p>This button toggles the "
	      "automatic update-and-follow mode</p>"
	      "<p>The automatic update-and-follow mode updates the graph "
	      "every ten seconds. "
	      "In this mode the graph still can be zoomed, but always displays "
	      "<i>now</i> near the right edge and "
	      "can not be scrolled any more.<br />"
	      "This makes kcollectd some kind of status monitor.</p>"));
  auto_update->setCheckable(true);
  auto_update->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  hbox2->addWidget(auto_update);

  connect(listview, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
  connect(last_month, SIGNAL(clicked()), graph, SLOT(last_month()));
  connect(last_week, SIGNAL(clicked()), graph, SLOT(last_week()));
  connect(last_day, SIGNAL(clicked()), graph, SLOT(last_day()));
  connect(last_hour, SIGNAL(clicked()), graph, SLOT(last_hour()));
  connect(zoom_in, SIGNAL(clicked()), graph, SLOT(zoomIn()));
  connect(zoom_out, SIGNAL(clicked()), graph, SLOT(zoomOut()));
  connect(auto_update, SIGNAL(toggled(bool)), 
	graph, SLOT(autoUpdate(bool)));
}

void KCollectdGui::selectionChanged()
{
  std::vector<Graph::datasource> list;

  QTreeWidgetItemIterator i(listview, QTreeWidgetItemIterator::Selected);
  for ( ;*i; ++i) {
    Graph::datasource l;
    l.rrd = (*i)->text(2);
    l.ds = (*i)->text(3);
    l.label = (*i)->text(1);
    list.push_back(l);
  }

  graph->setup(list);
  graph->update();
}
