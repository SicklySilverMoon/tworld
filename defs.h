/* defs.h: Definitions used throughout the program.
 *
 * Copyright (C) 2001-2017 by Brian Raiter, Madhav Shanbhag, and Eric Schmidt
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_defs_h_
#define	HEADER_defs_h_

#include	<stdio.h>
#include	<time.h>
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
    Ruleset_Count,
    Ruleset_First = Ruleset_Lynx
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

/* A move is specified by its direction and when it takes place.
 */
typedef	struct action { unsigned int when:23, dir:9; } action;

/* A structure for managing the memory holding the moves of a game.
 */
typedef struct actlist {
    int			allocated;	/* number of elements allocated */
    int			count;		/* size of the actual array */
    action	       *list;		/* the array */
} actlist;

/* The range of relative mouse moves is a 19x19 square around Chip.
 * (Mouse moves are stored as a relative offset in order to fit all
 * possible moves in nine bits.)
 */
#define	MOUSERANGEMIN	(-9)
#define	MOUSERANGEMAX	(+9)
#define	MOUSERANGE	19

/* The complete list of commands that the user can given.
 */
enum {
    CmdNone = NIL,
    CmdNorth = NORTH,
    CmdWest = WEST,
    CmdSouth = SOUTH,
    CmdEast = EAST,
    CmdKeyMoveFirst = NORTH,
    CmdKeyMoveLast = NORTH | WEST | SOUTH | EAST,
    CmdMouseMoveFirst,
    CmdMoveNop = CmdMouseMoveFirst - MOUSERANGEMIN * (MOUSERANGE + 1),
    CmdMouseMoveLast = CmdMouseMoveFirst + MOUSERANGE * MOUSERANGE - 1,
    CmdReservedFirst,
    CmdReservedLast = 511,
    CmdAbsMouseMoveFirst,
    CmdAbsMouseMoveLast = CmdAbsMouseMoveFirst + CXGRID * CYGRID - 1,
    CmdMoveFirst = CmdKeyMoveFirst,
    CmdMoveLast = CmdAbsMouseMoveLast,
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
    CmdKeys,
    CmdPlayback,
    CmdCheckSolution,
    CmdReplSolution,
    CmdKillSolution,
    CmdSeeScores,
    CmdSeeSolutionFiles,
    CmdTimesClipboard,
    CmdVolumeUp,
    CmdVolumeDown,
    CmdStepping,
    CmdSubStepping,
    CmdRandomFF,
    CmdShowInitState,
    CmdAdvanceGame,
    CmdAdvanceMoveGame,
    CmdProceed,
#ifndef NDEBUG
    CmdDebugCmd1,
    CmdDebugCmd2,
#endif
    CmdQuit,
    CmdPreserve,
    CmdSeek,
#ifndef NDEBUG
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
#endif
    CmdCount
};

/* True if cmd is a simple directional command, i.e. a single
 * orthogonal or diagonal move (or CmdNone).
 */
#define	directionalcmd(cmd)	(((cmd) & ~CmdKeyMoveLast) == 0)

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
    int			besttime;	/* time (in ticks) of best solution */
    int			sgflags;	/* saved-game flags (see below) */
    int			levelsize;	/* size of the level data */
    int			solutionsize;	/* size of the saved solution data */
    unsigned char      *leveldata;	/* the data defining the level */
    unsigned char      *solutiondata;	/* the player's best solution so far */
    unsigned long	levelhash;	/* the level data's hash value */
    char const	       *unsolvable;	/* why level is unsolvable, or NULL */
    char		name[256];	/* name of the level */
    char		passwd[256];	/* the level's password */
    char		author[256];	/* the level's author */
} gamesetup;

/* Flags associated with a saved game.
 */
#define	SGF_HASPASSWD		0x0001	/* player knows the level's password */
#define	SGF_REPLACEABLE		0x0002	/* solution is marked as replaceable */
#define	SGF_SETNAME		0x0004	/* internal to solution.c */

/* The history for the last time a levelset was played.
 */
typedef	struct history {
    char		name[256];	/* the set filename minus any path */
    char		passwd[256];	/* password of the last played level */
    int	   		levelnumber;	/* number of the last played level */
    struct tm		dt;		/* date/time set was last played */
} history;

/* The collection of data maintained for each series.
 */
typedef	struct gameseries {
    int			count;		/* number of levels in the series */
    int			allocated;	/* number of elements allocated */
    int			final;		/* number of the ending level */
    int			ruleset;	/* the ruleset for the game file */
    int			gsflags;	/* series flags (see below) */
    gamesetup	       *games;		/* the array of levels */
    fileinfo		mapfile;	/* the file containing the levels */
    char	       *mapfilename;	/* the name of said file */
    fileinfo		savefile;	/* the file holding the solutions */
    char	       *savefilename;	/* non-default name for said file */
    int			solheaderflags;	/* solution flags (none defined yet) */
    int			solheadersize;	/* size of extra solution header */
    char		filebase[256];	/* the level set's filename */
    char		name[256];	/* the filename minus any path */
    unsigned char	solheader[256];	/* extra solution header bytes */
} gameseries;

/* Just a list of ints */
typedef struct intlist {
    int 	*list;		/* intlist */
    int		 count;		/* size of list */
} intlist;

/* The possible locations for a levelset.
 */
#define LOC_SERIESDIR		0x01
#define LOC_SERIESDATDIR	0x02

/* Information associated with a levelset. Contains the information about
 * all gameseries that use the levelset.
 */
typedef struct mapfileinfo {
    char *filename;
    intlist sfilelst[Ruleset_Count]; /* indices for series files per ruleset */
    int levelcount;
    int locdirs;	   /* Flags indicating which dirs the set is in */
} mapfileinfo;

/* Flags associated with a series.
 */
#define	GSF_ALLMAPSREAD		0x0001	/* finished reading the data file */
#define	GSF_NOSAVING		0x0002	/* treat solution file as read-only */
#define	GSF_NODEFAULTSAVE	0x0004	/* don't use default tws filename */
#define	GSF_IGNOREPASSWDS	0x0008	/* don't require passwords */
#define	GSF_LYNXFIXES		0x0010	/* changes MS data into Lynx levels */
#define GSF_DATFORDACSERIESDIR	0x0020	/* datfile for .dac is in seriesdir */

#endif
