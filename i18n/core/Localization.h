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
#include <istream>

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

	// Context-aware translation: resolves cases where the same English word needs
	// different translations depending on where it is used. Falls back to Tr() if
	// no context-specific entry exists. The returned pointer stays valid for the
	// lifetime of the process.
	const char* TrContext(const char* context, const char* english);

	// Reverse lookup: maps a localised name back to its English original (e.g. a
	// Chinese planet or base name back to the name used in scenario/resource
	// files). Returns the passed pointer if the name is not in the table.
	const char* GetOriginalName(const char* localized);

	// Active locale name: "en" or "zh_CN".
	const char* GetLocaleName();

	// Register a translation at run time (typically from a plugin during Open()).
	// locale may be "zh"/"zh_CN" (or NULL/"" for the active language); "en" is a
	// no-op. The strings are copied. Registration only adds entries, existing
	// entries are not replaced. Returns false on invalid arguments.
	bool RegisterTranslation(const char* locale, const char* english, const char* translated);

	// Load translations from a .po-style catalog file (msgid/msgstr pairs).
	// Comments (#) and multi-line strings are supported. Returns true if the file
	// could be opened and parsed.
	bool RegisterTranslationCatalog(const char* path);

	// UTF-8 <-> UTF-16 conversions with caller-provided buffers. Both return the
	// required buffer size in elements including the terminating null character,
	// and fill the buffer only if it is large enough (buf may be NULL to query
	// the required size).
	int Utf8ToWideBuffer(const char* utf8, wchar_t* buf, int buflen);
	int WideToUtf8Buffer(const wchar_t* wide, char* buf, int buflen);

	// Convert a UTF-8 string to the system ANSI code page (for legacy ANSI APIs
	// such as the inline HTML control). Unmappable characters become '?'.
	std::string Utf8ToAnsiString(const char* utf8);

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

	// Read a "BEGIN_<blockname> ... END_<blockname>" text block from a scenario
	// or scenario-folder description file, normalise it the way the scenario list
	// displays it (each source line becomes one text line, empty lines become line
	// breaks) and translate it through the context "scenario.description".
	//
	// The whole block is read line by line with std::getline, i.e. long lines are
	// never split (the old fixed-size line buffer inserted spurious spaces and
	// broke the translation lookup). The stream is rewound before scanning.
	//
	// Returns a buffer allocated with new[] that the caller must delete[], or NULL
	// if the block does not exist. In English mode the original text is returned.
	char *ReadScenarioDescription(std::istream &is, const char *blockname);

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
