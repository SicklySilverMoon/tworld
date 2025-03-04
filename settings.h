/* settings.h: Functions for managing settings.
 *
 * Copyright (C) 2014-2017 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef HEADER_settings_h_
#define HEADER_settings_h_

#ifdef __cplusplus
extern "C" {
#endif

extern char *settingsdir;

void loadsettings(void);

void savesettings(void);

/* Obtain integer setting. Returns -1 if setting doesn't exist or cannot be
 * parsed as an int. */
int getintsetting(char const * name);
void setintsetting(char const * name, int val);

/* Obtain a string setting. Returned pointer is good until the setting is
   modified. Returns NULL if the setting doesn't exist. */
char const * getstringsetting(char const * name);
void setstringsetting(char const * name, char const * val);

#ifdef __cplusplus
}
#endif

#endif
