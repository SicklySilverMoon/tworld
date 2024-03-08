#include <cstdint>

#include <QSoundEffect>
#include <QUrl>

#include "defs.h"
#include "oshw.h"

/* The data needed for each sound effect's wave.
 */
typedef	struct sfxinfo {
    QSoundEffect sound{}; /* the sound effect data */
    char const* textsfx{nullptr}; /* the onomatopoeia string */
} sfxinfo;

static sfxinfo sounds[SND_COUNT];
static bool enabled = TRUE;
static float volume = 1;

/* Initialize the textual sound effects.
 */
static void initonomatopoeia() {
    sounds[SND_CHIP_LOSES].textsfx      = "\"Bummer\"";
    sounds[SND_CHIP_WINS].textsfx       = "Tadaa!";
    sounds[SND_TIME_OUT].textsfx        = "Clang!";
    sounds[SND_TIME_LOW].textsfx        = "Ktick!";
    sounds[SND_DEREZZ].textsfx		    = "Bzont!";
    sounds[SND_CANT_MOVE].textsfx       = "Mnphf!";
    sounds[SND_IC_COLLECTED].textsfx    = "Chack!";
    sounds[SND_ITEM_COLLECTED].textsfx  = "Slurp!";
    sounds[SND_BOOTS_STOLEN].textsfx    = "Flonk!";
    sounds[SND_TELEPORTING].textsfx     = "Bamff!";
    sounds[SND_DOOR_OPENED].textsfx     = "Spang!";
    sounds[SND_SOCKET_OPENED].textsfx   = "Clack!";
    sounds[SND_BUTTON_PUSHED].textsfx   = "Click!";
    sounds[SND_BOMB_EXPLODES].textsfx   = "Booom!";
    sounds[SND_WATER_SPLASH].textsfx    = "Plash!";
    sounds[SND_TILE_EMPTIED].textsfx    = "Whisk!";
    sounds[SND_WALL_CREATED].textsfx    = "Chunk!";
    sounds[SND_TRAP_ENTERED].textsfx    = "Shunk!";
    sounds[SND_SKATING_TURN].textsfx    = "Whing!";
    sounds[SND_SKATING_FORWARD].textsfx = "Whizz ...";
    sounds[SND_SLIDING].textsfx         = "Drrrr ...";
    sounds[SND_BLOCK_MOVING].textsfx    = "Scrrr ...";
    sounds[SND_SLIDEWALKING].textsfx    = "slurp slurp ...";
    sounds[SND_ICEWALKING].textsfx      = "snick snick ...";
    sounds[SND_WATERWALKING].textsfx    = "plip plip ...";
    sounds[SND_FIREWALKING].textsfx     = "crackle crackle ...";
}

/* Display the onomatopoeia for the currently playing sound effect.
 * Only the first sound is used, since we can't display multiple
 * strings.
 */
static void displaysoundeffects(unsigned long sfx) {
    unsigned long flag;
    int i;

    for (flag = 1, i = 0 ; flag ; flag <<= 1, ++i) {
        if (sfx & flag) {
            setdisplaymsg(sounds[i].textsfx, 500, 10);
            return;
        }
    }
}

int initaudiosystem(int silence, int _soundbufsize) {
    //todo: use the silence part
    initonomatopoeia();
}

/* Activate or deactivate the sound system. When activating for the
 * first time, the connection to the sound device is established. When
 * deactivating, the connection is closed.
 */
int setaudiosystem(int active) {
    return true; //useless in the new system but needed for header and call compat
}

/* Load a single wave file into memory.
 */
int loadsfxfromfile(int index, char const *filename) {
    if (!filename)
        return false;

    QSoundEffect& sound = sounds[index].sound;
    sound.setSource(QUrl::fromLocalFile(filename));
    if (index < SND_ONESHOT_COUNT) {
        sound.setLoopCount(1);
    } else {
        sound.setLoopCount(QSoundEffect::Infinite);
    }
    sound.setVolume(volume);
    return true;
}

/* Select the sounds effects to be played. sfx is a bitmask of sound
 * effect indexes. Any continuous sounds that are not included in sfx
 * are stopped. One-shot sounds that are included in sfx are
 * restarted.
 */
void playsoundeffects(unsigned long sfx) {
    if (!enabled) {
        return;
    }

    if (volume == 0) {
        displaysoundeffects(sfx);
    }

    int i;
    unsigned long flag;
    for (i = 0, flag = 1; i < SND_COUNT; i++, flag <<= 1) {
        if (sfx & flag) {
            sounds[i].sound.play();
        } else {
            if (i >= SND_ONESHOT_COUNT) {
                sounds[i].sound.stop();
            }
        }
    }
}

/* If action is negative, stop playing all sounds immediately.
 * Otherwise, just temporarily pause or unpause sound-playing.
 */
void setsoundeffects(int action) {
    if (action <= 0) {
        if (action == 0) {
            enabled = false;
        }
        for (int i = 0; i < SND_ONESHOT_COUNT; i++) {
            sounds[i].sound.stop();
        }
    } else {
        enabled = true; //todo: add proper pausing and resuming
    }
}

/* Release all memory for the given sound effect.
 */
void freesfx(int index) {
    sounds[index].sound.stop(); //no need to actually free mem, again compat call
}

/* Set the current volume level to v. If display is true, the
 * new volume level is displayed to the user.
 */
int setvolume(int v, int display) {
    if (v < 0) {
        v = 0;
    } else if (v > 10) {
        v = 10;
    }

    volume = v / 10.0f;
    for (int i = 0; i < SND_ONESHOT_COUNT; i++) {
        sounds[i].sound.setVolume(volume);
    }

    if (display) {
        char buf[16];
        sprintf(buf, "Volume: %d", v);
        setdisplaymsg(buf, 1000, 1000);
    }
    return true;
}

/* Change the current volume level by delta. If display is true, the
 * new volume level is displayed to the user.
 */
int changevolume(int delta, int display) {
    return setvolume(static_cast<int>(volume * 10) + delta, display);
}