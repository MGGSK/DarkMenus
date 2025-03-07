// ==WindhawkMod==
// @id              dark-menus
// @name            Dark mode context menus
// @description     Enables dark mode for all win32 menus.
// @version         1.1.1
// @author          Mgg Sk
// @github          https://github.com/MGGSK
// @include         *
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Dark mode context menus
Forces dark mode for all win32 context menus to create a more consistent UI.

### Before:
![Before](https://i.imgur.com/bGRVJz8.png)

### After:
![After](https://i.imgur.com/BURKEki.png)
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- AppMode: ForceDark
  $name: When do you want context menus in dark mode?
  $description: Set to "Only when Windows is in dark mode" if you switch between light and dark mode a lot or are using a tool like Auto Dark Mode.
  $options:
  - ForceDark: Always
  - AllowDark: Only when Windows is in dark mode.
*/
// ==/WindhawkModSettings==

#include <windhawk_api.h>
#include <windows.h>
#include <winerror.h>
#include <winnt.h>

enum AppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

using FlushMenuThemes_T = void (WINAPI *)();
using SetPreferredAppMode_T = HRESULT (WINAPI *)(AppMode appMode);

FlushMenuThemes_T FlushMenuThemes;
SetPreferredAppMode_T SetPreferredAppMode;

//Applies the theme to all menus.
HRESULT ApplyTheme(AppMode inputTheme = Max)
{
    AppMode theme = inputTheme;

    //Get the saved theme from the settings.
    if(theme == Max)
    {
        PCWSTR savedTheme = Wh_GetStringSetting(L"AppMode");

        if(wcscmp(savedTheme, L"AllowDark") == 0)
            theme = AllowDark;
        else
            theme = ForceDark;

        Wh_FreeStringSetting(savedTheme);
    }

    //Apply the theme
    FlushMenuThemes();
    return SetPreferredAppMode(theme);
}

bool ApplyThemeOld(bool darkMode = true);
bool ShouldUseOldApi();
BOOL InitOldApi();

//Updates the theme of the window when the settings change.
void Wh_ModSettingsChanged()
{
    if(ShouldUseOldApi())
        ApplyThemeOld();
    else
        ApplyTheme();
}

//Import functions
BOOL Wh_ModInit() {
    if(ShouldUseOldApi())
        return InitOldApi();

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
    ApplyTheme(Default);
}

#pragma region Compatibility with windows build 18361 or earlier

using AllowDarkModeForApp_T = bool (WINAPI*) (bool);
AllowDarkModeForApp_T AllowDarkModeForApp;

bool ApplyThemeOld(bool darkMode)
{
    FlushMenuThemes();
    return AllowDarkModeForApp(darkMode);
}

bool ShouldUseOldApi()
{
    OSVERSIONINFO osvi { .dwOSVersionInfoSize = sizeof(OSVERSIONINFO) };
    if(!GetVersionExW(&osvi))
        return false;

    return osvi.dwBuildNumber < 18362;
}

BOOL InitOldApi()
{
    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    FARPROC pAllowDarkModeForApp = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
    FARPROC pFlushMenuThemes = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136));
 
    AllowDarkModeForApp = reinterpret_cast<AllowDarkModeForApp_T>(pAllowDarkModeForApp);
    FlushMenuThemes = reinterpret_cast<FlushMenuThemes_T>(pFlushMenuThemes);

    bool succeeded = ApplyThemeOld();
    return succeeded;
}

#pragma endregion