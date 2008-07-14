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
#include <sstream>
#include <iomanip>

#include "labeling.h"

bool si_char(double d, std::string &s, double &m)
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

std::string si_number(double d, int p, const std::string &s, double m)
{
  std::ostringstream os;
  os << std::setprecision(p) << d/m;
  if (!s.empty())
    os << " " << s;
  return os.str();
}
