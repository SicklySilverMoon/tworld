/* help.h: Displaying online help screens.
 *
 * Copyright (C) 2001,2002 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	_help_h_
#define	_help_h_

/* The available help topics.
 */
enum {
    Help_None = 0,
    Help_KeysDuringGame,
    Help_KeysBetweenGames,
    Help_ObjectsOfGame,
    Help_AboutGame
};

/* Help for the command-line options.
 */
extern tablespec const *yowzitch;

/* Version and license information.
 */
extern tablespec const *vourzhon;

/* Display online help screens for the game, using the given topic as
 * the default topic.
 */
extern void onlinehelp(int topic);

#endif
