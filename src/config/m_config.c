//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//    Configuration file interface.
//


#include "m_config.h"
#include "m_variable.h"

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <locale.h>
#include <SDL_filesystem.h>
#include "doomkeys.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"


//
// DEFAULTS
//

// Location where all configuration data is stored - default.cfg, savegames, etc.

const char* configdir;

static char* autoload_path = "";

// Default filenames for configuration files.

static const char* default_main_config;
static const char* default_extra_config;

extern default_collection_t doom_defaults;
extern default_collection_t extra_defaults;

//
// Search a collection for a variable
//
static default_t* SearchCollection(default_collection_t* collection,
                                   const char* name) {
    for (int i = 0; i < collection->numdefaults; ++i) {
        if (!strcmp(name, collection->defaults[i].name)) {
            return &collection->defaults[i];
        }
    }
    return NULL;
}

// Mapping from DOS keyboard scan code to internal key code (as defined
// in doomkey.h). I think I (fraggle) reused this from somewhere else
// but I can't find where. Anyway, notes:
//  * KEY_PAUSE is wrong - it's in the KEY_NUMLOCK spot. This shouldn't
//    matter in terms of Vanilla compatibility because neither of
//    those were valid for key bindings.
//  * There is no proper scan code for PrintScreen (on DOS machines it
//    sends an interrupt). So I added a fake scan code of 126 for it.
//    The presence of this is important so we can bind PrintScreen as
//    a screenshot key.
static const int scantokey[128] = {
    0,             KEY_ESCAPE,     '1',           '2',
    '3',           '4',            '5',           '6',
    '7',           '8',            '9',           '0',
    '-',           '=',            KEY_BACKSPACE, KEY_TAB,
    'q',           'w',            'e',           'r',
    't',           'y',            'u',           'i',
    'o',           'p',            '[',           ']',
    KEY_ENTER,     KEY_RCTRL,      'a',           's',
    'd',           'f',            'g',           'h',
    'j',           'k',            'l',           ';',
    '\'',          '`',            KEY_RSHIFT,    '\\',
    'z',           'x',            'c',           'v',
    'b',           'n',            'm',           ',',
    '.',           '/',            KEY_RSHIFT,    KEYP_MULTIPLY,
    KEY_RALT,      ' ',            KEY_CAPSLOCK,  KEY_F1,
    KEY_F2,        KEY_F3,         KEY_F4,        KEY_F5,
    KEY_F6,        KEY_F7,         KEY_F8,        KEY_F9,
    KEY_F10,       KEY_PAUSE,      KEY_SCRLCK,    KEY_HOME,
    KEY_UPARROW,   KEY_PGUP,       KEY_MINUS,     KEY_LEFTARROW,
    KEYP_5,        KEY_RIGHTARROW, KEYP_PLUS,     KEY_END,
    KEY_DOWNARROW, KEY_PGDN,       KEY_INS,       KEY_DEL,
    0,             0,              0,             KEY_F11,
    KEY_F12,       0,              0,             0,
    0,             0,              0,             0,
    0,             0,              0,             0,
    0,             0,              0,             0,
    0,             0,              0,             0,
    0,             0,              0,             0,
    0,             0,              0,             0,
    0,             0,              0,             0,
    0,             0,              0,             0,
    0,             0,              KEY_PRTSCR,    0,
};


static void SaveDefaultCollection(default_collection_t* collection) {
    default_t* defaults;
    int i, v;
    FILE* f;

    f = M_fopen(collection->filename, "w");
    if (!f)
        return; // can't write the file, but don't complain

    defaults = collection->defaults;

    for (i = 0; i < collection->numdefaults; i++) {
        int chars_written;

        // Ignore unbound variables

        if (!defaults[i].bound) {
            continue;
        }

        // Print the name and line up all values at 30 characters

        chars_written = fprintf(f, "%s ", defaults[i].name);

        for (; chars_written < 30; ++chars_written)
            fprintf(f, " ");

        // Print the value

        switch (defaults[i].type) {
            case DEFAULT_KEY:

                // use the untranslated version if we can, to reduce
                // the possibility of screwing up the user's config
                // file

                v = *defaults[i].location.i;

                if (v == KEY_RSHIFT) {
                    // Special case: for shift, force scan code for
                    // right shift, as this is what Vanilla uses.
                    // This overrides the change check below, to fix
                    // configuration files made by old versions that
                    // mistakenly used the scan code for left shift.

                    v = 54;
                } else if (defaults[i].untranslated
                           && v == defaults[i].original_translated) {
                    // Has not been changed since the last time we
                    // read the config file.

                    v = defaults[i].untranslated;
                } else {
                    // search for a reverse mapping back to a scancode
                    // in the scantokey table

                    int s;

                    for (s = 0; s < 128; ++s) {
                        if (scantokey[s] == v) {
                            v = s;
                            break;
                        }
                    }
                }

                fprintf(f, "%i", v);
                break;

            case DEFAULT_INT:
                fprintf(f, "%i", *defaults[i].location.i);
                break;

            case DEFAULT_INT_HEX:
                fprintf(f, "0x%x", *defaults[i].location.i);
                break;

            case DEFAULT_FLOAT:
                fprintf(f, "%f", *defaults[i].location.f);
                break;

            case DEFAULT_STRING:
                fprintf(f, "\"%s\"", *defaults[i].location.s);
                break;
        }

        fprintf(f, "\n");
    }

    fclose(f);
}

//
// Parses integer values in the configuration file
//
static int ParseIntParameter(const char* strparm) {
    int parm;
    if (strparm[0] == '0' && strparm[1] == 'x') {
        sscanf(strparm + 2, "%x", (unsigned int*) &parm);
    } else {
        sscanf(strparm, "%i", &parm);
    }
    return parm;
}

static void SetVariable(default_t* def, const char* value) {
    int intparm;

    // parameter found
    switch (def->type) {
        case DEFAULT_STRING:
            *def->location.s = M_StringDuplicate(value);
            break;

        case DEFAULT_INT:
        case DEFAULT_INT_HEX:
            *def->location.i = ParseIntParameter(value);
            break;

        case DEFAULT_KEY:

            // translate scancodes read from config
            // file (save the old value in untranslated)

            intparm = ParseIntParameter(value);
            def->untranslated = intparm;
            if (intparm >= 0 && intparm < 128) {
                intparm = scantokey[intparm];
            } else {
                intparm = 0;
            }

            def->original_translated = intparm;
            *def->location.i = intparm;
            break;

        case DEFAULT_FLOAT: {
            // Different locales use different decimal separators.
            // However, the choice of the current locale isn't always
            // under our own control. If the atof() function fails to
            // parse the string representing the floating point number
            // using the current locale's decimal separator, it will
            // return 0, resulting in silent sound effects. To
            // mitigate this, we replace the first non-digit,
            // non-minus character in the string with the current
            // locale's decimal separator before passing it to atof().
            struct lconv* lc = localeconv();
            char dec, *str;
            int i = 0;

            dec = lc->decimal_point[0];
            str = M_StringDuplicate(value);

            // Skip sign indicators.
            if (str[i] == '-' || str[i] == '+') {
                i++;
            }

            for (; str[i] != '\0'; i++) {
                if (!isdigit(str[i])) {
                    str[i] = dec;
                    break;
                }
            }

            *def->location.f = (float) atof(str);
            free(str);
        } break;
    }
}

static void LoadDefaultCollection(default_collection_t* collection) {
    FILE* f;
    default_t* def;
    char defname[80];
    char strparm[100];

    // read the file in, overriding any set defaults
    f = M_fopen(collection->filename, "r");

    if (f == NULL) {
        // File not opened, but don't complain.
        // It's probably just the first time they ran the game.

        return;
    }

    while (!feof(f)) {
        if (fscanf(f, "%79s %99[^\n]\n", defname, strparm) != 2) {
            // This line doesn't match

            continue;
        }

        // Find the setting in the list

        def = SearchCollection(collection, defname);

        if (def == NULL || !def->bound) {
            // Unknown variable?  Unbound variables are also treated
            // as unknown.

            continue;
        }

        // Strip off trailing non-printable characters (\r characters
        // from DOS text files)

        while (strlen(strparm) > 0 && !isprint(strparm[strlen(strparm) - 1])) {
            strparm[strlen(strparm) - 1] = '\0';
        }

        // Surrounded by quotes? If so, remove them.
        if (strlen(strparm) >= 2 && strparm[0] == '"'
            && strparm[strlen(strparm) - 1] == '"') {
            strparm[strlen(strparm) - 1] = '\0';
            memmove(strparm, strparm + 1, sizeof(strparm) - 1);
        }

        SetVariable(def, strparm);
    }

    fclose(f);
}

//
// Set the default filenames to use for configuration files.
//
void M_SetConfigFilenames(const char* main_config, const char* extra_config) {
    default_main_config = main_config;
    default_extra_config = extra_config;
}

void M_SaveDefaults() {
    SaveDefaultCollection(&doom_defaults);
    SaveDefaultCollection(&extra_defaults);
}

//
// M_LoadDefaults
//

void M_LoadDefaults() {
    int i;

    // This variable is a special snowflake for no good reason.
    M_BindStringVariable("autoload_path", &autoload_path);

    // check for a custom default file

    //!
    // @arg <file>
    // @vanilla
    //
    // Load main configuration from the specified file, instead of the
    // default.
    //

    i = M_CheckParmWithArgs("-config", 1);

    if (i) {
        doom_defaults.filename = myargv[i + 1];
        printf("	default file: %s\n", doom_defaults.filename);
    } else {
        doom_defaults.filename =
            M_StringJoin(configdir, default_main_config, NULL);
    }

    printf("saving config in %s\n", doom_defaults.filename);

    //!
    // @arg <file>
    //
    // Load additional configuration from the specified file, instead of
    // the default.
    //

    i = M_CheckParmWithArgs("-extraconfig", 1);

    if (i) {
        extra_defaults.filename = myargv[i + 1];
        printf("        extra configuration file: %s\n",
               extra_defaults.filename);
    } else {
        extra_defaults.filename =
            M_StringJoin(configdir, default_extra_config, NULL);
    }

    LoadDefaultCollection(&doom_defaults);
    LoadDefaultCollection(&extra_defaults);
}

//
// Get a configuration file variable by its name
//
static default_t* GetDefaultForName(const char* name) {
    // Try the main list and the extras
    default_t* result = SearchCollection(&doom_defaults, name);
    if (result == NULL) {
        result = SearchCollection(&extra_defaults, name);
    }
    // Not found? Internal error.
    if (result == NULL) {
        I_Error("Unknown configuration variable: '%s'", name);
    }
    return result;
}

//
// Bind a variable to a given configuration file variable, by name.
//
void M_BindIntVariable(const char* name, int* location) {
    default_t* variable = GetDefaultForName(name);

    assert(variable->type == DEFAULT_INT || variable->type == DEFAULT_INT_HEX
           || variable->type == DEFAULT_KEY);

    variable->location.i = location;
    variable->bound = true;
}

void M_BindFloatVariable(const char* name, float* location) {
    default_t* variable;

    variable = GetDefaultForName(name);
    assert(variable->type == DEFAULT_FLOAT);

    variable->location.f = location;
    variable->bound = true;
}

void M_BindStringVariable(const char* name, char** location) {
    default_t* variable;

    variable = GetDefaultForName(name);
    assert(variable->type == DEFAULT_STRING);

    variable->location.s = location;
    variable->bound = true;
}

//
// Set the value of a particular variable; an API function for other
// parts of the program to assign values to config variables by name.
//
bool M_SetVariable(const char* name, const char* value) {
    default_t* variable = GetDefaultForName(name);
    if (!variable || !variable->bound) {
        return false;
    }
    SetVariable(variable, value);
    return true;
}

const char* M_GetStringVariable(const char* name) {
    const default_t* variable = GetDefaultForName(name);
    if (!variable || !variable->bound || variable->type != DEFAULT_STRING) {
        return NULL;
    }
    return *variable->location.s;
}

#if !defined(_WIN32) || defined(_WIN32_WCE)
//
// Get the path to the default configuration dir to use, if NULL
// is passed to M_SetConfigDir.
//
static char* M_GetDefaultConfigDir() {
    // Configuration settings are stored in an OS-appropriate path
    // determined by SDL. On typical Unix systems, this might be
    // ~/.local/share/broom. On Windows, we behave like Vanilla Doom
    // and save in the current directory.

    char* result;
    char* copy;

    result = SDL_GetPrefPath("", PACKAGE_TARNAME);
    if (result != NULL) {
        copy = M_StringDuplicate(result);
        SDL_free(result);
        return copy;
    }
}
#else
//
// Get the path to the default configuration dir to use, if NULL
// is passed to M_SetConfigDir.
//
static char* M_GetDefaultConfigDir() {
    return M_StringDuplicate(exedir);
}
#endif

//
// Sets the location of the configuration directory, where configuration
// files are stored - default.cfg, broom.cfg, savegames, etc.
//
void M_SetConfigDir(const char* dir) {
    // Use the directory that was passed, or find the default.
    configdir = (dir != NULL) ? dir : M_GetDefaultConfigDir();
    if (strcmp(configdir, exedir) != 0) {
        printf("Using %s for configuration and saves\n", configdir);
    }
    // Make the directory if it doesn't already exist.
    M_MakeDirectory(configdir);
}

#define MUSIC_PACK_README                                                      \
    "Extract music packs into this directory in .flac or .ogg format;\n"       \
    "they will be automatically loaded based on filename to replace the\n"     \
    "in-game music with high quality versions.\n\n"                            \
    "For more information check here:\n\n"                                     \
    "  "                                                                       \
    "<https://www.chocolate-doom.org/wiki/index.php/Digital_music_packs>\n\n"

// Set the value of music_pack_path if it is currently empty, and create
// the directory if necessary.
void M_SetMusicPackDir(void) {
    const char* current_path = M_GetStringVariable("music_pack_path");
    if (current_path != NULL && strlen(current_path) > 0) {
        return;
    }

    char* prefdir = SDL_GetPrefPath("", PACKAGE_TARNAME);
    if (prefdir == NULL) {
        printf("M_SetMusicPackDir: SDL_GetPrefPath failed, music pack "
               "directory not set\n");
        return;
    }
    char* music_pack_path = M_StringJoin(prefdir, "music-packs", NULL);

    M_MakeDirectory(prefdir);
    M_MakeDirectory(music_pack_path);
    M_SetVariable("music_pack_path", music_pack_path);

    // We write a README file with some basic instructions on how to use
    // the directory.
    char* readme_path =
        M_StringJoin(music_pack_path, DIR_SEPARATOR_S, "README.txt", NULL);
    M_WriteFile(readme_path, MUSIC_PACK_README, strlen(MUSIC_PACK_README));

    free(readme_path);
    free(music_pack_path);
    SDL_free(prefdir);
}

//
// Calculate the path to the directory to use to store save games.
// Creates the directory as necessary.
//
char* M_GetSaveGameDir(const char* iwadname) {
    //!
    // @arg <directory>
    //
    // Specify a path from which to load and save games. If the directory
    // does not exist then it will automatically be created.
    //
    int p = M_CheckParmWithArgs("-savedir", 1);
    if (p) {
        char* savegamedir = myargv[p + 1];
        if (!M_FileExists(savegamedir)) {
            M_MakeDirectory(savegamedir);
        }
        // Add separator at end just in case.
        savegamedir = M_StringJoin(savegamedir, DIR_SEPARATOR_S, NULL);
        printf("Save directory changed to %s.\n", savegamedir);
        return savegamedir;
    }

#ifdef _WIN32
    if (M_ParmExists("-cdrom")) {
        // In -cdrom mode, we write savegames to a specific
        // directory in addition to configs.
        return M_StringDuplicate(configdir);
    }
#endif

    if (!strcmp(configdir, exedir)) {
        // If not "doing" a configuration directory (Windows),
        // don't "do" a savegame directory, either.
        return M_StringDuplicate("");
    }

    // ~/.local/share/broom/savegames
    char* topdir = M_StringJoin(configdir, "savegames", NULL);
    M_MakeDirectory(topdir);

    // eg. ~/.local/share/broom/savegames/doom2.wad/
    char* savegamedir =
        M_StringJoin(topdir, DIR_SEPARATOR_S, iwadname, DIR_SEPARATOR_S, NULL);

    M_MakeDirectory(savegamedir);

    free(topdir);

    return savegamedir;
}

//
// Calculate the path to the directory for autoloaded WADs/DEHs.
// Creates the directory as necessary.
//
char* M_GetAutoloadDir(const char* iwadname) {
    char* result;

    if (autoload_path == NULL || strlen(autoload_path) == 0) {
        char* prefdir;
        prefdir = SDL_GetPrefPath("", PACKAGE_TARNAME);
        if (prefdir == NULL) {
            printf("M_GetAutoloadDir: SDL_GetPrefPath failed\n");
            return NULL;
        }
        autoload_path = M_StringJoin(prefdir, "autoload", NULL);
        SDL_free(prefdir);
    }

    M_MakeDirectory(autoload_path);

    result = M_StringJoin(autoload_path, DIR_SEPARATOR_S, iwadname, NULL);
    M_MakeDirectory(result);

    // TODO: Add README file

    return result;
}
