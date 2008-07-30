/*
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

#ifndef GUI_H
#define GUI_H

#include <string>
#include <iostream>
#include <vector>

#include <qwidget.h>

class QLabel;
class Graph;
class KListView;

class KCollectdGui : public QWidget
{
  Q_OBJECT;
public:
  KCollectdGui(QWidget *parent=0, const char *name=0);

public slots:  
  void selectionChanged();
  
public:
  KListView *listview;
  Graph *graph;
};

#endif
