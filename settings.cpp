/* settings.cpp: Functions for managing settings.
 *
 * Copyright (C) 2014-2017 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include "settings.h"

#include "err.h"
#include "fileio.h"

#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>

char *settingsdir = NULL;

using std::free;
using std::getline;
using std::ifstream;
using std::istringstream;
using std::map;
using std::move;
using std::ofstream;
using std::ostringstream;
using std::string;

namespace
{
    map<string, string> settings;
}

char const * sfname = "settings";

void loadsettings()
{
    char *fname = getpathforfileindir(settingsdir, sfname);
    ifstream in(fname);
    free(fname);

    if (!in)
        return;

    map<string, string> newsettings;
    string line;
    while (getline(in, line))
    {
        size_t pos(line.find('='));
        if (pos != string::npos)
            newsettings.insert({line.substr(0, pos), line.substr(pos+1)});
    }

    if (!in.eof())
        warn("Error reading settings file");

    settings = move(newsettings);
}

void savesettings()
{
    if (!finddir(settingsdir, TRUE)) {
        warn("could not access settings directory");
        return;
    }
    char *fname = getpathforfileindir(settingsdir, sfname);
    ofstream out(fname);
    free(fname);

    if (!out)
    {
        warn("Could not open settings file");
        return;
    }
    for (map<string,string>::const_iterator i(settings.begin());
         i != settings.end(); ++i)
    {
        out << i->first << '=' << i->second << '\n';
    }

    if (!out)
    {
        warn("Could not write settings");
    }
}

int getintsetting(char const * name)
{
    std::map<string, string>::const_iterator loc(settings.find(name));
    if (loc == settings.end())
        return -1;
    std::istringstream in(loc->second);
    int i;
    if (!(in >> i))
        return -1;
    return i;  
}

void setintsetting(char const * name, int val)
{
    std::ostringstream out;
    out << val;
    settings[name] = out.str();
}

char const * getstringsetting(char const * name)
{
    std::map<string, string>::const_iterator loc(settings.find(name));
    if (loc == settings.end())
	return nullptr;
    return loc->second.c_str();
}

void setstringsetting(char const * name, char const * val)
{
    settings[name] = val;
}
