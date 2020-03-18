/*
 * AnsiTerm.cpp
 *      Windows ANSI Terminal Emulation
 *
 * Author:  Robert Nelson robertnelson at users.sourceforge.net
 * Copyright (c) 2009 Robert Nelson
 *
 * 10/07/09 Robert Nelson - Created
 * 10/08/09 Robert Nelson
 *  - Fixed bug with resetting attribute to Black on Black on exit
 *  - Fixed setting of attribute when erasing
 *  - Added automatic handling of buffer resize events
 *  - Added display of unrecognized escape sequences
 * 10/15/09 Robert Nelson
 *  - Fixed display problems caused by custom ColorTable used by cmd.exe
 *  - Fixed cursor positioning problems with OriginMode.
 *  - Changed to use block cursor because I think its more terminal like :-)
 *  - Added Reset handling
 *  - Added Cursor Show / Hide
 * 10/16/09 Robert Nelson
 *  - Better handling of ColorTable.
 * 10/17/09 Robert Nelson
 *  - Use GetProcAddress for (Get/Set)ConsoleScreenBufferInfoEx since they
 *    are only available on Vista and beyond.
 * 01/27/2015 Andy Cress
 *  - handle ProcessRM asserts
 *
 * Todo:
 *  - Implement soft tabs
 */

/*
Copyright (c) 2009, Robert Nelson
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  a.. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer. 
  b.. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include "AnsiTerm.h"

extern "C" unsigned char fCRLF;
extern "C" void dbglog( char *pattn, ... );

CAnsiTerm::CSICode CAnsiTerm::s_CSITable[] =
{
    { '[', DS_CSIParam },
    { '#', DS_DECPrivate },
    { '(', DS_SelectG0 },
    { ')', DS_SelectG1 },
    { '=', DS_None, &ProcessDECKPAM },  // Keypad Application Mode
    { '>', DS_None, &ProcessDECKPNM },  // Keypad Numeric Mode
    { '7', DS_None, &ProcessDECSC },    // Save Cursor
    { '8', DS_None, &ProcessDECRC },    // Restore Cursor
    { 'D', DS_None, &ProcessIND },      // Index
    { 'E', DS_None, &ProcessNEL },      // Next Line
    { 'H', DS_None, &ProcessHTS },      // Horizontal Tab Set
    { 'M', DS_None, &ProcessRI },       // Reverse Index
    { 'Z', DS_None, &ProcessDECID },    // Identify Terminal
    { 'c', DS_None, &ProcessRIS },      // Reset to Initial State
    { 's', DS_None, &ProcessSCP },      // Save Cursor Position
    { 'u', DS_None, &ProcessRCP },      // Restore Cursor Position
    { '\0' }
};

CAnsiTerm::CSIFunction   CAnsiTerm::s_DECFunction[] =
{
    { '3', &ProcessDECDHLT },           // Double Height Line Top
    { '4', &ProcessDECDHLB },           // Double Height Line Bottom
    { '5', &ProcessDECSWL },            // Single Width Line
    { '6', &ProcessDECDWL },            // Double Width Line
    { '8', &ProcessDECALN },            // Screen Alignment Display
    { '\0' }
};

CAnsiTerm::CSIFunction   CAnsiTerm::s_CSIFunction[] =
{
    { 'A', &ProcessCUU },               // Cursor Up
    { 'B', &ProcessCUD },               // Cursor Down
    { 'C', &ProcessCUF },               // Cursor Forward
    { 'D', &ProcessCUB },               // Cursor Backward
    { 'H', &ProcessCUP },               // Cursor Position
    { 'J', &ProcessED },                // Erase in Display
    { 'K', &ProcessEL },                // Erase in Line
    { 'c', &ProcessDA },                // Device Attributes
    { 'f', &ProcessHVP },               // Horizontal and Vertical Position
    { 'g', &ProcessTBC },               // Tabulation Clear
    { 'h', &ProcessSM },                // Set Mode
    { 'l', &ProcessRM },                // Reset Mode
    { 'm', &ProcessSGR },               // Select Graphics Rendition
    { 'n', &ProcessDSR },               // Device Status Report
    { 'q', &ProcessDECLL },             // DEC Load LEDs
    { 'r', &ProcessDECSTBM },           // DEC Set Top and Bottom Margins
    { 'x', &ProcessDECREQTPARM },       // Request Terminal Parameters
    { 'y', &ProcessDECTST },            // Invoke Confidence Test
    { '\0' }
};

wchar_t CAnsiTerm::s_GraphicChars[kMaxGraphicsChar - kMinGraphicsChar + 1] =
{
    0x0020,     // 0137 5F  95 _ Blank
    0x2666,     // 0140 60  96 ` Diamond
    0x2592,     // 0141 61  97 a Checkerboard
    0x2409,     // 0142 62  98 b Horizontal Tab
    0x240C,     // 0143 63  99 c Form Feed
    0x240D,     // 0144 64 100 d Carriage Return
    0x240A,     // 0145 65 101 e Line Feed
    0x00B0,     // 0146 66 102 f Degree Symbol
    0x00B1,     // 0147 67 103 g Plus/Minus
    0x2424,     // 0150 68 104 h New Line
    0x240B,     // 0151 69 105 i Vertical Tab
    0x2518,     // 0152 6A 106 j Lower-right corner
    0x2510,     // 0153 6B 107 k Upper-right corner
    0x250C,     // 0154 6C 108 l Upper-left corner
    0x2514,     // 0155 6D 109 m Lower-left corner
    0x253C,     // 0156 6E 110 n Crossing Lines
    0x00AF,     // 0157 6F 111 o Horizontal Line - Scan 1
    0x0070,     // 0160 70 112 p Horizontal Line - Scan 3 (No translation)
    0x2500,     // 0161 71 113 q Horizontal Line - Scan 5
    0x0072,     // 0162 72 114 r Horizontal Line - Scan 7 (No translation)
    0x005F,     // 0163 73 115 s Horizontal Line - Scan 9
    0x251C,     // 0164 74 116 t Left "T"
    0x2524,     // 0165 75 117 u Right "T"
    0x2534,     // 0166 76 118 v Bottom "T"
    0x252C,     // 0167 77 119 w Top "T"
    0x2502,     // 0170 78 120 x | Vertical bar
    0x2264,     // 0171 79 121 y Less than or equal to
    0x2265,     // 0172 7A 122 z Greater than or equal to
    0x03C0,     // 0173 7B 123 { Pi
    0x2260,     // 0174 7C 124 | Not equal to
    0x00A3,     // 0175 7D 125 } UK Pound Sign
    0x00B7      // 0176 7E 126 ~ Centered dot
};

wchar_t CAnsiTerm::s_OemToUnicode[256] =
{
    0x2007, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, // 00-07
    0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C, // 08-0F
    0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8, // 10-17
    0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC, // 18-1F
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, // 20-27
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, // 28-2F
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, // 30-37
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, // 38-3F
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, // 40-47
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, // 48-4F
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, // 50-57
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, // 58-5F
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, // 60-67
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, // 68-6F
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, // 70-77
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F, // 78-7F
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, // 80-87
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5, // 88-8F
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, // 90-97
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192, // 98-9F
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, // A0-A7
    0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB, // A8-AF
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, // B0-B7
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510, // B8-BF
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, // C0-C7
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567, // C8-CF
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, // D0-D7
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580, // D8-DF
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, // E0-E7
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229, // E8-EF
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, // F0-F7
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0  // F8-FF
};

COLORREF    CAnsiTerm::s_ColorTable[kColorTableSize] =
{
    RGB(0, 0, 0),
    RGB(0, 0, 128),
    RGB(0, 128, 0),
    RGB(0, 128, 128),
    RGB(128, 0, 0),
    RGB(128, 0, 128),
    RGB(128, 128, 0),
    RGB(192, 192, 192),
    RGB(128, 128, 128),
    RGB(0, 0, 255),
    RGB(0, 255, 0),
    RGB(0, 255, 255),
    RGB(255, 0, 0),
    RGB(255, 0, 255),
    RGB(255, 255, 0),
    RGB(255, 255, 255)
};

CAnsiTerm::PFN_GetConsoleScreenBufferInfoEx CAnsiTerm::s_pfnGetConsoleScreenBufferInfoEx;
CAnsiTerm::PFN_SetConsoleScreenBufferInfoEx CAnsiTerm::s_pfnSetConsoleScreenBufferInfoEx;

CAnsiTerm::CAnsiTerm(void)
{
    m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleMode(m_hConsole, &m_dwOrigConsoleMode);

    SetConsoleMode(m_hConsole, ENABLE_PROCESSED_OUTPUT);

    CONSOLE_CURSOR_INFO     cursorInfo;

    GetConsoleCursorInfo(m_hConsole, &cursorInfo);

    m_dwOrigCursorSize = cursorInfo.dwSize;

    WindowSizeChanged(true);
}

CAnsiTerm::~CAnsiTerm(void)
{
    CONSOLE_CURSOR_INFO             cursorInfo = { m_dwOrigCursorSize, TRUE };

    SetConsoleCursorInfo(m_hConsole, &cursorInfo);

    SetConsoleMode(m_hConsole, m_dwOrigConsoleMode);

    SetConsoleTextAttribute(m_hConsole, m_wOrigConsoleAttribute);

    if (m_bResetColorTable)
    {
        CONSOLE_SCREEN_BUFFER_INFOEX    consoleInfo = { sizeof(consoleInfo) };

        s_pfnGetConsoleScreenBufferInfoEx(m_hConsole, &consoleInfo);

        memcpy(consoleInfo.ColorTable, m_OrigColorTable, kColorTableSize * sizeof(consoleInfo.ColorTable[0]));

        // There is a bug between GetConsoleScreenBufferInfoEx and SetConsoleScreenBufferInfoEx.
        // The first treats srWindow.Right and srWindow.Bottom as inclusive and the latter as exclusive.
        consoleInfo.srWindow.Right++;
        consoleInfo.srWindow.Bottom++;

        s_pfnSetConsoleScreenBufferInfoEx(m_hConsole, &consoleInfo);

        // Reset the attributes on existing lines so that at least white on black looks
        // correct.
        COORD   coordStart = { 0, 0 };
        DWORD   dwWritten;

        FillConsoleOutputAttribute(m_hConsole, m_wOrigConsoleAttribute, m_BufferSize.X * m_BufferSize.Y, coordStart, &dwWritten);
    }

    CloseHandle(m_hConsole);
}

void
CAnsiTerm::WindowSizeChanged(bool bInitial)
{
    WORD    wCurrentAttribute;
    COORD   dwCurrentCursorPosition;

    if (bInitial)
    {
        HMODULE hKernel32 = GetModuleHandle("kernel32");

        if (hKernel32 != NULL)
        {
            s_pfnGetConsoleScreenBufferInfoEx = (PFN_GetConsoleScreenBufferInfoEx)GetProcAddress(hKernel32, "GetConsoleScreenBufferInfoEx");
            s_pfnSetConsoleScreenBufferInfoEx = (PFN_SetConsoleScreenBufferInfoEx)GetProcAddress(hKernel32, "SetConsoleScreenBufferInfoEx");

            if (s_pfnGetConsoleScreenBufferInfoEx == NULL || s_pfnSetConsoleScreenBufferInfoEx == NULL)
            {
                s_pfnGetConsoleScreenBufferInfoEx = NULL;
                s_pfnSetConsoleScreenBufferInfoEx = NULL;
            }
        }

        m_bResetColorTable = false;
    }

    if (s_pfnGetConsoleScreenBufferInfoEx != NULL)
    {
        CONSOLE_SCREEN_BUFFER_INFOEX    consoleInfo = { sizeof(consoleInfo) };

        s_pfnGetConsoleScreenBufferInfoEx(m_hConsole, &consoleInfo);

        m_BufferSize = consoleInfo.dwSize;

        m_WindowSize.X = consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1;
        m_WindowSize.Y = consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top + 1;

        wCurrentAttribute = consoleInfo.wAttributes;
        dwCurrentCursorPosition = consoleInfo.dwCursorPosition;

        if (bInitial)
        {
            m_bResetColorTable = memcmp(consoleInfo.ColorTable, s_ColorTable, sizeof(s_ColorTable)) != 0;

            if (m_bResetColorTable)
            {
                // The command prompt (cmd.exe) uses a nonstandard color table
                // So we save it away and reset it to match the Console API documentation
                size_t  colorCopyLen = kColorTableSize * sizeof(consoleInfo.ColorTable[0]);

                memcpy(m_OrigColorTable, consoleInfo.ColorTable, colorCopyLen);

                memcpy(consoleInfo.ColorTable, s_ColorTable, colorCopyLen);

                // There is a bug between GetConsoleScreenBufferInfoEx and SetConsoleScreenBufferInfoEx.
                // The first treats srWindow.Right and srWindow.Bottom as inclusive and the latter as exclusive.
                consoleInfo.srWindow.Right++;
                consoleInfo.srWindow.Bottom++;

                s_pfnSetConsoleScreenBufferInfoEx(m_hConsole, &consoleInfo);

                // Reset the attributes on existing lines so that at least white on black looks
                // correct.
                COORD   coordStart = { 0, 0 };
                DWORD   dwWritten;

                FillConsoleOutputAttribute(m_hConsole, kDefaultAttribute, m_BufferSize.X * m_BufferSize.Y, coordStart, &dwWritten);
            }
        }
    }
    else
    {
        CONSOLE_SCREEN_BUFFER_INFO      consoleInfo;

        GetConsoleScreenBufferInfo(m_hConsole, &consoleInfo);

        m_BufferSize = consoleInfo.dwSize;

        m_WindowSize.X = consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1;
        m_WindowSize.Y = consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top + 1;

        wCurrentAttribute = consoleInfo.wAttributes;
        dwCurrentCursorPosition = consoleInfo.dwCursorPosition;
    }

    m_WindowOrigin.X = 0;
    m_WindowOrigin.Y = m_BufferSize.Y - m_WindowSize.Y;

    if (bInitial)
    {
        m_wOrigConsoleAttribute = wCurrentAttribute;

        SHORT nLines = dwCurrentCursorPosition.Y - m_WindowOrigin.Y;

        if (nLines != 0)
        {
            SMALL_RECT  rectSource = { 0, 0, m_BufferSize.X - 1, m_BufferSize.Y - 1 };
            COORD       coordDest = { 0, 0 };
            CHAR_INFO   charInfo = { ' ', kDefaultAttribute };

            if (nLines > 0)
            {
                rectSource.Top = nLines;
            }
            else
            {
                coordDest.Y -= nLines;
                rectSource.Bottom += nLines;
            }

            ScrollConsoleScreenBuffer(m_hConsole, &rectSource, NULL, coordDest, &charInfo);
        }
    }

    ResetTerm();
}

int
CAnsiTerm::ProcessInput(CAnsiTerm::KeyCode keyCode, unsigned char *pOutput, int iOutputSize)
{
    int iOutputLength = 0;

    if (pOutput == NULL || iOutputSize < 1)
    {
        return 0;
    }

    if (!keyCode.bKeyDown)
    {
        return 0;
    }

    if (VK_F1 <= keyCode.VirtualKeyCode && keyCode.VirtualKeyCode <= VK_F12)
    {
        pOutput[iOutputLength++] = CH_ESC;
        pOutput[iOutputLength++] = 'O';
        pOutput[iOutputLength++] = 'P' + keyCode.VirtualKeyCode - VK_F1;
    }
    else
    if (keyCode.VirtualKeyCode == VK_UP ||
        keyCode.VirtualKeyCode == VK_DOWN ||
        keyCode.VirtualKeyCode == VK_RIGHT ||
        keyCode.VirtualKeyCode == VK_LEFT)
    {
        pOutput[iOutputLength++] = CH_ESC;

        if ((m_bCursorKeyMode && !keyCode.bControl) ||
            (!m_bCursorKeyMode && keyCode.bControl))
        {
            pOutput[iOutputLength++] = 'O';
        }
        else
        {
            pOutput[iOutputLength++] = '[';
        }

        switch (keyCode.VirtualKeyCode)
        {
        case VK_UP:
            pOutput[iOutputLength++] = 'A';
            break;

        case VK_DOWN:
            pOutput[iOutputLength++] = 'B';
            break;

        case VK_RIGHT:
            pOutput[iOutputLength++] = 'C';
            break;

        case VK_LEFT:
            pOutput[iOutputLength++] = 'D';
            break;
        }
    }
    else
    if (keyCode.VirtualKeyCode == VK_HOME ||
        keyCode.VirtualKeyCode == VK_INSERT ||
        keyCode.VirtualKeyCode == VK_DELETE ||
        keyCode.VirtualKeyCode == VK_END ||
        keyCode.VirtualKeyCode == VK_PRIOR ||
        keyCode.VirtualKeyCode == VK_NEXT)
    {
        pOutput[iOutputLength++] = CH_ESC;
        pOutput[iOutputLength++] = '[';

        switch (keyCode.VirtualKeyCode)
        {
        case VK_HOME:
            pOutput[iOutputLength++] = '1';
            break;

        case VK_INSERT:
            pOutput[iOutputLength++] = '2';
            break;

        case VK_DELETE:
            pOutput[iOutputLength++] = '3';
            break;

        case VK_END:
            pOutput[iOutputLength++] = '4';
            break;

        case VK_PRIOR:
            pOutput[iOutputLength++] = '5';
            break;

        case VK_NEXT:
            pOutput[iOutputLength++] = '6';
            break;
        }

        pOutput[iOutputLength++] = '~';
    }
    else
    if (keyCode.VirtualKeyCode == VK_RETURN)
    {
        pOutput[iOutputLength++] = CH_CR;
        if (fCRLF == 1)
        {
            pOutput[iOutputLength++] = CH_LF;
        } 
    }
    else
    if (keyCode.AsciiChar != '\0')
    {
        pOutput[iOutputLength++] = keyCode.AsciiChar;
    }

    return iOutputLength;
}

bool
CAnsiTerm::ProcessOutput(const unsigned char *szData, int iLength)
{
    const unsigned char *pEnd = &szData[iLength];

    for (const unsigned char *pCurrent = szData; pCurrent < pEnd; pCurrent++)
    {
        if (*pCurrent < 0x20 || *pCurrent == 0x7F)
        {
	    dbglog("ProcessOutput: control_ch = %02x\n",*pCurrent); 
            OutputText();

            switch (*pCurrent)
            {
            case CH_NUL:
            case CH_ENQ:
            case CH_DEL:
                // These are ignored
                break;

            case CH_BEL:
                MessageBeep(MB_ICONASTERISK);
                break;

            case CH_BS:
                ProcessBackspace();
                break;

            case CH_HT:
                ProcessTab();
                break;

            case CH_LF:
            case CH_VT:
            case CH_FF:
                ProcessLinefeed(m_bLineFeedNewLineMode);
                break;

            case CH_CR:
                ProcessReturn();
                break;

            case CH_SO:
                m_SelectedCharset = CharsetG1;
                break;

            case CH_SI:
                m_SelectedCharset = CharsetG0;
                break;

            case CH_XON:
                // Not yet implemented
                break;

            case CH_XOF:
                // Not yet implemented
                break;

#if 0
            case CH_CAN:
            case CH_SUB:
                // Output error character
                break;
#endif

            case CH_ESC:
                m_State = DS_Escape;
                break;

            default:
                AddOutputData(s_OemToUnicode[*pCurrent]);
                break;
            }
        }
        else
        {
	    /* db b3 or b0 b3 */
	    if (*pCurrent & 0x80) 
	       dbglog("ProcessOutput: state=%d ch=%02x\n",m_State,*pCurrent); 
            switch (m_State)
            {
            case DS_Normal:
                if (*pCurrent & 0x80)
                {
                    // Could be start of a UTF-8 sequence or an ANSI extended character

                    if ((*pCurrent & 0xE0) == 0xC0)
                    {
                        m_UTF8Size = 2;
                    }
                    else if ((*pCurrent & 0xF0) == 0xE0)
                    {
                        m_UTF8Size = 3;
                    }
                    else 
                    {
                        // Not a UTF-8 lead character
                        AddOutputData(s_OemToUnicode[*pCurrent]);
                        break;
                    }
                    m_UTF8Count = 1;
                    m_UTF8Buffer[0] = *pCurrent;
                    m_State = DS_UTF8;
                    break;
                }

                if ((m_SelectedCharset == CharsetG0 && m_G0Charset == SpecialGraphicsCharset) ||
                    (m_SelectedCharset == CharsetG1 && m_G1Charset == SpecialGraphicsCharset))
                {
                    if (kMinGraphicsChar <= *pCurrent && *pCurrent <= kMaxGraphicsChar)
                    {
                        AddOutputData(s_GraphicChars[*pCurrent - kMinGraphicsChar]);
                    }
                    else
                    {
                        AddOutputData(*pCurrent);
                    }
                }
                else
                {
                    AddOutputData(*pCurrent);
                }
                break;

            case DS_UTF8:
                if ((*pCurrent & 0xC0) != 0x80)
                {
                    for (int index = 0; index < m_UTF8Count; index++)
                    {
                        AddOutputData(s_OemToUnicode[m_UTF8Buffer[index]]);
                    }

                    if (*pCurrent & 0x80)
                    {
                        AddOutputData(s_OemToUnicode[*pCurrent]);
                    }
                    else
                    {
                        AddOutputData(*pCurrent);
                    }
                    m_State = DS_Normal;
                }
                else
                {
                    m_UTF8Buffer[m_UTF8Count++] = *pCurrent;

                    if (m_UTF8Count == m_UTF8Size)
                    {
                        wchar_t wchUTF16;
                        if (m_UTF8Size == 2)
                        {
                            wchUTF16 = ((m_UTF8Buffer[0] & 0x1F) << 6) | (m_UTF8Buffer[1] & 0x3F);
                        }
                        else
                        {
                            wchUTF16 = ((m_UTF8Buffer[0] & 0x0F) << 12) | ((m_UTF8Buffer[1] & 0x3F) << 6) | (m_UTF8Buffer[2] & 0x3F);
                        }

                        AddOutputData(wchUTF16);
                        m_State = DS_Normal;
                    }
                }
                break;

            case DS_Escape:
                for (int index = 0; s_CSITable[index].chCode != '\0'; index++)
                {
                    if (*pCurrent == s_CSITable[index].chCode)
                    {
                        if (s_CSITable[index].dsNextState != DS_None)
                        {
                            m_State = s_CSITable[index].dsNextState;
                            m_Parameters[0] = 0;
                            m_ParameterCount = 0;
                            m_bParametersStart = true;
                        }
                        else
                        {
                            (this->*s_CSITable[index].pfnProcess)();
                            m_State = DS_Normal;
                        }
                        break;
                    }
                }

                if (m_State == DS_Escape)
                {
                    AddOutputData(L'^');
                    AddOutputData(L'[');
                    AddOutputData(*pCurrent);
                    m_State = DS_Normal;
                }
                break;

            case DS_CSIParam:
                if (m_bParametersStart)
                {
                    m_bParametersStart = false;

                    if (*pCurrent == '?')
                    {
                        m_bPrivateParameters = true;
                        break;
                    }
                    else
                    {
                        m_bPrivateParameters = false;
                    }
                }
                if ('0' <= *pCurrent && *pCurrent <= '9')
                {
                    if (m_ParameterCount < kMaxParameterCount)
                    {
                        m_Parameters[m_ParameterCount] *= 10;
                        m_Parameters[m_ParameterCount] += *pCurrent - '0';
                    }
                }
                else if (*pCurrent == ';')
                {
                    if (m_ParameterCount < kMaxParameterCount)
                    {
                        m_ParameterCount++;
                    }

                    if (m_ParameterCount < kMaxParameterCount)
                    {
                        m_Parameters[m_ParameterCount] = 0;
                    }
                }
                else 
                {
                    if (m_ParameterCount < kMaxParameterCount)
                    {
                        m_ParameterCount++;
                    }

                    for (int index = 0; s_CSIFunction[index].chCode != '\0'; index++)
                    {
                        if (*pCurrent == s_CSIFunction[index].chCode)
                        {
                            (this->*s_CSIFunction[index].pfnProcess)();
                            m_State = DS_Normal;
                            break;
                        }
                    }
                    if (m_State != DS_Normal)
                    {
                        DisplayCSI(*pCurrent);
                        m_State = DS_Normal;
                    }
                }
                break;

            case DS_DECPrivate:
                for (int index = 0; s_DECFunction[index].chCode != '\0'; index++)
                {
                    if (*pCurrent == s_DECFunction[index].chCode)
                    {
                        (this->*s_DECFunction[index].pfnProcess)();
                        m_State = DS_Normal;
                        break;
                    }
                }

                if (m_State != DS_Normal)
                {
                    AddOutputData(L'^');
                    AddOutputData(L'[');
                    AddOutputData(L'#');
                    AddOutputData(*pCurrent);
                }
                break;

            case DS_SelectG0:
                ProcessSCSG0(*pCurrent);
                m_State = DS_Normal;
                break;

            case DS_SelectG1:
                ProcessSCSG1(*pCurrent);
                m_State = DS_Normal;
                break;

            default:
		dbglog("ProcessOutput: illegal m_State=%d\n",m_State);
                assert(false);
                break;
            }
        }
    }

    OutputText();

    return true;
}

void
CAnsiTerm::DisplayCSI(char ch)
{
    char    szParam[15];

    AddOutputData(L'^');
    AddOutputData(L'[');
    AddOutputData(L'[');
    for (int idxParam = 0; idxParam < m_ParameterCount; idxParam++)
    {
        if (idxParam > 0)
        {
            AddOutputData(L';');
        }

        int     iLenParam = sprintf(szParam, "%d", m_Parameters[idxParam]);

        for (int idxChar = 0; idxChar < iLenParam; idxChar++)
        {
            AddOutputData(szParam[idxChar]);
        }
    }
    AddOutputData(ch);
}

bool
CAnsiTerm::ResetTerm(void)
{
    m_State = DS_Normal;

    m_SelectedCharset = CharsetG0;
    m_G0Charset = AsciiCharset;
    m_G1Charset = SpecialGraphicsCharset;

    m_Cursor.X = 0;
    m_Cursor.Y = 0;

    m_SavedCursor = m_Cursor;

    m_sTopMargin = 0;
    m_sBottomMargin = m_WindowSize.Y - 1;

    m_Attribute = kDefaultAttribute;

    UpdateTextAttribute();

    m_dwOutputCount = 0;

    // if (fCRLF == 1) 
    m_bLineFeedNewLineMode = true;  /*was false*/
    m_bCursorKeyMode = false;
    m_bAnsiMode = true;
    m_bColumnMode = true;
    m_bScrollingMode = false;
    m_bScreenMode = false;
    m_bOriginMode = false;
    m_bAutoRepeatingMode = true;
    m_bInterlaceMode = false;
    m_bDisplayCursor = true;
    m_bAutoWrapMode = true;  /*default to wrap*/

    EraseDisplay(EraseAll);

    SetCursorPosition();

    DisplayCursor();

    return true;
}

bool
CAnsiTerm::DisplayCursor(void)
{
    CONSOLE_CURSOR_INFO     cursorInfo = { 100, m_bDisplayCursor };

    return SetConsoleCursorInfo(m_hConsole, &cursorInfo) != FALSE;
}

bool
CAnsiTerm::UpdateTextAttribute(void)
{
    return SetConsoleTextAttribute(m_hConsole, m_Attribute) != FALSE;
}

bool
CAnsiTerm::GetCursorPosition(void)
{
    CONSOLE_SCREEN_BUFFER_INFO  bufferInfo;

    if (GetConsoleScreenBufferInfo(m_hConsole, &bufferInfo))
    {
        m_Cursor = bufferInfo.dwCursorPosition;
        m_Cursor.Y -= m_WindowOrigin.Y + (m_bOriginMode ? m_sTopMargin : 0);

        return true;
    }

    return false;
}

bool
CAnsiTerm::SetCursorPosition(void)
{
    COORD   cursor = m_Cursor;

    cursor.Y += m_WindowOrigin.Y + (m_bOriginMode ? m_sTopMargin : 0);

    return SetConsoleCursorPosition(m_hConsole, cursor) != FALSE;
}

bool
CAnsiTerm::ScrollDisplay(int n, bool bWindowOnly)
{
    SHORT nLines = (SHORT)n;
    if (nLines == 0)
    {
        return true;
    }

    SMALL_RECT  rectSource = { 0, 0, m_BufferSize.X - 1, m_BufferSize.Y - 1 };
    COORD       coordDest = { 0, 0 };
    CHAR_INFO   charInfo = { ' ', m_Attribute };

    if (nLines > 0)
    {
        if (bWindowOnly || m_sTopMargin > 0)
        {
            coordDest.Y = m_WindowOrigin.Y + m_sTopMargin;
            rectSource.Top = m_WindowOrigin.Y + m_sTopMargin + nLines;
            rectSource.Bottom = m_WindowOrigin.Y + m_sBottomMargin;
        }
        else
        {
            rectSource.Top = nLines;
            rectSource.Bottom = m_WindowOrigin.Y + m_sBottomMargin;
        }
    }
    else
    {
        if (bWindowOnly)
        {
            coordDest.Y = m_WindowOrigin.Y + m_sTopMargin - nLines;
            rectSource.Top = m_WindowOrigin.Y + m_sTopMargin;
            rectSource.Bottom = m_WindowOrigin.Y + m_sBottomMargin + 1 + nLines;
        }
        else
        {
            coordDest.Y -= nLines;
        }

        rectSource.Bottom += nLines;
    }

    return ScrollConsoleScreenBuffer(m_hConsole, &rectSource, NULL, coordDest, &charInfo) != FALSE;
}

bool
CAnsiTerm::EraseLine(EraseType eType)
{
    DWORD   dwLength;

    COORD   coordStart = m_WindowOrigin;

    coordStart.Y += m_Cursor.Y + (m_bOriginMode ? m_sTopMargin : 0);

    switch (eType)
    {
    case EraseCursorToEnd:
        if (m_Cursor.X < m_WindowSize.X)
        {
            coordStart.X += m_Cursor.X;
            dwLength = m_BufferSize.X - m_Cursor.X;
        }
        else
        {
            dwLength = 0;
        }
        break;

    case EraseBeginningToCursor:
        if (m_Cursor.X < m_WindowSize.X)
        {
            dwLength = m_Cursor.X + 1;
        }
        else
        {
            dwLength = m_BufferSize.X;
        }
        break;

    case EraseAll:
        dwLength = m_BufferSize.X;
        break;

    default:
        return false;
    }

    if (dwLength > 0)
    {
        DWORD   dwWritten;

        FillConsoleOutputAttribute(m_hConsole, m_Attribute, dwLength, coordStart, &dwWritten);
        return FillConsoleOutputCharacter(m_hConsole, ' ', dwLength, coordStart, &dwWritten) != FALSE;
    }
    else
    {
        return true;
    }
}

bool
CAnsiTerm::EraseDisplay(EraseType eType)
{
    COORD   coordStart = m_WindowOrigin;
    DWORD   dwLength;

    switch (eType)
    {
    case EraseCursorToEnd:
        if (m_Cursor.X < m_WindowSize.X)
        {
            coordStart.X += m_Cursor.X;
            coordStart.Y += m_Cursor.Y + (m_bOriginMode ? m_sTopMargin : 0);
            dwLength = (m_WindowSize.Y - m_Cursor.Y - (m_bOriginMode ? m_sTopMargin : 0)) * m_BufferSize.X - m_Cursor.X;
        }
        else if (m_Cursor.Y < (m_WindowSize.Y - 1))
        {
            coordStart.X = 0;
            coordStart.Y += m_Cursor.Y + 1;
            dwLength = (m_WindowSize.Y - m_Cursor.Y - (m_bOriginMode ? m_sTopMargin : 0) - 1) * m_BufferSize.X;
        }
        else
        {
            dwLength = 0;
        }
        break;

    case EraseBeginningToCursor:
        if (m_Cursor.X < m_WindowSize.X)
        {
            dwLength = (m_Cursor.Y + (m_bOriginMode ? m_sTopMargin : 0)) * m_BufferSize.X + m_Cursor.X;
        }
        else
        {
            dwLength = (m_Cursor.Y + (m_bOriginMode ? m_sTopMargin : 0) + 1) * m_BufferSize.X;
        }
        break;

    case EraseAll:
        dwLength = m_BufferSize.X * m_WindowSize.Y;
        break;

    default:
        return false;
    }

    if (dwLength > 0)
    {
        DWORD   dwWritten;

        FillConsoleOutputAttribute(m_hConsole, m_Attribute, dwLength, coordStart, &dwWritten);
        return FillConsoleOutputCharacter(m_hConsole, ' ', dwLength, coordStart, &dwWritten) != FALSE;
    }
    else
    {
        return true;
    }
}

DWORD
CAnsiTerm::OutputText(void)
{
    if (m_dwOutputCount == 0)
    {
        return 0;
    }

    DWORD       dwTotalWritten = 0;
    wchar_t *   pwszCurrent = m_OutputBuffer;
    DWORD       dwLeftToWrite = m_dwOutputCount;

    while (dwLeftToWrite > 0)
    {
        DWORD       dwWritten;

        if (m_Cursor.X >= m_WindowSize.X)
        {
            if (m_bAutoWrapMode)
            {
                ProcessLinefeed(true);
            }
            else
            {
                m_Cursor.X = m_WindowSize.X - 1;

                SetCursorPosition();

                if (WriteConsoleW(m_hConsole, &m_OutputBuffer[m_dwOutputCount - 1], 1, &dwWritten, NULL))
                {
		    dbglog("OutputTest: WriteConsoleW error1\n");
                    assert(dwWritten == 1);
                }

                m_Cursor.X++;
                dwTotalWritten += dwLeftToWrite;
                break;
            }
        }

        DWORD       dwPartialCount = min(dwLeftToWrite, (DWORD)(m_WindowSize.X - m_Cursor.X));

        if (WriteConsoleW(m_hConsole, pwszCurrent, dwPartialCount, &dwWritten, NULL))
        {
	    dbglog("OutputTest: WriteConsoleW error2\n");
            assert(dwWritten == dwPartialCount);
        }
        else
        {
            DWORD dwError = GetLastError();
        }

        m_Cursor.X += (SHORT)dwPartialCount;
        pwszCurrent += dwPartialCount;
        dwTotalWritten += dwPartialCount;
        dwLeftToWrite -= dwPartialCount;
    }

    m_dwOutputCount = 0;

    return dwTotalWritten;
}

bool
CAnsiTerm::ProcessBackspace(void)
{
    if (m_Cursor.X > 0)
    {
        if (m_Cursor.X < m_WindowSize.X)
        {
            m_Cursor.X--;
        }
        else
        {
            m_Cursor.X = m_WindowSize.X - 1;
        }

        return SetCursorPosition();
    }
    return true;
}

bool
CAnsiTerm::ProcessTab(void)
{
    if (m_Cursor.X >= m_WindowSize.X)
    {
        if (m_bAutoWrapMode)
        {
            ProcessLinefeed(true);
        }
        else
        {
            return true;
        }
    }

    int newX = m_Cursor.X + 8;

    newX &= ~7;

    if (newX >= m_WindowSize.X)
    {
        newX = m_WindowSize.X - 1;
    }

    int cntSpaces = newX - m_Cursor.X;

    for (int index = 0; index < cntSpaces; index++)
    {
        AddOutputData(L' ');
    }

    return true;
}

bool
CAnsiTerm::ProcessReverseLinefeed(void)
{
    if (m_Cursor.Y == 0)
    {
        ScrollDisplay(-1, true);
    }
    else
    {
        m_Cursor.Y--;
    }

    return SetCursorPosition();
}

bool
CAnsiTerm::ProcessLinefeed(bool bNewLine)
{
    if (bNewLine)
    {
        m_Cursor.X = 0;
    }

    if (m_bOriginMode || (m_Cursor.Y >= m_sTopMargin && m_Cursor.Y <= m_sBottomMargin))
    {
        if (m_Cursor.Y >= (m_sBottomMargin - m_sTopMargin))
        {
            ScrollDisplay(1, false);
            m_Cursor.Y = m_sBottomMargin;
        }
        else
        {
            m_Cursor.Y++;
        }
    }
    else if (m_Cursor.Y < m_sBottomMargin)
    {
        m_Cursor.Y++;
    }

    return SetCursorPosition();
}

bool
CAnsiTerm::ProcessReturn(void)
{
    m_Cursor.X = 0;

    return SetCursorPosition();
}

bool
CAnsiTerm::ProcessDECALN(void)
{
    // Fill the display with 'E' for adjusting the CRT on a VT100 - Ignore it
    return true;
}

bool
CAnsiTerm::ProcessDECDHLB(void)
{
    // Double Height Line Bottom - Not supported
    return true;
}

bool
CAnsiTerm::ProcessDECDHLT(void)
{
    // Double Height Line Top - Not supported
    return true;
}

bool
CAnsiTerm::ProcessDECDWL(void)
{
    // Double Width Line - Not supported
    return true;
}

bool
CAnsiTerm::ProcessDECID(void)
{
    return ProcessDA();
}

bool
CAnsiTerm::ProcessDECKPAM(void)
{
    // Keypad Application Mode - Not yet implemented
    return true;
}

bool
CAnsiTerm::ProcessDECKPNM(void)
{
    // Keypad Numeric Mode - Not yet implemented
    return true;
}

bool
CAnsiTerm::ProcessDECLL(void)
{
    // Load LEDs - Not Supported
    return true;
}

bool
CAnsiTerm::ProcessDECRC(void)
{
    return ProcessRCP();
}

bool
CAnsiTerm::ProcessDECREQTPARM(void)
{
    // Request Terminal Parameters (Baud Rate, Parity, etc) - Not supported
    return true;
}

bool
CAnsiTerm::ProcessDECSC(void)
{
    return ProcessSCP();
}

bool
CAnsiTerm::ProcessDECSTBM(void)
{
    assert(m_ParameterCount >= 1);

    if (m_Parameters[0] > 0)
    {
        m_Parameters[0]--;
    }

    if (m_ParameterCount < 2)
    {
        m_Parameters[1] = m_WindowSize.Y - 1;
    }
    else
    {
        if (m_Parameters[1] > 0)
        {
            m_Parameters[1]--;
        }
        else
        {
            m_Parameters[1] = m_WindowSize.Y - 1;
        }
    }

    if (m_Parameters[0] >= m_WindowSize.Y ||
        m_Parameters[1] >= m_WindowSize.Y ||
        m_Parameters[0] >= m_Parameters[1])
    {
        return false;
    }

    m_sTopMargin = (SHORT)m_Parameters[0];
    m_sBottomMargin = (SHORT)m_Parameters[1];

    m_Cursor.X = 0;
    m_Cursor.Y = 0;

    SetCursorPosition();

    return true;
}

bool
CAnsiTerm::ProcessDECSWL(void)
{
    // Single Width Line - Since Double Width Line isn't supported, this isn't necessary
    return true;
}

bool
CAnsiTerm::ProcessDECTST(void)
{
    // Perform Self Test - Not supported
    return true;
}

bool
CAnsiTerm::ProcessCUB(void)
{
    if (m_Parameters[0] == 0)
    {
        m_Parameters[0]++;
    }

    m_Cursor.X -= min(m_Cursor.X, m_Parameters[0]);

    return SetCursorPosition();
}

bool
CAnsiTerm::ProcessCUD(void)
{
    if (m_Parameters[0] == 0)
    {
        m_Parameters[0]++;
    }

    m_Cursor.Y += min((m_bOriginMode ? m_sBottomMargin : m_WindowSize.Y - 1) - m_Cursor.Y, m_Parameters[0]);

    return SetCursorPosition();
}

bool
CAnsiTerm::ProcessCUF(void)
{
    if (m_Parameters[0] == 0)
    {
        m_Parameters[0]++;
    }

    m_Cursor.X += min(m_WindowSize.X - m_Cursor.X - 1, m_Parameters[0]);

    return SetCursorPosition();
}

bool
CAnsiTerm::ProcessCUP(void)
{
    return ProcessHVP();
}

bool
CAnsiTerm::ProcessCUU(void)
{
    if (m_Parameters[0] == 0)
    {
        m_Parameters[0]++;
    }

    m_Cursor.Y -= min(m_Cursor.Y, m_Parameters[0]);

    return SetCursorPosition();
}

bool
CAnsiTerm::ProcessDA(void)
{
    // Send Device Attributes - Not supported
    return true;
}

bool
CAnsiTerm::ProcessDSR(void)
{
    // Send Device Status Request - Not supported
    return true;
}

bool
CAnsiTerm::ProcessED(void)
{
    return EraseDisplay((EraseType)m_Parameters[0]);
}

bool
CAnsiTerm::ProcessEL(void)
{
    return EraseLine((EraseType)m_Parameters[0]);
}

bool
CAnsiTerm::ProcessHTS(void)
{
    // Soft Tab Set - Not implemented yet
    return true;
}

bool
CAnsiTerm::ProcessHVP(void)
{
    assert(m_ParameterCount >= 1);

    if (m_Parameters[0] > 0)
    {
        m_Parameters[0]--;
    }

    if (m_ParameterCount < 2)
    {
        m_Parameters[1] = 0;
    }
    else
    {
        if (m_Parameters[1] > 0)
        {
            m_Parameters[1]--;
        }
    }

    if (m_bOriginMode)
    {
        if (m_Parameters[0] >= (m_sBottomMargin - m_sTopMargin + 1))
        {
            m_Parameters[0] = m_sBottomMargin - m_sTopMargin;
        }
    }
    else
    {
        if (m_Parameters[0] >= m_WindowSize.Y)
        {
            m_Parameters[0] = m_WindowSize.Y - 1;
        }
    }

    if (m_Parameters[1] >= m_WindowSize.X)
    {
        m_Parameters[1] = m_WindowSize.X - 1;
    }

    m_Cursor.Y = (SHORT)m_Parameters[0];
    m_Cursor.X = (SHORT)m_Parameters[1];

    return SetCursorPosition();
}

bool
CAnsiTerm::ProcessIND(void)
{
    return ProcessLinefeed(false);
}

bool
CAnsiTerm::ProcessNEL(void)
{
    return ProcessLinefeed(true);
}

bool
CAnsiTerm::ProcessRCP(void)
{
    m_Cursor = m_SavedCursor;

    return SetCursorPosition();
}

bool
CAnsiTerm::ProcessRI(void)
{
    return ProcessReverseLinefeed();
}

bool
CAnsiTerm::ProcessRIS(void)
{
    return ResetTerm();
}

bool
CAnsiTerm::ProcessRM(void)
{
    bool bret = true;

    if (m_bPrivateParameters)
    {
        for (int index = 0; index < m_ParameterCount; index++)
        {
            switch (m_Parameters[index])
            {
            case 0:
            default:
		dbglog("ProcessRM: illegal private param %d\n",m_Parameters[index]);
		bret = false;
                break;
            case DECCKM:
                m_bCursorKeyMode = false;
                break;
            case DECANM:
                m_bAnsiMode = false;
                break;
            case DECCOLM:
                m_bColumnMode = false;
                break;
            case DECSCLM:
                m_bScrollingMode = false;
                break;
            case DECSCNM:
                m_bScreenMode = false;
                break;
            case DECOM:
                m_bOriginMode = false;
                m_Cursor.X = 0;
                m_Cursor.Y = 0;
                SetCursorPosition();
                break;
            case DECAWM:
                m_bAutoWrapMode = false;
                break;
            case DECARM:
                m_bAutoRepeatingMode = false;
                break;
            case DECINLM:
                m_bInterlaceMode = false;
                break;
            case DECTCEM:
                m_bDisplayCursor = false;
                DisplayCursor();
                break;
            }
        }
    }
    else
    {
        for (int index = 0; index < m_ParameterCount; index++)
        {
            switch (m_Parameters[index])
            {
            case 20: m_bLineFeedNewLineMode = false;    break;  // LNM
            default: 
		dbglog("ProcessRM: illegal public param %d\n",m_Parameters[index]);
		bret = false; break;
            }
        }
    }
    return bret;
}

bool
CAnsiTerm::ProcessSCP(void)
{
    m_SavedCursor = m_Cursor;
    return true;
}

bool
CAnsiTerm::ProcessSCSG0(char ch)
{
    switch (ch)
    {
    case 'B':   // ASCII Charset
        m_G0Charset = AsciiCharset;
        break;

    case '0':   // Special Graphics Charset
        m_G0Charset = SpecialGraphicsCharset;
        break;

    case 'A':   // UK Charset
    case '1':   // Alternate Character ROM Standard Charset
    case '2':   // Alternate Character ROM Special Graphics Charset
    default:    // Unsupported
        return false;

    }
    return true;
}

bool
CAnsiTerm::ProcessSCSG1(char ch)
{
    switch (ch)
    {
    case 'B':   // ASCII Charset
        m_G1Charset = AsciiCharset;
        break;

    case '0':   // Special Graphics Charset
        m_G1Charset = SpecialGraphicsCharset;
        break;

    case 'A':   // UK Charset
    case '1':   // Alternate Character ROM Standard Charset
    case '2':   // Alternate Character ROM Special Graphics Charset
    default:    // Unsupported
        return false;

    }
    return true;
}

bool
CAnsiTerm::ProcessSGR(void)
{
    for (int index = 0; index < m_ParameterCount; index++)
    {
        switch (m_Parameters[index])
        {
        case 0:
            m_Attribute = kDefaultAttribute;
            break;

        case 1:
            m_Attribute |= FOREGROUND_INTENSITY;
            break;

        case 4:
            m_Attribute |= COMMON_LVB_UNDERSCORE;
            break;

        case 5:
            // Blinking isn't supported
            break;

        case 7:
            m_Attribute |= COMMON_LVB_REVERSE_VIDEO;
            break;

        case 22:
            m_Attribute &= ~FOREGROUND_INTENSITY;
            break;

        case 24:
            m_Attribute &= ~COMMON_LVB_UNDERSCORE;
            break;

        case 25:
            // Blinking isn't supported
            break;

        case 27:
            m_Attribute &= ~COMMON_LVB_REVERSE_VIDEO;
            break;

        case 30: // Black text
            m_Attribute &= ~(FOREGROUND_INTENSITY | COMMON_LVB_UNDERSCORE |
                             FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            break;

        case 31: // Red text
            m_Attribute &= ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            m_Attribute |= FOREGROUND_RED;
            break;

        case 32: // Green text
            m_Attribute &= ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            m_Attribute |= FOREGROUND_GREEN;
            break;

        case 33: // Yellow text
            m_Attribute &= ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            m_Attribute |= FOREGROUND_RED | FOREGROUND_GREEN;
            break;

        case 34: // Blue text
            m_Attribute &= ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            m_Attribute |= FOREGROUND_BLUE;
            break;

        case 35: // Magenta text
            m_Attribute &= ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            m_Attribute |= FOREGROUND_RED | FOREGROUND_BLUE;
            break;

        case 36: // Cyan text
            m_Attribute &= ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            m_Attribute |= FOREGROUND_GREEN | FOREGROUND_BLUE;
            break;

        case 37: // White text
            m_Attribute |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            break;

        case 40: // Black background
            m_Attribute &= ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
            break;

        case 41: // Red background
            m_Attribute &= ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
            m_Attribute |= BACKGROUND_RED;
            break;

        case 42: // Green background
            m_Attribute &= ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
            m_Attribute |= BACKGROUND_GREEN;
            break;

        case 43: // Yellow background
            m_Attribute &= ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
            m_Attribute |= BACKGROUND_RED | BACKGROUND_GREEN;
            break;

        case 44: // Blue background
            m_Attribute &= ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
            m_Attribute |= BACKGROUND_BLUE;
            break;

        case 45: // Magenta background
            m_Attribute &= ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
            m_Attribute |= BACKGROUND_RED | BACKGROUND_BLUE;
            break;

        case 46: // Cyan background
            m_Attribute &= ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
            m_Attribute |= BACKGROUND_GREEN | BACKGROUND_BLUE;
            break;

        case 47: // White background
            m_Attribute |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
            break;

        default:
	    	dbglog("ERROR: bad SGR param %d (0x%02x)\n",index,index);
            m_Attribute = kDefaultAttribute;
            break;
        }
    }

    UpdateTextAttribute();

    return true;
}

bool
CAnsiTerm::ProcessSM(void)
{
		
    dbglog("ProcessSM: start, priv=%d param0=%d cnt=%d\n", 
			m_bPrivateParameters,m_Parameters[0],m_ParameterCount);
    if (m_bPrivateParameters)
    {
        for (int index = 0; index < m_ParameterCount; index++)
        {
            switch (m_Parameters[0])
            {
            case 0:
            default:
	    		dbglog("ERROR: bad private SM param %d\n",m_Parameters[0]);
                assert(false);
                break;
            case DECCKM:
                m_bCursorKeyMode = true;
                break;
            case DECANM:
                m_bAnsiMode = true;
                break;
            case DECCOLM:
                m_bColumnMode = true;
                break;
            case DECSCLM:
                m_bScrollingMode = true;
                break;
            case DECSCNM:
                m_bScreenMode = true;
                break;
            case DECOM:
                m_bOriginMode = true;
                m_Cursor.X = 0;
                m_Cursor.Y = 0;
                m_SavedCursor = m_Cursor;
                SetCursorPosition();
                break;
            case DECAWM:
                m_bAutoWrapMode = true;
                break;
            case DECARM:
                m_bAutoRepeatingMode = true;
                break;
            case DECINLM:
                m_bInterlaceMode = true;
                break;
            case DECTCEM:
                m_bDisplayCursor = true;
                DisplayCursor();
                break;
            }
        }
    }
    else
    {
        for (int index = 0; index < m_ParameterCount; index++)
        {
            switch (m_Parameters[0])
            {
            case 20: m_bLineFeedNewLineMode = true; break;  // LNM
            default: 
				dbglog("ProcessSM: param %d != 20\n",m_Parameters[0]);
				assert(false);                 
				break;
            }
        }
    }
    return true;
}

bool
CAnsiTerm::ProcessTBC(void)
{
    // Soft Tab Clear - Not implemented yet
    return true;
}

static CAnsiTerm *g_pDisplay;

typedef unsigned char uchar;

extern "C"
{

void console_open(char fdebugcmd)
{
    g_pDisplay = new CAnsiTerm();
}

void console_close(void)
{
    delete g_pDisplay;
    g_pDisplay = NULL;
}

int console_in(DWORD keydata, uchar *pdata, int len)
{
    if (keydata == ~0)
    {
        g_pDisplay->WindowSizeChanged(false);
        return 0;
    }

    return g_pDisplay->ProcessInput(*(CAnsiTerm::KeyCode *)&keydata, pdata, len);
}

void console_out(uchar *pdata, int len)
{
    if (len > 0)
    {
        g_pDisplay->ProcessOutput(pdata, len);
    }
}

}
