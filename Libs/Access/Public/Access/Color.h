#pragma once

#include "CoreMinimal.h"

// Logs to comment out
//      Could not find Glyph Index 0
//          Engine\Source\Runtime\SlateCore\Private\Fonts\SlateFontRenderer.cpp
//      Using fallback font
//          Engine\Source\Runtime\SlateCore\Private\Fonts\FontCacheCompositeFont.cpp
// // It happens due to ANSI escape codes in the UE_LOG

class Color
{
public:
    static const FColor White;
    static const FColor Black;    // Black (Waiting for Action prior to VPIP)
    static const FColor Grey;     // Folded (Actions are Gryed Out)

    static const FColor Yellow;   // Bet
    static const FColor Orange;   // Raise (3-Bet)
    static const FColor Red;      // 4-Bet+

    static const FColor Green;      // Limp or Blinds
    static const FColor Turquoise;  // Called Bet
    static const FColor Blue;       // Called Raise
    static const FColor Purple;     // Called 4-Bet+
};

#define RESET               "\x1b[0m"
#define BOLD                "\x1b[1m"
#define DIM                 "\x1b[2m"
#define UNDERLINE           "\x1b[4m"
#define BLINK               "\x1b[5m"
#define REVERSE             "\x1b[7m"
#define HIDDEN              "\x1b[8m"

// Standard foreground colors
#define BLACK               "\x1b[30m"
#define RED                 "\x1b[31m"
#define GREEN               "\x1b[32m"
#define YELLOW              "\x1b[33m"
#define BLUE                "\x1b[34m"
#define MAGENTA             "\x1b[35m"
#define CYAN                "\x1b[36m"
#define WHITE               "\x1b[37m"

// Bright foreground colors
#define BRIGHT_BLACK        "\x1b[90m"
#define BRIGHT_RED          "\x1b[91m"
#define BRIGHT_GREEN        "\x1b[92m"
#define BRIGHT_YELLOW       "\x1b[93m"
#define BRIGHT_BLUE         "\x1b[94m"
#define BRIGHT_MAGENTA      "\x1b[95m"
#define BRIGHT_CYAN         "\x1b[96m"
#define BRIGHT_WHITE        "\x1b[97m"

// Standard background colors
#define BG_BLACK            "\x1b[40m"
#define BG_RED              "\x1b[41m"
#define BG_GREEN            "\x1b[42m"
#define BG_YELLOW           "\x1b[43m"
#define BG_BLUE             "\x1b[44m"
#define BG_MAGENTA          "\x1b[45m"
#define BG_CYAN             "\x1b[46m"
#define BG_WHITE            "\x1b[47m"

// Bright background colors
#define BG_BRIGHT_BLACK     "\x1b[100m"
#define BG_BRIGHT_RED       "\x1b[101m"
#define BG_BRIGHT_GREEN     "\x1b[102m"
#define BG_BRIGHT_YELLOW    "\x1b[103m"
#define BG_BRIGHT_BLUE      "\x1b[104m"
#define BG_BRIGHT_MAGENTA   "\x1b[105m"
#define BG_BRIGHT_CYAN      "\x1b[106m"
#define BG_BRIGHT_WHITE     "\x1b[107m"

// 256-color / Truecolor helpers (use with FString::Printf)
// Example: FString::Printf("\x1b[38;2;%d;%d;%dm"), R,G,B)
#define FG_TRUECOLOR(R,G,B) "\x1b[38;2;" #R ";" #G ";" #B "m"
#define BG_TRUECOLOR(R,G,B) "\x1b[48;2;" #R ";" #G ";" #B "m"

