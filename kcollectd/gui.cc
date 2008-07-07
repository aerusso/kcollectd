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

#include <klistview.h>

#include "graph.h"
#include "gui.moc"

KCollectdGui::KCollectdGui(QWidget *parent, const char *name)
  : QWidget(parent, name)
{
  QHBoxLayout *hbox = new QHBoxLayout(this, 0, 4);

  listview = new KListView(this);
  listview->addColumn("Sensordata");
  listview->setRootIsDecorated(true);
  listview->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  connect(listview, SIGNAL(executed(QListViewItem *)), 
	SLOT(selectionChanged(QListViewItem *)));
  hbox->addWidget(listview);

  QVBoxLayout *vbox = new QVBoxLayout(hbox);
  graph = new Graph(this);
  vbox->addWidget(graph);

  label = new QLabel(this);
  vbox->addWidget(label);

  hbox->activate();
}

void KCollectdGui::selectionChanged(QListViewItem * item)
{
  if (item && item->text(1)) {
    graph->setup(item->text(2), item->text(0));
    label->setText(QString(item->text(1)));
    graph->update();
  }
}
