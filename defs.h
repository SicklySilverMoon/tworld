/* defs.h: Definitions used throughout the program.
 *
 * Copyright (C) 2001-2004 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	_defs_h_
#define	_defs_h_

#include	<stdio.h>
#include	"gen.h"

/*
 * Miscellaneous definitions.
 */

/* The various rulesets the program can emulate.
 */
enum {
    Ruleset_None = 0,
    Ruleset_Lynx = 1,
    Ruleset_MS = 2,
    Ruleset_Count
};

/* File I/O structure.
 */
typedef	struct fileinfo {
    char       *name;		/* the name of the file */
    FILE       *fp;		/* the real file handle */
    char	alloc;		/* TRUE if name was allocated internally */
} fileinfo;

/* Pseudorandom number generators.
 */
typedef	struct prng {
    unsigned long	initial;	/* initial seed value */
    unsigned long	value;		/* latest random value */
    char		shared;		/* FALSE if independent sequence */
} prng;

/*
 * Definitions used in game play.
 */

/* Turning macros.
 */
#define	left(dir)	((((dir) << 1) | ((dir) >> 3)) & 15)
#define	back(dir)	((((dir) << 2) | ((dir) >> 2)) & 15)
#define	right(dir)	((((dir) << 3) | ((dir) >> 1)) & 15)

/* A move is specified by its direction and when it takes place.
 */
typedef	struct action { int when:27, dir:5; } action;

/* A structure for managing the memory holding the moves of a game.
 */
typedef struct actlist {
    int			allocated;	/* number of elements allocated */
    int			count;		/* size of the actual array */
    action	       *list;		/* the array */
} actlist;

/* Two x,y-coordinates give the locations of a button and what it is
 * connected to.
 */
typedef	struct xyconn {
    short		from;		/* location of the button */
    short		to;		/* location of the trap/cloner */
} xyconn;

/* The complete list of commands that the user can given.
 */
enum {
    CmdNone = NIL,
    CmdNorth = NORTH,
    CmdWest = WEST,
    CmdSouth = SOUTH,
    CmdEast = EAST,
    CmdMoveFirst = NORTH,
    CmdMoveLast = NORTH | WEST | SOUTH | EAST,
    CmdPrevLevel,
    CmdNextLevel,
    CmdSameLevel,
    CmdQuitLevel,
    CmdGotoLevel,
    CmdPrev,
    CmdNext,
    CmdSame,
    CmdPrev10,
    CmdNext10,
    CmdPauseGame,
    CmdHelp,
    CmdPlayback,
    CmdSeeScores,
    CmdKillSolution,
    CmdVolumeUp,
    CmdVolumeDown,
    CmdSteppingUp,
    CmdSteppingDown,
    CmdProceed,
    CmdDebugCmd1,
    CmdDebugCmd2,
    CmdQuit,
    CmdPreserve,
    CmdCheatNorth,
    CmdCheatWest,
    CmdCheatSouth,
    CmdCheatEast,
    CmdCheatHome,
    CmdCheatKeyRed,
    CmdCheatKeyBlue,
    CmdCheatKeyYellow,
    CmdCheatKeyGreen,
    CmdCheatBootsIce,
    CmdCheatBootsSlide,
    CmdCheatBootsFire,
    CmdCheatBootsWater,
    CmdCheatICChip,
    CmdCount
};

/* The list of available sound effects.
 */
#define	SND_CHIP_LOSES		0
#define	SND_CHIP_WINS		1
#define	SND_TIME_OUT		2
#define	SND_TIME_LOW		3
#define	SND_DEREZZ		4
#define	SND_CANT_MOVE		5
#define	SND_IC_COLLECTED	6
#define	SND_ITEM_COLLECTED	7
#define	SND_BOOTS_STOLEN	8
#define	SND_TELEPORTING		9
#define	SND_DOOR_OPENED		10
#define	SND_SOCKET_OPENED	11
#define	SND_BUTTON_PUSHED	12
#define	SND_TILE_EMPTIED	13
#define	SND_WALL_CREATED	14
#define	SND_TRAP_ENTERED	15
#define	SND_BOMB_EXPLODES	16
#define	SND_WATER_SPLASH	17

#define	SND_ONESHOT_COUNT	18

#define	SND_BLOCK_MOVING	18
#define	SND_SKATING_FORWARD	19
#define	SND_SKATING_TURN	20
#define	SND_SLIDING		21
#define	SND_SLIDEWALKING	22
#define	SND_ICEWALKING		23
#define	SND_WATERWALKING	24
#define	SND_FIREWALKING		25

#define	SND_COUNT		26

/*
 * Structures for defining the games proper.
 */

/* The collection of data maintained for each level.
 */
typedef	struct gamesetup {
    int			number;		/* numerical ID of the level */
    int			time;		/* no. of seconds allotted */
    int			chips;		/* no. of chips for the socket */
    int			besttime;	/* time (in ticks) of best solution */
    unsigned long	savedrndseed;	/* PRNG seed of best solution */
    unsigned char	savedrndslidedir; /* rnd-slide dir of best solution */
    signed char		savedstepping;	/* timer offset of best solution */
    unsigned char	sgflags;	/* saved-game flags (see below) */
    int			map1size;	/* compressed size of layer 1 */
    int			map2size;	/* compressed size of layer 2 */
    unsigned char      *map1;		/* layer 1 (top) of the map */
    unsigned char      *map2;		/* layer 2 (bottom) of the map */
    int			creaturecount;	/* size of active creature list */
    int			trapcount;	/* size of beartrap connection list */
    int			clonercount;	/* size of cloner connection list */
    actlist		savedsolution;	/* the player's best solution so far */
    short		creatures[256];	/* the active creature list */
    xyconn		traps[256];	/* the beatrap connection list */
    xyconn		cloners[256];	/* the clone machine connection list */
    char		name[256];	/* name of the level */
    char		passwd[256];	/* the level's password */
    char		hinttext[256];	/* the level's hint */
} gamesetup;

/* Flags associated with a saved game.
 */
#define	SGF_HASPASSWD	0x01		/* player knows the level's password */
#define	SGF_REPLACEABLE	0x02		/* solution is marked as replaceable */

/* The collection of data maintained for each series.
 */
typedef	struct gameseries {
    int			total;		/* number of levels in the series */
    int			allocated;	/* number of elements allocated */
    int			count;		/* actual size of array */
    int			final;		/* number of the ending level */
    int			ruleset;	/* the ruleset for the game file */
    int			gsflags;	/* flags (see below) */
    gamesetup	       *games;		/* the array of levels */
    fileinfo		mapfile;	/* the file containing the levels */
    char	       *mapfilename;	/* the name of said file */
    fileinfo		solutionfile;	/* the file of the user's solutions */
    int			solutionflags;	/* settings for the saved solutions */
    char		filebase[256];	/* the root of the main filename */
    char		name[256];	/* the name of the series */
} gameseries;

/* Flags associated with a series.
 */
#define	GSF_ALLMAPSREAD		0x0001	/* finished reading the data file */
#define	GSF_IGNOREPASSWDS	0x0002	/* don't require passwords */
#define	GSF_LYNXFIXES		0x0004	/* change MS data into Lynx levels */

#endif
