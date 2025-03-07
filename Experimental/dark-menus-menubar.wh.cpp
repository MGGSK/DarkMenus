// ==WindhawkMod==
// @id              dark-menus
// @name            Dark mode context menus
// @description     Enables dark mode for all win32 menus.
// @version         1.3
// @author          Mgg Sk
// @github          https://github.com/MGGSK
// @include         *
// @compilerOptions -lUxTheme -lGdi32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Dark mode context menus
Forces dark mode for all win32 context menus to create a more consistent UI. Requires Windows 10 build 18362 or later.

### Before:
![Before](https://i.imgur.com/bGRVJz8.png)
![Before10](https://i.imgur.com/FGyph05.png)

### After:
![After](https://i.imgur.com/BURKEki.png)
![After10](https://i.imgur.com/MUqVAcG.png)
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- AppMode: ForceDark
  $name: When do you want context menus in dark mode?
  $description: Set to "Only when Windows is in dark mode" if you switch between light and dark mode a lot or are using a tool like Auto Dark Mode.
  $options:
  - ForceDark: Always
  - AllowDark: Only when Windows is in dark mode
  - ForceLight: Never
*/
// ==/WindhawkModSettings==

#include <uxtheme.h>
#include <vsstyle.h>
#include <windhawk_api.h>
#include <windhawk_utils.h>
#include <windows.h>

static HBRUSH brBarBackground = CreateSolidBrush(0x262626);
static HBRUSH brItemBackground = CreateSolidBrush(0x262626);

static COLORREF fgItemBackground = 0xFFFFFF;
static HBRUSH brItemBackgroundHot = CreateSolidBrush(0x353535);
static HBRUSH brItemBackgroundSelected = CreateSolidBrush(0x353535);
static HBRUSH brItemBorder = CreateSolidBrush(0x353535);

#pragma region UAHMenuBar

// window messages related to menu bar drawing
#define WM_UAHDRAWMENU         0x0091	// lParam is UAHMENU
#define WM_UAHDRAWMENUITEM     0x0092	// lParam is UAHDRAWMENUITEM

// describes the sizes of the menu bar or menu item
union UAHMENUITEMMETRICS
{
	// cx appears to be 14 / 0xE less than rcItem's width!
	// cy 0x14 seems stable, i wonder if it is 4 less than rcItem's height which is always 24 atm
	struct {
		DWORD cx;
		DWORD cy;
	} rgsizeBar[2];
	struct {
		DWORD cx;
		DWORD cy;
	} rgsizePopup[4];
};

// not really used in our case but part of the other structures
struct UAHMENUPOPUPMETRICS
{
	DWORD rgcx[4];
	DWORD fUpdateMaxWidths : 2; // from kernel symbols, padded to full dword
};

// hmenu is the main window menu; hdc is the context to draw in
struct UAHMENU
{
	HMENU hmenu;
	HDC hdc;
	DWORD dwFlags; // no idea what these mean, in my testing it's either 0x00000a00 or sometimes 0x00000a10
};

// menu items are always referred to by iPosition here
struct UAHMENUITEM
{
	int iPosition; // 0-based position of menu item in menubar
	UAHMENUITEMMETRICS umim;
	UAHMENUPOPUPMETRICS umpm;
};

// the DRAWITEMSTRUCT contains the states of the menu items, as well as
// the position index of the item in the menu, which is duplicated in
// the UAHMENUITEM's iPosition as well
struct UAHDRAWMENUITEM
{
	DRAWITEMSTRUCT dis; // itemID looks uninitialized
	UAHMENU um;
	UAHMENUITEM umi;
};

static HTHEME menuTheme = nullptr;

// processes messages related to UAH / custom menubar drawing.
// return true if handled, false to continue with normal processing in your wndproc
bool UAHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr)
{
    switch (message)
    {
    case WM_UAHDRAWMENU:
    {
        UAHMENU* pUDM = (UAHMENU*)lParam;
        RECT rc = { 0 };

        // get the menubar rect
        {
            MENUBARINFO mbi = { sizeof(mbi) };
            GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

            RECT rcWindow;
            GetWindowRect(hWnd, &rcWindow);

            // the rcBar is offset by the window rect
            rc = mbi.rcBar;
            OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
        }

        FillRect(pUDM->hdc, &rc, brBarBackground);

        return true;
    }
    case WM_UAHDRAWMENUITEM:
    {
        auto pUDMI = (UAHDRAWMENUITEM*)lParam;

        HBRUSH* pbrBackground = &brItemBackground;
        HBRUSH* pbrBorder = &brItemBackground;

        // get the menu item string
        wchar_t menuString[256] = { 0 };
        MENUITEMINFO mii = { sizeof(mii), MIIM_STRING };
        {
            mii.dwTypeData = menuString;
            mii.cch = (sizeof(menuString) / 2) - 1;

            GetMenuItemInfoW(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);
        }

        // get the item state for drawing

        DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

        int iTextStateID = 0;
        {
            if ((pUDMI->dis.itemState & ODS_INACTIVE) || (pUDMI->dis.itemState & ODS_DEFAULT)) 
                // normal display
                iTextStateID = MBI_NORMAL;
            if (pUDMI->dis.itemState & ODS_HOTLIGHT) {
                // hot tracking
                iTextStateID = MBI_HOT;

                pbrBackground = &brItemBackgroundHot;
                pbrBorder = &brItemBorder;
            }
            if (pUDMI->dis.itemState & ODS_SELECTED) {
                // clicked
                iTextStateID = MBI_PUSHED;

                pbrBackground = &brItemBackgroundSelected;
                pbrBorder = &brItemBorder;
            }
            if ((pUDMI->dis.itemState & ODS_GRAYED) || (pUDMI->dis.itemState & ODS_DISABLED))
                // disabled / grey text
                iTextStateID = MBI_DISABLED;
            if (pUDMI->dis.itemState & ODS_NOACCEL)
                dwFlags |= DT_HIDEPREFIX;
        }

        if (!menuTheme)
            menuTheme = OpenThemeData(hWnd, L"Menu");

        DTTOPTS opts = { sizeof(opts), DTT_TEXTCOLOR, iTextStateID != MBI_DISABLED ? RGB(0x00, 0x00, 0x20) : RGB(0x40, 0x40, 0x40), .crText = fgItemBackground };

        FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, *pbrBackground);
        FrameRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, *pbrBorder);
        DrawThemeTextEx(menuTheme, pUDMI->um.hdc, MENU_BARITEM, MBI_NORMAL, menuString, mii.cch, dwFlags, &pUDMI->dis.rcItem, &opts);

        return true;
    }
    default:
        return false;
    }
}
#pragma endregion

using RtlGetNtVersionNumbers_T = void (WINAPI *)(LPDWORD, LPDWORD, LPDWORD);
RtlGetNtVersionNumbers_T RtlGetNtVersionNumbers;

using DefWindowProcW_T = decltype(&DefWindowProcW);
DefWindowProcW_T DefWindowProcW_Original;

LRESULT CALLBACK DefWindowProcW_Hook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lr = 0;
    if(UAHWndProc(hWnd, uMsg, wParam, lParam, &lr))
        return lr;

    return DefWindowProcW_Original(hWnd, uMsg, wParam, lParam);
}

enum AppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

using FlushMenuThemes_T = void (WINAPI *)();
using SetPreferredAppMode_T = HRESULT (WINAPI *)(AppMode);

FlushMenuThemes_T FlushMenuThemes;
SetPreferredAppMode_T SetPreferredAppMode;

//Applies the theme to all menus.
HRESULT ApplyTheme(const AppMode inputTheme = Max)
{
    AppMode theme = inputTheme;

    //Get the saved theme from the settings.
    if(theme == Max)
    {
        PCWSTR savedTheme = Wh_GetStringSetting(L"AppMode");

        if(wcscmp(savedTheme, L"AllowDark") == 0)
            theme = AllowDark;
        else if(wcscmp(savedTheme, L"ForceLight") == 0)
            theme = ForceLight;
        else
            theme = ForceDark;

        Wh_FreeStringSetting(savedTheme);
    }

    //Apply the theme
    FlushMenuThemes();
    return SetPreferredAppMode(theme);
}

//Checks if the windows build is 18362 or later.
bool IsAPISupported()
{
    if(!RtlGetNtVersionNumbers)
    {
        HMODULE hNtDll = LoadLibraryExW(L"ntdll.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        FARPROC pRtlGetNtVersionNumbers = GetProcAddress(hNtDll, "RtlGetNtVersionNumbers");
	    RtlGetNtVersionNumbers = reinterpret_cast<RtlGetNtVersionNumbers_T>(pRtlGetNtVersionNumbers);
    }

    DWORD build;
    RtlGetNtVersionNumbers(nullptr, nullptr, &build);

    return build >= 18362;
}

//Updates the theme of the window when the settings change.
void Wh_ModSettingsChanged()
{
    Wh_Log(L"Settings changed");
    ApplyTheme();
}

//Import functions
BOOL Wh_ModInit() {
    if(!IsAPISupported())
    {
        Wh_Log(L"Outdated Windows version!");
        return FALSE;
    }

    Wh_Log(L"Init");

    WindhawkUtils::SetFunctionHook(DefWindowProcW, DefWindowProcW_Hook, &DefWindowProcW_Original);

    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    FARPROC pSetPreferredAppMode = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
    FARPROC pFlushMenuThemes = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136));

    SetPreferredAppMode = reinterpret_cast<SetPreferredAppMode_T>(pSetPreferredAppMode);
    FlushMenuThemes = reinterpret_cast<FlushMenuThemes_T>(pFlushMenuThemes);

    HRESULT hResult = ApplyTheme();
    return SUCCEEDED(hResult);
}

//Restores the default theme.
void Wh_ModUninit()
{
    Wh_Log(L"Restoring the default theme.");
    ApplyTheme(Default);
}