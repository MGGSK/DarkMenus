// ==WindhawkMod==
// @id                dark-menus
// @version           1.3
// @author            Mgg Sk
// @github            https://github.com/MGGSK
// @include           *
// @license           MIT

// @name              Dark mode context menus
// @description       Enables dark mode for all win32 menus.

// @name:de-DE        Dunkler Modus für Menüs
// @description:de-DE Aktiviert den Dunklen Modus für alle Win32 Menüs.
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
  $description: Set to "Use system setting" if you switch between light and dark mode a lot or are using a tool like Auto Dark Mode.
  $options:
  - ForceDark: Always
  - AllowDark: Use system setting
  - ForceLight: Never

  $name:de-DE: Wann soll der Dunkle Modus aktiviert sein?
  $description:de-DE: Setzen sie auf "Systemeinstellung verwenden" wenn sie oft zwischen den Dunklen und Hellen Modus wechseln oder ein Program wie Auto Dark Mode verwenden.
  $options:de-DE:
  - ForceDark: Immer
  - AllowDark: Systemeinstellung verwenden
  - ForceLight: Nie
*/
// ==/WindhawkModSettings==

#include <windhawk_api.h>
#include <windows.h>

using RtlGetNtVersionNumbers_T = void (WINAPI *)(LPDWORD, LPDWORD, LPDWORD);
RtlGetNtVersionNumbers_T RtlGetNtVersionNumbers;

enum AppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

using FlushMenuThemes_T = void (WINAPI *)();
FlushMenuThemes_T FlushMenuThemes;

using SetPreferredAppMode_T = HRESULT (WINAPI *)(const AppMode);
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

    build &= ~0xF0000000;
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