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
#include <QWhatsThis>
#include <QXmlStreamWriter>
#include <QFile>
#include <QDomDocument>

#include <kactioncollection.h>
#include <KPushButton>
#include <KIconLoader>
#include <KGlobal>
#include <KLocale>
#include <KMainWindow>
#include <KMenuBar>
#include <KMenu>
#include <KStandardAction>
#include <KAction>
#include <KToggleAction>
#include <KFileDialog>
#include <kactioncollection.h>
#include <kmessagebox.h> 

#include "graph.h"
#include "gui.moc"


static struct {
  KStandardAction::StandardAction actionType;
  const char *name;
  const char *slot;
} standard_actions[] = {
  { KStandardAction::ZoomIn,  "zoomIn",  SLOT(zoomIn()) },
  { KStandardAction::ZoomOut, "zoomOut", SLOT(zoomOut()) },
  { KStandardAction::Open,    "open",    SLOT(load()) },
  { KStandardAction::SaveAs,  "save",    SLOT(save()) },
  { KStandardAction::Quit,    "quit",    SLOT(close()) }
};

static struct {
  const char *label;
  const char *name;
  const char *slot;
} normal_actions[] = {
  { I18N_NOOP("last hour"),        "lastHour",   SLOT(last_hour()) },
  { I18N_NOOP("last day"),         "lastDay",    SLOT(last_day()) },
  { I18N_NOOP("last week"),        "lastWeek",   SLOT(last_week()) },
  { I18N_NOOP("last month"),       "lastMonth",  SLOT(last_month()) },
  { I18N_NOOP("add new subgraph"), "splitGraph", SLOT(splitGraph()) },
};

/** 
 * Constructs a KCollectdGui
 * 
 * @param parent parent-widget see KMainWindow
 */
KCollectdGui::KCollectdGui(QWidget *parent)
  : KMainWindow(parent), action_collection(parent)
{
  // standard_actions
  for (size_t i=0; i< sizeof(standard_actions)/sizeof(*standard_actions); ++i)
    actionCollection()->addAction(standard_actions[i].actionType, 
	  standard_actions[i].name, this, standard_actions[i].slot);
  // normal actions
  for (size_t i=0; i< sizeof(normal_actions)/sizeof(*normal_actions); ++i) {
    KAction *act = new KAction(i18n(normal_actions[i].label), this);
    connect(act, SIGNAL(triggered()), this, normal_actions[i].slot);
    actionCollection()->addAction(normal_actions[i].name, act);
  }
  // toggle_actions
  auto_action = new KAction(KIcon("chronometer"), i18n("automatic update"), this);
  auto_action->setCheckable(true);
  actionCollection()->addAction("autoUpdate", auto_action);
  connect(auto_action, SIGNAL(toggled(bool)), this, SLOT(autoUpdate(bool)));

  // build widgets
  QWidget *main_widget = new QWidget;
  setCentralWidget(main_widget);

  QHBoxLayout *hbox = new QHBoxLayout(main_widget);
  listview_ = new QTreeWidget;
  listview_->setColumnCount(1);
  listview_->setHeaderLabels(QStringList(i18n("Sensordata")));
  listview_->setRootIsDecorated(true);
  listview_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  hbox->addWidget(listview_);

  vbox = new QVBoxLayout;
  hbox->addLayout(vbox);
  graph = new Graph;
  vbox->addWidget(graph);

  QHBoxLayout *hbox2 = new QHBoxLayout;
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
  zoom_out->setWhatsThis(i18n("zooms out"));
  zoom_out->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  hbox2->addWidget(zoom_out);
  auto_button = new KPushButton(KIcon("chronometer"), QString());
  auto_button->setToolTip(i18n("toggle automatic update-and-follow mode."));
  auto_button->setWhatsThis(i18n("<p>This button toggles the "
	      "automatic update-and-follow mode</p>"
	      "<p>The automatic update-and-follow mode updates the graph "
	      "every ten seconds. "
	      "In this mode the graph still can be zoomed, but always displays "
	      "<i>now</i> near the right edge and "
	      "can not be scrolled any more.<br />"
	      "This makes kcollectd some kind of status monitor.</p>"));
  auto_button->setCheckable(true);
  auto_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  hbox2->addWidget(auto_button);

  // signals
  connect(listview_, SIGNAL(itemPressed(QTreeWidgetItem *, int)), 
	SLOT(startDrag(QTreeWidgetItem *, int)));
  connect(last_month,  SIGNAL(clicked()), this, SLOT(last_month()));
  connect(last_week,   SIGNAL(clicked()), this, SLOT(last_week()));
  connect(last_day,    SIGNAL(clicked()), this, SLOT(last_day()));
  connect(last_hour,   SIGNAL(clicked()), this, SLOT(last_hour()));
  connect(zoom_in,     SIGNAL(clicked()), this, SLOT(zoomIn()));
  connect(zoom_out,    SIGNAL(clicked()), this, SLOT(zoomOut()));
  connect(auto_button, SIGNAL(toggled(bool)), this, SLOT(autoUpdate(bool)));

  // Menu
  KMenu *fileMenu = new KMenu(i18n("&File"));
  menuBar()->addMenu(fileMenu);
  fileMenu->addAction(actionCollection()->action("open"));
  fileMenu->addAction(actionCollection()->action("save"));
  fileMenu->addSeparator();
  fileMenu->addAction(actionCollection()->action("quit"));
  
  KMenu *editMenu = new KMenu(i18n("&Edit"));
  menuBar()->addMenu(editMenu);
  editMenu->addAction(actionCollection()->action("splitGraph"));

  KMenu *viewMenu = new KMenu(i18n("&View"));
  menuBar()->addMenu(viewMenu);
  viewMenu->addAction(actionCollection()->action("zoomIn"));
  viewMenu->addAction(actionCollection()->action("zoomOut"));
  viewMenu->addSeparator();
  viewMenu->addAction(actionCollection()->action("lastHour"));
  viewMenu->addAction(actionCollection()->action("lastDay"));
  viewMenu->addAction(actionCollection()->action("lastWeek"));
  viewMenu->addAction(actionCollection()->action("lastMonth"));
  viewMenu->addSeparator();
  viewMenu->addAction(actionCollection()->action("autoUpdate"));
}

KCollectdGui::~KCollectdGui()
{
}

void KCollectdGui::startDrag(QTreeWidgetItem *widget, int col)
{
  //       if (event->button() == Qt::LeftButton
  // && iconLabel->geometry().contains(event->pos())) {
  
  QDrag *drag = new QDrag(this);
  GraphMimeData *mimeData = new GraphMimeData;
  
  mimeData->setText(widget->text(1));
  mimeData->setGraph(widget->text(2), widget->text(3), widget->text(1));
  drag->setMimeData(mimeData);
  //drag->setPixmap(iconPixmap);
  
  drag->exec();
}

void KCollectdGui::set(Graph *new_graph)
{
  if (graph) {
    vbox->removeWidget(graph);
    delete graph;
  }
  vbox->insertWidget(0, new_graph);
}

void KCollectdGui::autoUpdate(bool t)
{
  auto_button->setChecked(t);
  auto_action->setChecked(t);
  graph->autoUpdate(t);
}

void KCollectdGui::load()
{
  QString filename = KFileDialog::getOpenFileName(KUrl(), 
	"application/x-kcollectd", this);
  if (filename.isEmpty()) return;

  load(filename);
}

void KCollectdGui::save()
{
  QString filename = KFileDialog::getSaveFileName(KUrl(), 
	"application/x-kcollectd", this);
  if (filename.isEmpty()) return;

  QFile out(filename);
  if (out.exists()) {
    int answer = KMessageBox::questionYesNo(this, 
	  i18n("file ‘%1’ allready exists.\n"
		"Do you want to overwrite it?", filename));
    if (answer != KMessageBox::Yes) {
      return;
    }
  }
  out.close();
  save(filename);
}

void KCollectdGui::load(const QString &filename)
{
  QFile in(filename);
  if (in.open(QIODevice::ReadOnly)) {
    QDomDocument doc;
    doc.setContent(&in);
    
    graph->clear();
    
    QDomElement t = doc.documentElement().firstChildElement("tab");
    while(!t.isNull()) {
      QDomElement g = t.firstChildElement("graph");
      while(!g.isNull()) {
	GraphInfo &graphinfo = graph->add();
	QDomElement p = g.firstChildElement("plot");
	while(!p.isNull()) {
	  graphinfo.add(p.attribute("rrd"), p.attribute("ds"), p.attribute("label"));
	  p = p.nextSiblingElement();
	}
	g = g.nextSiblingElement();
      }
      t = t.nextSiblingElement();
    }
  } else {
    KMessageBox::detailedSorry(this, 
	  i18n("reading file ‘%1’ failed.", filename), 
	  i18n("System message is: ‘%1’", in.errorString()));
  }
}

void KCollectdGui::save(const QString &filename)
{
  QFile out(filename);
  if (out.open(QIODevice::WriteOnly)) {
    QXmlStreamWriter stream(&out);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeDTD("<!DOCTYPE kcollectd SYSTEM "
	  "\"/usr/share/kcollectd/kcollectd.xsd\">");
    
    stream.writeStartElement("kcollectd");
    stream.writeStartElement("tab");
    for(Graph::const_iterator i = graph->begin(); i != graph->end(); ++i) {
      stream.writeStartElement("graph");
      for(GraphInfo::const_iterator j = i->begin(); j != i->end(); ++j) {
	stream.writeStartElement("plot");
	stream.writeAttribute("rrd", j->rrd);
	stream.writeAttribute("ds", j->ds);
	stream.writeAttribute("label", j->label);
	stream.writeEndElement();
      }
      stream.writeEndElement();
    }
    stream.writeEndElement();
    stream.writeEndElement();
    
    stream.writeEndDocument();
    out.close();
  } else {
    KMessageBox::detailedSorry(this, 
	  i18n("opening the file ‘%1’ for writing failed.", filename), 
	  i18n("System message is: ‘%1’", out.errorString()));
  }
}
