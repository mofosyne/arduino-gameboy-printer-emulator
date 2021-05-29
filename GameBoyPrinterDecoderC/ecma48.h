/*
  Title: ECMA-48 ANSI ESC CODE (e.g. VT100)
  Author: Brian Khuu 2021
  About: Simple header files for inclusion in embedded system for serial interfaces
  Applications: Adding colors to VT100 serial terminal emulators.
  Tips: This headerfile also contains some code that you can use to test/preview all the codes

  Ref: https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit
  Ref: http://osr507doc.sco.com/man/html.HW/screen.HW.html
  Ref: SO/IEC 6429 https://www.ecma-international.org/publications-and-standards/standards/ecma-48/
  Ref: https://en.wikipedia.org/wiki/C0_and_C1_control_codes

*/

/* ECMA48 5.4 Control sequences  */
// Dev Notes: Defined by ISO-6429 (aka EMCA-48) and ISO-2022 (aka ECMA-35)

// C0 : ASCII control codes
#define ECMA48_C0_BEL '\x07'    ///< | ^G | Bell            | Makes an audible noise.                                                                                                                            |
#define ECMA48_C0_BS  '\x08'    ///< | ^H | Backspace       | Moves the cursor left (but may "backwards wrap" if cursor is at start of line).                                                                    |
#define ECMA48_C0_TAB '\x09'    ///< | ^I | Tab             | Moves the cursor right to next multiple of 8.                                                                                                      |
#define ECMA48_C0_LF  '\x0A'    ///< | ^J | Line Feed       | Moves to next line, scrolls the display up if at bottom of the screen. Usually does not move horizontally, though programs should not rely on this.|
#define ECMA48_C0_FF  '\x0C'    ///< | ^L | Form Feed       | Move a printer to top of next page. Usually does not move horizontally, though programs should not rely on this. Effect on video terminals varies. |
#define ECMA48_C0_CR  '\x0D'    ///< | ^M | Carriage Return | Moves the cursor to column zero.                                                                                                                   |
#define ECMA48_C0_ESC '\x1B'    ///< | ^[ | Escape          | Starts all the escape sequences

// C1 : Control Codes (Just the relevant bits for terminal usage)
#define ECMA48_C1_DCS 'P'       ///< Device Control String    (DCS)
#define ECMA48_C1_CSI '['       ///< Control Sequence Inducer (CSI)
#define ECMA48_C1_OSC ']'       ///< Operating System Command (OSC)

// C0C1 : Beginning sequence
#define ECMA48_ESC_CSI "\x1B["  ///< 'ESC CSI'

/******************************************************************************
 * Cursor Control
******************************************************************************/
#define _ECMA48_CSI_CUU(N)                      ECMA48_ESC_CSI #N "A"
#define _ECMA48_CSI_CUD(N)                      ECMA48_ESC_CSI #N "B"
#define _ECMA48_CSI_CUF(N)                      ECMA48_ESC_CSI #N "C"
#define _ECMA48_CSI_CUB(N)                      ECMA48_ESC_CSI #N "D"
#define _ECMA48_CSI_CNL(N)                      ECMA48_ESC_CSI #N "E"
#define _ECMA48_CSI_CPL(N)                      ECMA48_ESC_CSI #N "F"
#define _ECMA48_CSI_CHA(N)                      ECMA48_ESC_CSI #N "G"
#define _ECMA48_CSI_CUP(N, M)                   ECMA48_ESC_CSI #N ";" #M "H"
#define _ECMA48_CSI_ED(N)                       ECMA48_ESC_CSI #N "J"
#define _ECMA48_CSI_EL(N)                       ECMA48_ESC_CSI #N "K"
#define _ECMA48_CSI_SU(N)                       ECMA48_ESC_CSI #N "S"
#define _ECMA48_CSI_SD(N)                       ECMA48_ESC_CSI #N "T"
#define _ECMA48_CSI_HVP(N, M)                   ECMA48_ESC_CSI #N ";" #M "f"
#define _ECMA48_CSI_SGR(N)                      ECMA48_ESC_CSI #N "m"
#define _ECMA48_CSI_SGR_COLOR_8BIT(N, BYTE)     ECMA48_ESC_CSI #N ";5;" #BYTE "m"
#define _ECMA48_CSI_SGR_COLOR_24BIT(N, R, G, B) ECMA48_ESC_CSI #N ";2;" #R ";" #G ";" #B "m"
#define _ECMA48_CSI_AUX_PORT_ON                 ECMA48_ESC_CSI "5i"
#define _ECMA48_CSI_AUX_PORT_OFF                ECMA48_ESC_CSI "4i"
#define _ECMA48_CSI_DSR                         ECMA48_ESC_CSI "6n"

#define ECMA48_CSI_CUU(N)                      _ECMA48_CSI_CUU(N)                        ///< Cursor Up
#define ECMA48_CSI_CUD(N)                      _ECMA48_CSI_CUD(N)                        ///< Cursor Down
#define ECMA48_CSI_CUF(N)                      _ECMA48_CSI_CUF(N)                        ///< Cursor Forward
#define ECMA48_CSI_CUB(N)                      _ECMA48_CSI_CUB(N)                        ///< Cursor Back
#define ECMA48_CSI_CNL(N)                      _ECMA48_CSI_CNL(N)                        ///< Cursor Next Line
#define ECMA48_CSI_CPL(N)                      _ECMA48_CSI_CPL(N)                        ///< Cursor Previous Line
#define ECMA48_CSI_CHA(N)                      _ECMA48_CSI_CHA(N)                        ///< Cursor Horizontal Absolute
#define ECMA48_CSI_CUP(N, M)                   _ECMA48_CSI_CUP(N, M)                     ///< Cursor Position
#define ECMA48_CSI_ED(N)                       _ECMA48_CSI_ED(N)                         ///< Erase in Display
#define ECMA48_CSI_EL(N)                       _ECMA48_CSI_EL(N)                         ///< Erase in Line
#define ECMA48_CSI_SU(N)                       _ECMA48_CSI_SU(N)                         ///< Scroll Up
#define ECMA48_CSI_SD(N)                       _ECMA48_CSI_SD(N)                         ///< Scroll Down
#define ECMA48_CSI_HVP(N, M)                   _ECMA48_CSI_HVP(N, M)                     ///< Horizontal Vertical Position
#define ECMA48_CSI_SGR(N)                      _ECMA48_CSI_SGR(N)                        ///< Select Graphic Rendition
#define ECMA48_CSI_SGR_COLOR_8BIT(N, BYTE)     _ECMA48_CSI_SGR_COLOR_8BIT(N,BYTE)        ///< Select Graphic Rendition (Color 8bit)
#define ECMA48_CSI_SGR_COLOR_24BIT(N, R, G, B) _ECMA48_CSI_SGR_COLOR_24BIT(N, R, G, B)   ///< Select Graphic Rendition (Color 24bit)
#define ECMA48_CSI_AUX_PORT_ON                 _ECMA48_CSI_AUX_PORT_ON                   ///< AUX Port On
#define ECMA48_CSI_AUX_PORT_OFF                _ECMA48_CSI_AUX_PORT_OFF                  ///< AUX Port Off
#define ECMA48_CSI_DSR                         _ECMA48_CSI_DSR                           ///< Device Status Report


/******************************************************************************
 * SGR (Select Graphic Rendition)
******************************************************************************/
/* ECMA48 5.4 Control sequences : SGR (Select Graphic Rendition) parameters */
                                                       ///  |          | Name                                                        |
                                                       ///  | -------- | ----------------------------------------------------------- |
#define ECMA48_SGR_RESET                           0   ///< | 0        | Reset or normal                                             |
#define ECMA48_SGR_BOLD                            1   ///< | 1        | Bold or increased intensity                                 |
#define ECMA48_SGR_FAINT                           2   ///< | 2        | Faint, decreased intensity, or dim                          |
#define ECMA48_SGR_ITALIC                          3   ///< | 3        | Italic                                                      |
#define ECMA48_SGR_UNDERLINE                       4   ///< | 4        | Underline                                                   |
#define ECMA48_SGR_SLOWBLINK                       5   ///< | 5        | Slow blink                                                  |
#define ECMA48_SGR_RAPIDBLINK                      6   ///< | 6        | Rapid blink                                                 |
#define ECMA48_SGR_INVERT                          7   ///< | 7        | Reverse video or invert                                     |
#define ECMA48_SGR_CONCEAL                         8   ///< | 8        | Conceal or hide                                             |
#define ECMA48_SGR_STRIKEOUT                       9   ///< | 9        | Crossed-out, or strike                                      |
#define ECMA48_SGR_DEFAULT_FONT                    10  ///< | 10       | Primary (default) font                                      |
#define ECMA48_SGR_ALTFONT_0                       11  ///< | 11–19    | Alternative font                                            |
#define ECMA48_SGR_ALTFONT_1                       12
#define ECMA48_SGR_ALTFONT_2                       13
#define ECMA48_SGR_ALTFONT_3                       14
#define ECMA48_SGR_ALTFONT_4                       15
#define ECMA48_SGR_ALTFONT_5                       16
#define ECMA48_SGR_ALTFONT_6                       17
#define ECMA48_SGR_ALTFONT_7                       18
#define ECMA48_SGR_ALTFONT_8                       19
#define ECMA48_SGR_BLACKLETTER                     20  ///< | 20       | Blackletter font                                            |
#define ECMA48_SGR_DOUBLE_UNDERLINE                21  ///< | 21       | Doubly underlined; or: not bold                             |
#define ECMA48_SGR_NORMAL_INTENSITY                22  ///< | 22       | Normal intensity                                            |
#define ECMA48_SGR_CLR_ITALIC_BLACKLETTER          23  ///< | 23       | Neither italic, nor blackletter                             |
#define ECMA48_SGR_CLR_UNDERLINE                   24  ///< | 24       | Not underlined                                              |
#define ECMA48_SGR_CLR_BLINK                       25  ///< | 25       | Not blinking                                                |
#define ECMA48_SGR_PROPORTIONAL_SPACING            26  ///< | 26       | Proportional spacing                                        |
#define ECMA48_SGR_CLR_REVERSE                     27  ///< | 27       | Not reversed                                                |
#define ECMA48_SGR_CLR_CONCEALED                   28  ///< | 28       | Reveal                                                      |
#define ECMA48_SGR_CLR_CROSSEDOUT                  29  ///< | 29       | Not crossed out                                             |
#define ECMA48_SGR_FOREGROUND_COLOR_BLACK          30  ///< | 30–37    | Set foreground color                                        |
#define ECMA48_SGR_FOREGROUND_COLOR_RED            31
#define ECMA48_SGR_FOREGROUND_COLOR_GREEN          32
#define ECMA48_SGR_FOREGROUND_COLOR_YELLOW         33
#define ECMA48_SGR_FOREGROUND_COLOR_BLUE           34
#define ECMA48_SGR_FOREGROUND_COLOR_MAGENTA        35
#define ECMA48_SGR_FOREGROUND_COLOR_CYAN           36
#define ECMA48_SGR_FOREGROUND_COLOR_WHITE          37
#define ECMA48_SGR_FOREGROUND_COLOR_EXTENDED       38  ///< | 38       | Set foreground color                                        |
#define ECMA48_SGR_FOREGROUND_COLOR_DEFAULT        39  ///< | 39       | Default foreground color                                    |
#define ECMA48_SGR_BACKGROUND_COLOR_BLACK          40  ///< | 40–47   | Set background color                                        |
#define ECMA48_SGR_BACKGROUND_COLOR_RED            41
#define ECMA48_SGR_BACKGROUND_COLOR_GREEN          42
#define ECMA48_SGR_BACKGROUND_COLOR_YELLOW         43
#define ECMA48_SGR_BACKGROUND_COLOR_BLUE           44
#define ECMA48_SGR_BACKGROUND_COLOR_MAGENTA        45
#define ECMA48_SGR_BACKGROUND_COLOR_CYAN           46
#define ECMA48_SGR_BACKGROUND_COLOR_WHITE          47
#define ECMA48_SGR_BACKGROUND_COLOR_EXTENDED       48  ///< | 48       | Set background color                                        |
#define ECMA48_SGR_BACKGROUND_COLOR_DEFAULT        49  ///< | 49       | Default background color                                    |
#define ECMA48_SGR_CLR_PROPORTIONAL_SPACING        50  ///< | 50       | Disable proportional spacing                                |
#define ECMA48_SGR_FRAMED                          51  ///< | 51       | Framed                                                      |
#define ECMA48_SGR_ENCIRCLED                       52  ///< | 52       | Encircled                                                   |
#define ECMA48_SGR_OVERLINED                       53  ///< | 53       | Overlined                                                   |
#define ECMA48_SGR_CLR_FRAMED                      54  ///< | 54       | Neither framed nor encircled                                |
#define ECMA48_SGR_CLR_OVERLINED                   55  ///< | 55       | Not overlined                                               |
#define ECMA48_SGR_UNDERLINE_EXTENDED              58  ///< | 58       | Set underline color                                         |
#define ECMA48_SGR_UNDERLINE_DEFAULT               59  ///< | 59       | Default underline color                                     |
#define ECMA48_SGR_IDEOGRAM_UNDERLINE              60  ///< | 60       | Ideogram underline or right side line                       |
#define ECMA48_SGR_IDEOGRAM_DOUBLE_UNDERLINE       61  ///< | 61       | Ideogram double underline, or double line on the right side |
#define ECMA48_SGR_IDEOGRAM_OVERLINE               62  ///< | 62       | Ideogram overline or left side line                         |
#define ECMA48_SGR_IDEOGRAM_DOUBLE_OVERLINE        63  ///< | 63       | Ideogram double overline, or double line on the left side   |
#define ECMA48_SGR_IDEOGRAM_STRESS_MARKING         64  ///< | 64       | Ideogram stress marking                                     |
#define ECMA48_SGR_CLR_IDEOGRAM                    65  ///< | 65       | No ideogram attributes                                      |
#define ECMA48_SGR_SUPERSCRIPT                     73  ///< | 73       | Superscript                                                 |
#define ECMA48_SGR_SUBSCRIPT                       74  ///< | 74       | Subscript                                                   |
#define ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_BLACK   90  ///< | 90–97    | Set bright foreground color                                 |
#define ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_RED     91
#define ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_GREEN   92
#define ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_YELLOW  93
#define ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_BLUE    94
#define ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_MAGENTA 95
#define ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_CYAN    96
#define ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_WHITE   97
#define ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_BLACK   100 ///< | 100–107 | Set bright background color                                 |
#define ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_RED     101
#define ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_GREEN   102
#define ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_YELLOW  103
#define ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_BLUE    104
#define ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_MAGENTA 105
#define ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_CYAN    106
#define ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_WHITE   107

/* Macros (Useful for fixed printf() based logging) */
#define ECMA48_SGR_FOREGROUND_COLOR_8BIT(BYTE)   ECMA48_CSI_SGR_COLOR_8BIT( ECMA48_SGR_FOREGROUND_COLOR_EXTENDED,  BYTE)
#define ECMA48_SGR_FOREGROUND_COLOR_24BIT(R,G,B) ECMA48_CSI_SGR_COLOR_24BIT(ECMA48_SGR_FOREGROUND_COLOR_EXTENDED, R,G,B)
#define ECMA48_SGR_BACKGROUND_COLOR_8BIT(BYTE)   ECMA48_CSI_SGR_COLOR_8BIT( ECMA48_SGR_BACKGROUND_COLOR_EXTENDED,  BYTE)
#define ECMA48_SGR_BACKGROUND_COLOR_24BIT(R,G,B) ECMA48_CSI_SGR_COLOR_24BIT(ECMA48_SGR_BACKGROUND_COLOR_EXTENDED, R,G,B)


/******************************************************************************
*                             TEST/DEMO CASES                                 *
******************************************************************************/
#if 0 ///< Uncomment and compile as a single C file to preview
#include <stdio.h>
void main (void)
{
#define PREVIEWESC(X) printf(ECMA48_CSI_SGR(0) " %60s : " X "XXXXX" ECMA48_CSI_SGR(0) "\r\n",  #X)
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_RESET                           ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BOLD                            ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FAINT                           ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_ITALIC                          ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_UNDERLINE                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_SLOWBLINK                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_RAPIDBLINK                      ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_INVERT                          ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_CONCEAL                         ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_STRIKEOUT                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_DEFAULT_FONT                    ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_ALTFONT_0                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_ALTFONT_1                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_ALTFONT_2                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_ALTFONT_3                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_ALTFONT_4                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_ALTFONT_5                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_ALTFONT_6                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_ALTFONT_7                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_ALTFONT_8                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BLACKLETTER                     ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_DOUBLE_UNDERLINE                ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_NORMAL_INTENSITY                ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_CLR_ITALIC_BLACKLETTER          ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_CLR_UNDERLINE                   ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_CLR_BLINK                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_PROPORTIONAL_SPACING            ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_CLR_REVERSE                     ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_CLR_CONCEALED                   ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_CLR_CROSSEDOUT                  ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FOREGROUND_COLOR_BLACK          ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FOREGROUND_COLOR_RED            ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FOREGROUND_COLOR_GREEN          ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FOREGROUND_COLOR_YELLOW         ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FOREGROUND_COLOR_BLUE           ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FOREGROUND_COLOR_MAGENTA        ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FOREGROUND_COLOR_CYAN           ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FOREGROUND_COLOR_WHITE          ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FOREGROUND_COLOR_EXTENDED       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FOREGROUND_COLOR_DEFAULT        ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BACKGROUND_COLOR_BLACK          ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BACKGROUND_COLOR_RED            ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BACKGROUND_COLOR_GREEN          ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BACKGROUND_COLOR_YELLOW         ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BACKGROUND_COLOR_BLUE           ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BACKGROUND_COLOR_MAGENTA        ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BACKGROUND_COLOR_CYAN           ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BACKGROUND_COLOR_WHITE          ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BACKGROUND_COLOR_EXTENDED       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BACKGROUND_COLOR_DEFAULT        ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_CLR_PROPORTIONAL_SPACING        ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_FRAMED                          ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_ENCIRCLED                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_OVERLINED                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_CLR_FRAMED                      ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_CLR_OVERLINED                   ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_UNDERLINE_EXTENDED              ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_UNDERLINE_DEFAULT               ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_IDEOGRAM_UNDERLINE              ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_IDEOGRAM_DOUBLE_UNDERLINE       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_IDEOGRAM_OVERLINE               ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_IDEOGRAM_DOUBLE_OVERLINE        ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_IDEOGRAM_STRESS_MARKING         ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_CLR_IDEOGRAM                    ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_SUPERSCRIPT                     ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_SUBSCRIPT                       ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_BLACK   ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_RED     ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_GREEN   ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_YELLOW  ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_BLUE    ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_MAGENTA ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_CYAN    ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_FOREGROUND_COLOR_WHITE   ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_BLACK   ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_RED     ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_GREEN   ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_YELLOW  ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_BLUE    ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_MAGENTA ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_CYAN    ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_WHITE   ));
    PREVIEWESC(ECMA48_CSI_SGR( ECMA48_SGR_BRIGHT_BACKGROUND_COLOR_WHITE   ));
    PREVIEWESC(ECMA48_SGR_FOREGROUND_COLOR_8BIT(24)                    );
    PREVIEWESC(ECMA48_SGR_FOREGROUND_COLOR_24BIT(2, 4, 55)             );
    PREVIEWESC(ECMA48_SGR_BACKGROUND_COLOR_8BIT(57)                    );
    PREVIEWESC(ECMA48_SGR_BACKGROUND_COLOR_24BIT(23, 5, 45)            );
    printf( "\n\n\n\n\n\n\n\n\n\n\n\n");
    printf( ECMA48_CSI_CUU(10)     "CUU");
    printf( ECMA48_CSI_CUD(2)     "CUD");
    printf( ECMA48_CSI_CUF(3)     "CUF");
    printf( ECMA48_CSI_CUB(4)     "CUB");
    printf( ECMA48_CSI_CNL(5)     "CNL");
    printf( ECMA48_CSI_CPL(6)     "CPL");
    printf( ECMA48_CSI_CHA(7)     "CHA");
    printf( ECMA48_CSI_CUP(20,20) "CUP");
}
#endif