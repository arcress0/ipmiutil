/*
 * AnsiTerm.h
 *      Windows ANSI Terminal Emulation
 *
 * Author:  Robert Nelson robertnelson at users.sourceforge.net
 * Copyright (c) 2009 Robert Nelson
 *
 * 10/07/09 Robert Nelson - Created
 * See ChangeLog for further changes
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

#pragma once

class CAnsiTerm
{
public:
    struct KeyCode
    {
        unsigned char   bKeyDown:1;
        unsigned char   bEnhanced:1;
        unsigned char   bCapsLock:1;
        unsigned char   bScrollLock:1;
        unsigned char   bNumLock:1;
        unsigned char   bShift:1;
        unsigned char   bControl:1;
        unsigned char   bAlt:1;
        unsigned char   RepeatCount;
        unsigned char   VirtualKeyCode;
        unsigned char   AsciiChar;
    };

private:
    enum ControlCharacters
    {
        CH_NUL = 0x00,  // Ignored on input
        CH_ENQ = 0x05,  // Transmit answerback
        CH_BEL = 0x07,  // Ring Bell
        CH_BS  = 0x08,  // Move left one character, no effect in column 0
        CH_HT  = 0x09,  // Tab 
        CH_LF  = 0x0A,  // Line feed or new line depending on line mode
        CH_VT  = 0x0B,  // Vertical tab, same as line feed
        CH_FF  = 0x0C,  // Form feed, same as line feed
        CH_CR  = 0x0D,  // Carriage return, return to column 0
        CH_SO  = 0x0E,  // Shift Out, Invoke G1 charset
        CH_SI  = 0x0F,  // Shift In, Invoke G0 charset
        CH_XON = 0x11,  // Resume transmission
        CH_XOF = 0x13,  // Suspend transmission
        CH_CAN = 0x18,  // Cancel, aborts the current control sequence
        CH_SUB = 0x1A,  // Same as Cancel
        CH_ESC = 0x1B,  // Escape, start of control sequence
        CH_DEL = 0x7F   // Delete, ignored
    };

    enum DECModes
    {
        DECCKM = 1,     // Cursor Key Mode (set = application codes)
        DECANM = 2,     // Ansi Mode (set = ansi)
        DECCOLM = 3,    // 80/132 Column Mode (set = 132)
        DECSCLM = 4,    // Scroll Mode (set = smooth)
        DECSCNM = 5,    // Screen Mode (set = black on white)
        DECOM = 6,      // Origin Mode (set = origin relative to top margin)
        DECAWM = 7,     // Autowrap Mode (set = wrap)
        DECARM = 8,     // Autorepeat Mode (set = repeat)
        DECINLM = 9,    // Interlace Mode (set = interlace)
        DECTCEM = 25    // Text Cursor Enable Mode

    };

    enum DisplayState
    {
        DS_None,
        DS_Normal,
        DS_UTF8,
        DS_Escape,
        DS_CSIParam,
        DS_DECPrivate,
        DS_SelectG0,
        DS_SelectG1
    };

    enum SelectedCharset
    {
        CharsetG0,
        CharsetG1
    };

    enum CharacterSet
    {
        AsciiCharset = 0,
        SpecialGraphicsCharset = 1,
        UKCharset = 2
    };

    enum EraseType
    {
        EraseCursorToEnd = 0,
        EraseBeginningToCursor = 1,
        EraseAll = 2
    };

    static const int    kMaxParameterCount = 16;

    static const int    kOutputBufferSize = 256;

    static const int    kMinGraphicsChar = 0x5F;
    static const int    kMaxGraphicsChar = 0x7E;

    static const int    kColorTableSize = 16;

    static const WORD   kDefaultAttribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

    struct CSICode
    {
        char            chCode;
        DisplayState    dsNextState;
        bool            (CAnsiTerm::*pfnProcess)(void);
    };

    struct CSIFunction
    {
        char            chCode;
        bool            (CAnsiTerm::*pfnProcess)(void);
    };

protected:
    typedef struct _CONSOLE_SCREEN_BUFFER_INFOEX {
	ULONG      cbSize;
	COORD      dwSize;
	COORD      dwCursorPosition;
	WORD       wAttributes;
	SMALL_RECT srWindow;
	COORD      dwMaximumWindowSize;
	WORD       wPopupAttributes;
	BOOL       bFullscreenSupported;
	COLORREF   ColorTable[16];
    } CONSOLE_SCREEN_BUFFER_INFOEX, *PCONSOLE_SCREEN_BUFFER_INFOEX;

    typedef BOOL (WINAPI *PFN_GetConsoleScreenBufferInfoEx)(
        HANDLE hConsoleOutput,
        PCONSOLE_SCREEN_BUFFER_INFOEX lpConsoleScreenBufferInfoEx);

    typedef BOOL (WINAPI *PFN_SetConsoleScreenBufferInfoEx)(
        HANDLE hConsoleOutput,
        PCONSOLE_SCREEN_BUFFER_INFOEX lpConsoleScreenBufferInfoEx);

    static PFN_GetConsoleScreenBufferInfoEx s_pfnGetConsoleScreenBufferInfoEx;
    static PFN_SetConsoleScreenBufferInfoEx s_pfnSetConsoleScreenBufferInfoEx;

private:
    static CSICode      s_CSITable[];
    static CSIFunction  s_DECFunction[];
    static CSIFunction  s_CSIFunction[];
    static wchar_t      s_GraphicChars[kMaxGraphicsChar - kMinGraphicsChar + 1];
    static wchar_t      s_OemToUnicode[256];
    static COLORREF     s_ColorTable[kColorTableSize];


public:
    CAnsiTerm(void);
    virtual ~CAnsiTerm(void);

    void WindowSizeChanged(bool bInitial);

    int  ProcessInput(KeyCode keyCode, unsigned char *pOutput, int iOutputLen);

    bool ProcessOutput(const unsigned char *szData, int iLength);

protected:
    virtual bool ResetTerm(void);
    virtual bool GetCursorPosition(void);
    virtual bool SetCursorPosition(void);
    virtual bool DisplayCursor(void);

    virtual bool ScrollDisplay(int nLines, bool bWindowOnly);
    virtual bool EraseLine(EraseType eType);
    virtual bool EraseDisplay(EraseType eType);
    virtual bool UpdateTextAttribute(void);

    virtual DWORD OutputText(void);

private:
    bool ProcessBackspace(void);
    bool ProcessTab(void);
    bool ProcessLinefeed(bool bNewLine);
    bool ProcessReverseLinefeed(void);
    bool ProcessReturn(void);

    void AddOutputData(wchar_t wchData)
    {
        m_OutputBuffer[m_dwOutputCount++] = wchData;

        if (m_dwOutputCount >= kOutputBufferSize)
        {
            OutputText();
        }
    }

    void DisplayCSI(char ch);

    bool ProcessSCSG0(char ch);
    bool ProcessSCSG1(char ch);

    bool ProcessDECALN(void);
    bool ProcessDECDHLB(void);
    bool ProcessDECDHLT(void);
    bool ProcessDECDWL(void);
    bool ProcessDECID(void);
    bool ProcessDECKPAM(void);
    bool ProcessDECKPNM(void);
    bool ProcessDECLL(void);
    bool ProcessDECRC(void);
    bool ProcessDECREQTPARM(void);
    bool ProcessDECSC(void);
    bool ProcessDECSTBM(void);
    bool ProcessDECSWL(void);
    bool ProcessDECTST(void);

    bool ProcessCUB(void);
    bool ProcessCUD(void);
    bool ProcessCUF(void);
    bool ProcessCUP(void);
    bool ProcessCUU(void);
    bool ProcessDA(void);
    bool ProcessDSR(void);
    bool ProcessED(void);
    bool ProcessEL(void);
    bool ProcessHTS(void);
    bool ProcessHVP(void);
    bool ProcessIND(void);
    bool ProcessNEL(void);
    bool ProcessRCP(void);
    bool ProcessRI(void);
    bool ProcessRIS(void);
    bool ProcessRM(void);
    bool ProcessSCP(void);
    bool ProcessSGR(void);
    bool ProcessSM(void);
    bool ProcessTBC(void);

private:
    HANDLE          m_hConsole;
    DWORD           m_dwOrigConsoleMode;
    WORD            m_wOrigConsoleAttribute;
    DWORD           m_dwOrigCursorSize;
    COLORREF        m_OrigColorTable[kColorTableSize];
    bool            m_bResetColorTable;
    DisplayState    m_State;

    SelectedCharset m_SelectedCharset;
    CharacterSet    m_G0Charset;
    CharacterSet    m_G1Charset;

    COORD           m_BufferSize;
    COORD           m_WindowSize;
    COORD           m_WindowOrigin;
    COORD           m_Cursor;
    COORD           m_SavedCursor;
    SHORT           m_sTopMargin;
    SHORT           m_sBottomMargin;
    WORD            m_Attribute;
    wchar_t         m_OutputBuffer[kOutputBufferSize];
    DWORD           m_dwOutputCount;

    unsigned char   m_UTF8Buffer[3];
    int             m_UTF8Count;
    int             m_UTF8Size;

    int             m_Parameters[kMaxParameterCount];
    int             m_ParameterCount;
    bool            m_bParametersStart;
    bool            m_bPrivateParameters;

    // Modes
    bool            m_bLineFeedNewLineMode; // LNM
    bool            m_bCursorKeyMode;       // DECCKM
// Always set - VT52 not supported
    bool            m_bAnsiMode;            // DECANM
// Not used - column width is based on width of console window
    bool            m_bColumnMode;          // DECCOLM
// Not used - always reset
    bool            m_bScrollingMode;       // DECSCLM
// Not implemented - always reset
    bool            m_bScreenMode;          // DECSCNM
    bool            m_bOriginMode;          // DECOM
    bool            m_bAutoWrapMode;        // DECAWM
// Not implemented - always set
    bool            m_bAutoRepeatingMode;   // DECARM
// Not implemented - always reset
    bool            m_bInterlaceMode;       // DECINLM
    bool            m_bDisplayCursor;       // DECTCEM
};
