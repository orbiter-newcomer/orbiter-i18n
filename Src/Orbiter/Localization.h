// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// Localization support for the Orbiter Launchpad and in-simulation UI.
//
// The language can only be switched from the Launchpad. The selection is saved
// to Localization.cfg and applied again on the next start.
//
// UI strings are translated at display time: the English originals remain in the
// source code / dialog resources and are mapped to the selected language by the
// translation table in Localization.cpp. This keeps the English UI fully usable
// as a fallback and makes adding translations a data-only change.

#ifndef __LOCALIZATION_H
#define __LOCALIZATION_H

#include <windows.h>
#include <string>

namespace orbiter {

	// Guard set while the Launchpad re-applies the UI language. Control
	// notifications (combobox selection changes etc.) triggered during that
	// process must be ignored to avoid recursive relocalization.
	bool IsRelocalizing();
	void SetRelocalizing(bool active);

	enum class Language {
		English = 0,
		Chinese = 1
	};

	// Load the language setting. Call once during application start-up,
	// before any UI is created.
	void InitLocalization();

	Language GetLanguage();
	bool IsChinese();

	// Set the active language and (optionally) persist it to Localization.cfg.
	// Does not update existing windows; call LocalizeWindowTree afterwards.
	void SetLanguage(Language lang, bool save = true);

	// Translate an English UI string to the current language.
	// Returns the passed pointer if no translation is available.
	const char* Tr(const char* english);
	const char* Tr2(const char* aliasKey);

	// Convert a UTF-8 string to UTF-16. The returned value must stay alive while
	// it is used (e.g. for Unicode control messages such as TVM_INSERTITEMW).
	std::wstring Utf8ToWideString(const char* utf8);

	// Add a UTF-8 string to a combobox / listbox using the Unicode control
	// messages (the ANSI variants would garble non-ASCII text).
	void ComboAddStringUtf8(HWND hCombo, const char* utf8);
	void ListBoxAddStringUtf8(HWND hListBox, const char* utf8);

	// Set the text of a window or control from a UTF-8 string, using the
	// Unicode API (SetWindowTextA would garble non-ASCII text on Chinese systems).
	void SetWindowTextUtf8(HWND hWnd, const char* utf8);
	// Convert UTF-16 to UTF-8.
	std::string WideToUtf8String(const wchar_t* w);

	// Re-translate a UI string that is currently displayed: it is mapped back to
	// its English original (if known) and then into the currently active
	// language. Used to refresh dynamic controls such as tree view items.
	std::string RetranslateCurrent(const char* currentUtf8);

	// Apply the active language to a window and all of its descendants
	// (dialog controls, tab pages, nested dialogs). Switching back to English
	// is supported via the reverse mapping of the translation table.
	void LocalizeWindowTree(HWND root);

	// Initialize the language selector combobox of the Launchpad.
	// The combobox must be a CBS_DROPDOWNLIST control.
	void InitLanguageCombo(HWND hCombo);

	// Select the combobox entry that matches the active language.
	void SyncLanguageCombo(HWND hCombo);

	// Read the language from a combobox selection and apply it.
	void ApplyLanguageFromCombo(HWND hCombo);

	// Path of the localization configuration file (relative to the Orbiter
	// working directory).
	const char* LocalizationConfigFile();

} // namespace orbiter

#endif // !__LOCALIZATION_H
