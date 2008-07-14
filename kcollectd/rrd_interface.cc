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

#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <cstring>
#include <cstdlib>

#include <rrd.h>

#include "rrd_interface.h"

/*
 * read the datasources-names of a rrd from “rrdtools info”, because
 * there is no official API to get them
 */
void get_dsinfo(const std::string &rrdfile, std::set<std::string> &list)
{
  using namespace std;

  // just to be sure
  list.clear();

  // call rrdtool info <filename>
  string command("rrdtool info ");
  command += rrdfile;
  FILE *in = popen(command.c_str(), "r");
  if (!in) {
    throw bad_rrdinfo();
  } 

  // read in the output and find ds[...] lines
  int c;
  string line;
  line.reserve(128);
  while((c = getc(in)) != EOF) {
    if (c == '\n') {
      if (!line.compare(0, 3, "ds[")) {
	string::size_type close = line.find(']');
	if (close != string::npos)
	  list.insert(line.substr(3, close-3));
      }
      line.clear();
    } else {
      line += static_cast<char>(c);
   }
  }
  if (pclose(in)) {
    throw bad_rrdinfo();
  }
}

void get_rrd_data (const std::string &file, const std::string &ds, 
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
