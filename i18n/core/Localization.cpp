// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#include "Localization.h"

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <map>
#include <fstream>
#include "Log.h"
#include <vector>
#include <set>

namespace orbiter {

	// ========================================================================
	// Translation table
	// ========================================================================
	//
	// Each entry maps the English UI text (exactly as it appears in the dialog
	// resources or source code) to its Chinese translation.
	//
	// Notes:
	//  - Multi-line entries use "\n" for line breaks. At lookup time the current
	//    window text is normalized ("\r\n" -> "\n") before it is matched, so the
	//    table stays independent of the control's line-ending style.
	//  - The reverse mapping (Chinese -> English) is built from the same table,
	//    which allows switching back to English without restarting.
	//  - Strings are UTF-8; non-ASCII text is applied through the wide-character
	//    path in LocalizeWindowTree, so ANSI dialog resources are fine.
	//  - Accelerator keys are preserved: "&Launch Orbiter" -> "启动(&L)".
	//  - Non-translatable tokens (numbers, "4:3", "256 x 256", "v.123456", ...)
	//    are intentionally not listed.

	struct Translation {
		const char* en;
		const char* zh;
	};

	// Context-specific translation entry. Used when the same English word needs
	// different translations depending on where it occurs (e.g. "Target" as a
	// button vs. as an MFD mode label).
	struct ContextTranslation {
		const char* context;
		const char* en;
		const char* zh;
	};


	// ========================================================================
	// State
	// ========================================================================

	static Language g_language = Language::English;
	static bool g_tablesBuilt = false;
	static std::map<std::string, std::string> g_enToZh;
	static std::map<std::string, std::string> g_zhToEn;
	static std::map<std::string, std::string> g_enToZhLower;
	// Context-specific translations: key = context + '\x1f' + normalized English.
	static std::map<std::string, std::string> g_ctxEnToZh;
	// Case-insensitive keys that map to different translations (e.g. "rad"/"Rad").
	// Strict lookup is still exact; these keys are excluded from the case-insensitive
	// fallback so that e.g. "rad" can never resolve to the translation of "Rad".
	static std::set<std::string> g_caseAmbiguous;

	static std::string NormalizeLineEndings(const std::string &s);

	// 别名键（SURF_*、ORBIT_PRJ_*、MFD_*、PANEL_*、插件按钮等）只用于 Tr2()/代码内部
	// 查表，不是真实 UI 文本，因此不参与反向映射（否则会用别名覆盖窗口文本的中文→英文恢复）。
	static bool IsAliasKey(const std::string &en)
	{
		static const char *prefix[] = { "MFD_", "PANEL_", "TRX_", "TERM_", "APA_", "ATT_", "ASC_", "GCAM_", "SURF_", "ORBIT_PRJ_" };
		for (size_t i = 0; i < sizeof(prefix)/sizeof(prefix[0]); i++) {
			size_t n = strlen(prefix[i]);
			if (en.size() >= n && en.compare(0, n, prefix[i]) == 0) return true;
		}
		return false;
	}

	// Add one English->Chinese entry to the lookup tables. Shared by the static
	// translation data and by run-time registrations (oapiRegisterTranslation).
	static void AddTranslationEntry(const char *en0, const char *zh)
	{
		if (!en0 || !zh) return;
		if (strcmp(en0, zh) == 0) return;  // placeholder
		std::string en = NormalizeLineEndings(en0);
		std::string lower = en;
		for (char &c : lower) c = (char)tolower((unsigned char)c);
		g_enToZh[en] = zh;
		// Build the case-insensitive fallback table. If two entries differ only by
		// letter case but have different translations (e.g. "rad" vs "Rad"), the
		// fallback is ambiguous: mark the key so Tr() uses strict matching only.
		auto itL = g_enToZhLower.find(lower);
		if (itL == g_enToZhLower.end()) {
			g_enToZhLower[lower] = zh;
		} else if (itL->second != zh) {
			g_caseAmbiguous.insert(lower);
		}
		if (!IsAliasKey(en)) g_zhToEn[zh] = en;
	}

	// Load the bundled Chinese translation catalogs (UTF-8 .po subset) from the
	// game directory. The data is external on purpose: translations can be
	// edited or extended without recompiling the executable, and addons can ship
	// their own catalogs. Missing files are simply skipped (English fallback).
	static void LoadBundledCatalogs()
	{
		static const char *files[] = {
			"i18n\\locales\\zh_CN\\core.po",
			"i18n\\locales\\zh_CN\\launchpad.po",
			"i18n\\locales\\zh_CN\\scenario.po",
			"i18n\\locales\\zh_CN\\plugins.po",
			"i18n\\locales\\zh_CN\\scripts.po",
		};
		for (size_t i = 0; i < sizeof(files)/sizeof(files[0]); i++)
			RegisterTranslationCatalog(files[i]);
	}

static void BuildTables()
	{
		if (g_tablesBuilt) return;
		g_tablesBuilt = true;
		// Translation data is loaded from external .po catalogs (see
		// LoadBundledCatalogs / oapiRegisterTranslationCatalog), not compiled in.
	}

	// Normalize line endings: "\r\n" -> "\n", lone "\r" -> "\n".
	// The translation table stores line breaks as "\n".
	static std::string NormalizeLineEndings(const std::string &s)
	{
		std::string r;
		r.reserve(s.size());
		for (size_t i = 0; i < s.size(); i++) {
			if (s[i] == '\r') {
				if (i + 1 < s.size() && s[i + 1] == '\n') continue;
				r += '\n';
			} else {
				r += s[i];
			}
		}
		return r;
	}

	// ========================================================================
	// UTF-8 <-> UTF-16 conversion
	// ========================================================================

	static std::wstring Utf8ToWide(const char* utf8)
	{
		if (!utf8 || !*utf8) return std::wstring();
		int len = int(strlen(utf8));
		int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, len, nullptr, 0);
		if (wlen <= 0) return std::wstring();
		std::wstring w(size_t(wlen), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, utf8, len, &w[0], wlen);
		return w;
	}

	static std::string WideToUtf8(const wchar_t* w)
	{
		if (!w || !*w) return std::string();
		int len = int(wcslen(w));
		int n = WideCharToMultiByte(CP_UTF8, 0, w, len, nullptr, 0, nullptr, nullptr);
		if (n <= 0) return std::string();
		std::string s(size_t(n), '\0');
		WideCharToMultiByte(CP_UTF8, 0, w, len, &s[0], n, nullptr, nullptr);
		return s;
	}

	// ========================================================================
	// Configuration
	// ========================================================================

	static const char* kCfgFile = "Localization.cfg";

	const char* LocalizationConfigFile()
	{
		return kCfgFile;
	}

	void InitLocalization()
	{
		BuildTables();
		LoadBundledCatalogs();   // external UTF-8 .po catalogs (i18n/locales/zh_CN/)
		g_language = Language::English;

		std::ifstream ifs(kCfgFile);
		if (ifs) {
			std::string line;
			while (std::getline(ifs, line)) {
				while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
					line.pop_back();
				size_t pos = line.find_first_not_of(" \t");
				if (pos != std::string::npos && pos > 0)
					line = line.substr(pos);
				if (line.empty() || line[0] == ';' || line[0] == '#') continue;
				if (line.compare(0, 9, "Language=") == 0) {
					std::string v = line.substr(9);
					if (v == "zh" || v == "ZH" || v == "Chinese")
						g_language = Language::Chinese;
					else
						g_language = Language::English;
				}
			}
		}
	}

	Language GetLanguage()
	{
		return g_language;
	}

	bool IsChinese()
	{
		return g_language == Language::Chinese;
	}

	void SetLanguage(Language lang, bool save)
	{
		g_language = lang;
		BuildTables();
		if (!save) return;

		std::ofstream ofs(kCfgFile);
		if (ofs) {
			ofs << "; Orbiter localization settings\n";
			ofs << "; Language: en = English, zh = Chinese\n";
			ofs << "Language=" << (lang == Language::Chinese ? "zh" : "en") << "\n";
		}
	}


	// ------------------------------------------------------------------------
	// Alias keys: allow dedicated keys (e.g. "SURF_GS") to be translated
	// without clashing with other UI texts. The alias maps to the original
	// English text, which is returned in English mode.
	// ------------------------------------------------------------------------
	struct AliasEntry { const char* alias; const char* english; };
	static const AliasEntry g_aliases[] = {
		{ "SURF_Title", "Surface: " },
		{ "SURF_ATM",  "ATM DATA" },
		{ "SURF_OAT",  "OAT" },
		{ "SURF_M",    "M" },
		{ "SURF_DNS",  "DNS" },
		{ "SURF_STP",  "STP" },
		{ "SURF_DNP",  "DNP" },
		{ "SURF_EQP",  "EQU POS" },
		{ "SURF_RATE", "RATE" },
		{ "SURF_BNK",  "BNK" },
		{ "SURF_PTCH", "PTCH" },
		{ "SURF_L",    "L" },
		{ "SURF_R",    "R" },
		{ "SURF_ALTR", "ALT-R" },
		{ "SURF_ALT",  "ALT" },
		{ "SURF_VS",   "VS" },
		{ "SURF_VACC", "VACC" },
		{ "SURF_AOA",  "AOA" },
		{ "SURF_ACC",  "ACC" },
		{ "SURF_GS",   "GS" },
		{ "SURF_TAS",  "TAS" },
		{ "SURF_IAS",  "IAS" },
		{ "SURF_OS",   "OS" },
		{ "ORBIT_PRJ_ECL",  "Ecliptic" },
		{ "ORBIT_PRJ_SHIP", "Ship" },
		{ "ORBIT_PRJ_TGT",  "Target" },
		{ "MFD_MAP_DN",       "DN" },   // Map MFD 的 DN 按钮（向下滚动）
		{ "MFD_MAPOLD_LEFT",  "<<" },   // 旧版 Map MFD 的滚动按钮
		{ "MFD_MAPOLD_RIGHT", ">>" },
		{ "MFD_MAPOLD_ZOOM",  "ZM" },
		// ---- MFD 按钮标签别名（玻璃座舱 2D 面板；由 MfdZh.inc 使用）----
		{ "MFD_MAP_TRK",   "TRK" },
		{ "MFD_SYNC_MOD",  "MOD" },
		{ "MFD_TRANS_HTO", "HTO" },
		{ "PANEL_PWR", "PWR" },
		{ "PANEL_SEL", "SEL" },
		{ "PANEL_MNU", "MNU" },
		// TransX
		{ "TRX_HLP", "HLP" },
		{ "TRX_FWD", "FWD" },
		{ "TRX_BCK", "BCK" },
		{ "TRX_VW",  "VW" },
		{ "TRX_VAR", "VAR" },
		{ "TRX_PRV", "-VR" },
		{ "TRX_ADJ", "ADJ" },
		{ "TRX_ADM", "-AJ" },
		{ "TRX_INC", "++" },
		{ "TRX_DEC", "--" },
		{ "TRX_EXE", "EXE" },
		{ "TRX_ENT", "ENT" },
		// Terminal MFD（LuaMFD 插件）
		{ "TERM_INP",  "INP" },
		{ "TERM_NEW",  "NEW" },
		{ "TERM_DEL",  "DEL" },
		{ "TERM_PGUP", "<PG" },
		{ "TERM_PGDN", "PG>" },
		// Ascent profile MFD（Orbitersdk/samples/AscentMFD 示例插件）
		{ "ASC_PG",  "PG" },
		{ "ASC_AR",  "AR" },
		{ "ASC_VRR", "VRR" },
		{ "ASC_VTR", "VTR" },
		// Generic Camera MFD（D3D9Client 示例插件：通用相机）
		{ "GCAM_NA",   "NA" },
		{ "GCAM_PA",   "PA" },
		{ "GCAM_ND",   "ND" },
		{ "GCAM_PD",   "PD" },
		{ "GCAM_FWD",  "FWD" },
		{ "GCAM_BCK",  "BWD" },
		{ "GCAM_VES",  "VES" },
		{ "GCAM_NV",   "NV" },
		{ "GCAM_ZIN",  "ZM+" },
		{ "GCAM_ZOUT", "ZM-" },
		{ "GCAM_PAR",  "PAR" },
		{ "GCAM_CRS",  "CRS" },
		// AscentAP MFD（Atlantis 上升自动驾驶）
		{ "APA_LAUNCH",    "L" },
		{ "APA_ENGAGE",    "EA" },
		{ "APA_DISENGAGE", "DA" },
		{ "APA_PGUP",      "PG-" },
		{ "APA_PGDN",      "PG+" },
		{ "APA_AZM",       "AZ-" },
		{ "APA_AZP",       "AZ+" },
		{ "APA_ALM",       "AL-" },
		{ "APA_ALP",       "AL+" },
		{ "APA_OMS2",      "OM2" },
		// Attitude RCS MFD
		{ "ATT_SET",  "SET" },
		{ "ATT_DCK",  "DCK" },
		{ "ATT_SUS",  "SUS" },
		{ "ATT_END",  "END" },
		{ "ATT_CFG",  "CFG" },
		{ "ATT_ADDR", "+R" },
		{ "ATT_DELR", "-R" },
		{ "ATT_AXIS", "AX" },
		{ "ATT_INC",  "+V" },
		{ "ATT_DEC",  "-V" },
		{ "ATT_UP",   "UP" },
		{ "ATT_DN",   "DN" },
		{ "ATT_GO",   "GO" },
		{ "ATT_RTN",  "RTN" },
		{ "ATT_BAS",  "BAS" },
		{ "ATT_INV",  "INV" },
		{ "ATT_CUR",  "CUR" },
		{ "ATT_NAV",  "NAV" },
		{ "ATT_ACT",  "ACT" },
		{ "ATT_CNC",  "CNC" },
	};
	static const size_t kNumAliases = sizeof(g_aliases) / sizeof(g_aliases[0]);

	const char* Tr2(const char* aliasKey)
	{
		if (!aliasKey || !*aliasKey) return aliasKey;
		if (g_language == Language::English) {
			for (size_t i = 0; i < kNumAliases; i++)
				if (strcmp(g_aliases[i].alias, aliasKey) == 0) return g_aliases[i].english;
			return aliasKey;
		}
		return Tr(aliasKey);
	}
	const char* Tr(const char* english)
	{
		if (!english || !*english) return english;
		if (g_language == Language::English) return english;
		BuildTables();
		std::string key = NormalizeLineEndings(english);
		auto it = g_enToZh.find(key);
		if (it != g_enToZh.end()) return it->second.c_str();
		{
			std::string lower = key;
			for (char &c : lower) c = (char)tolower((unsigned char)c);
			// Case-insensitive fallback only for keys without case-variant ambiguity
			if (!g_caseAmbiguous.count(lower)) {
				auto it2 = g_enToZhLower.find(lower);
				if (it2 != g_enToZhLower.end()) return it2->second.c_str();
			}
		}
		return english;
	}

	// ========================================================================
	// Extended localisation interface (exposed to plugins via OrbiterAPI)
	// ========================================================================

	const char* TrContext(const char* context, const char* english)
	{
		if (!english || !*english) return english;
		if (g_language == Language::English) return english;
		BuildTables();
		if (context && *context) {
			std::string key = context;
			key += '\x1f';
			key += NormalizeLineEndings(english);
			auto it = g_ctxEnToZh.find(key);
			if (it != g_ctxEnToZh.end()) return it->second.c_str();
		}
		return Tr(english);   // fall back to the plain table
	}

	const char* GetOriginalName(const char* localized)
	{
		if (!localized || !*localized) return localized;
		BuildTables();
		auto it = g_zhToEn.find(localized);
		if (it != g_zhToEn.end()) return it->second.c_str();
		return localized;
	}

	const char* GetLocaleName()
	{
		return g_language == Language::Chinese ? "zh_CN" : "en";
	}

	bool RegisterTranslation(const char* locale, const char* english, const char* translated)
	{
		if (!english || !english[0] || !translated || !translated[0]) return false;
		if (locale && locale[0]) {
			// Only Chinese locales carry a translation; English needs none.
			std::string loc = locale;
			for (char &c : loc) c = (char)tolower((unsigned char)c);
			if (loc.compare(0, 2, "en") == 0) return true;   // nothing to register
			if (loc.compare(0, 2, "zh") != 0) return false;  // unsupported locale
		}
		BuildTables();
		AddTranslationEntry(english, translated);
		return true;
	}

	// Minimal .po parser: msgid/msgstr pairs, quoted strings (possibly continued
	// over several lines), comments and empty lines as separators.
	bool RegisterTranslationCatalog(const char* path)
	{
		if (!path || !*path) return false;
		std::ifstream ifs(path, std::ios::in | std::ios::binary);
		if (!ifs.is_open()) return false;

		std::string line, msgctxt, msgid, msgstr;
		enum State { S_NONE, S_CTX, S_ID, S_STR } state = S_NONE;
		bool have = false, hasCtx = false, any = false;

		auto unescape = [](const std::string &s) {
			std::string r;
			for (size_t i = 0; i < s.size(); i++) {
				if (s[i] == '\\' && i + 1 < s.size()) {
					char c = s[++i];
					switch (c) {
					case 'n':  r += '\n'; break;
					case 't':  r += '\t'; break;
					case 'r':  r += '\r'; break;
					case '"':  r += '"';  break;
					case '\\': r += '\\'; break;
					default:   r += c;    break;
					}
				} else {
					r += s[i];
				}
			}
			return r;
		};
		auto quoted = [](const std::string &l, size_t pos, std::string &out) -> bool {
			size_t a = l.find('"', pos);
			if (a == std::string::npos) return false;
			size_t b = l.rfind('"');
			if (b <= a) return false;
			out = l.substr(a + 1, b - a - 1);
			return true;
		};
		auto commit = [&]() {
			if (have && !msgid.empty() && !msgstr.empty()) {
				if (hasCtx && !msgctxt.empty()) {
					// Context-specific entry (e.g. msgctxt "scenario.description").
					std::string key = unescape(msgctxt);
					key += '\x1f';
					key += NormalizeLineEndings(unescape(msgid).c_str());
					g_ctxEnToZh[key] = unescape(msgstr);
					any = true;
				} else if (RegisterTranslation("zh_CN", unescape(msgid).c_str(), unescape(msgstr).c_str())) {
					any = true;
				}
			}
			msgctxt.clear(); msgid.clear(); msgstr.clear();
			have = hasCtx = false; state = S_NONE;
		};

		while (std::getline(ifs, line)) {
			if (!line.empty() && line[line.size()-1] == '\r') line.erase(line.size()-1);
			size_t p = line.find_first_not_of(" \t");
			if (p == std::string::npos) { commit(); continue; }  // blank line
			if (line[p] == '#') continue;                        // comment
			std::string t = line.substr(p);
			if (t.compare(0, 8, "msgctxt ") == 0) {
				if (have && (!msgid.empty() || hasCtx)) commit();  // previous entry without blank line
				have = true; hasCtx = true; state = S_CTX;
				std::string s; if (quoted(t, 7, s)) { msgctxt = s; }
			} else if (t.compare(0, 6, "msgid ") == 0) {
				if (have && !msgid.empty()) commit();  // previous entry without blank line
				have = true; state = S_ID;
				std::string s; if (quoted(t, 5, s)) { msgid = s; }
			} else if (t.compare(0, 7, "msgstr ") == 0) {
				state = S_STR;
				std::string s; if (quoted(t, 6, s)) { msgstr = s; }
			} else if (t[0] == '"') {
				std::string s;
				if (quoted(t, 0, s)) {
					if (state == S_CTX) msgctxt += s;
					else if (state == S_ID) msgid += s;
					else if (state == S_STR) msgstr += s;
				}
			}
		}
		commit();
		return any;
	}

	int Utf8ToWideBuffer(const char* utf8, wchar_t* buf, int buflen)
	{
		if (!utf8) return 0;
		int len = int(strlen(utf8));
		int n = MultiByteToWideChar(CP_UTF8, 0, utf8, len, nullptr, 0);
		if (n <= 0) return 0;
		int need = n + 1;   // include terminating null
		if (buf && buflen >= need) {
			MultiByteToWideChar(CP_UTF8, 0, utf8, len, buf, buflen);
			buf[n] = L'\0';
			return need;
		}
		return need;
	}

	int WideToUtf8Buffer(const wchar_t* wide, char* buf, int buflen)
	{
		if (!wide) return 0;
		int len = int(wcslen(wide));
		int n = WideCharToMultiByte(CP_UTF8, 0, wide, len, nullptr, 0, nullptr, nullptr);
		if (n <= 0) return 0;
		int need = n + 1;   // include terminating null
		if (buf && buflen >= need) {
			WideCharToMultiByte(CP_UTF8, 0, wide, len, buf, buflen, nullptr, nullptr);
			buf[n] = '\0';
			return need;
		}
		return need;
	}

	// Convert UTF-8 to the system ANSI code page. Used for legacy ANSI APIs (e.g.
	// the inline HTML control, which converts its char* input with CP_ACP).
	std::string Utf8ToAnsiString(const char* utf8)
	{
		if (!utf8 || !*utf8) return std::string();
		std::wstring w = Utf8ToWide(utf8);
		if (w.empty()) return std::string();
		int n = WideCharToMultiByte(CP_ACP, 0, w.c_str(), int(w.length()), nullptr, 0, "?", nullptr);
		if (n <= 0) return std::string();
		std::string s(size_t(n), '\0');
		WideCharToMultiByte(CP_ACP, 0, w.c_str(), int(w.length()), &s[0], n, "?", nullptr);
		return s;
	}

	std::wstring Utf8ToWideString(const char* utf8)
	{
		return Utf8ToWide(utf8);
	}

	void ComboAddStringUtf8(HWND hCombo, const char* utf8)
	{
		if (!hCombo || !utf8) return;
		std::wstring w = Utf8ToWide(utf8);
		SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)w.c_str());
	}

	void ListBoxAddStringUtf8(HWND hListBox, const char* utf8)
	{
		if (!hListBox || !utf8) return;
		std::wstring w = Utf8ToWide(utf8);
		SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)w.c_str());
	}

	void SetWindowTextUtf8(HWND hWnd, const char* utf8)
	{
		if (!hWnd) return;
		if (!utf8) { SetWindowTextW(hWnd, L""); return; }
		std::wstring w = Utf8ToWide(utf8);
		SetWindowTextW(hWnd, w.c_str());
	}

	// ========================================================================
	// Window tree localization
	// ========================================================================

	// Rebuild the item list of a combobox in the active language. Item data and
	// the current selection are preserved. Comboboxes whose items have no
	// translation are left untouched.
	// Read one combobox item as UTF-16. Combobox messages have no ANSI/Unicode
	// variants: the string encoding is determined by the window's Unicode flag.
	static std::wstring ComboGetItemText(HWND hCombo, int index)
	{
		if (IsWindowUnicode(hCombo)) {
			wchar_t wbuf[512];
			LRESULT len = SendMessageW(hCombo, CB_GETLBTEXT, (WPARAM)index, (LPARAM)wbuf);
			if (len == CB_ERR || len < 0) return std::wstring();
			return std::wstring(wbuf, size_t(len));
		}
		char abuf[1024];
		LRESULT len = SendMessageW(hCombo, CB_GETLBTEXT, (WPARAM)index, (LPARAM)abuf);
		if (len == CB_ERR || len < 0) return std::wstring();
		int n = MultiByteToWideChar(CP_ACP, 0, abuf, int(len), nullptr, 0);
		if (n <= 0) return std::wstring();
		std::wstring w(size_t(n), L'\0');
		MultiByteToWideChar(CP_ACP, 0, abuf, int(len), &w[0], n);
		return w;
	}

	static void ComboAddItemText(HWND hCombo, const std::wstring& text)
	{
		if (IsWindowUnicode(hCombo)) {
			SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)text.c_str());
			return;
		}
		int n = WideCharToMultiByte(CP_ACP, 0, text.c_str(), int(text.length()), nullptr, 0, nullptr, nullptr);
		if (n <= 0) { SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)""); return; }
		std::string s(size_t(n), '\0');
		WideCharToMultiByte(CP_ACP, 0, text.c_str(), int(text.length()), &s[0], n, nullptr, nullptr);
		SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)s.c_str());
	}

	// Rebuild the item list of a combobox in the active language. Item data and
	// the current selection are preserved. Comboboxes whose items have no
	// translation are left untouched.
	static void LocalizeComboBox(HWND hCombo)
	{
		int count = (int)SendMessageW(hCombo, CB_GETCOUNT, 0, 0);
		if (count <= 0) return;

		struct Item {
			std::wstring text;
			LRESULT data;
			bool hasData;
		};
		std::vector<Item> items;
		items.reserve(size_t(count));
		bool changed = false;

		for (int i = 0; i < count; i++) {
			std::wstring wtext = ComboGetItemText(hCombo, i);
			std::string current = WideToUtf8(wtext.c_str());
			std::string target = RetranslateCurrent(current.c_str());
			if (target != current) changed = true;
			Item it;
			it.text = Utf8ToWide(target.c_str());
			it.data = SendMessageW(hCombo, CB_GETITEMDATA, (WPARAM)i, 0);
			it.hasData = (it.data != CB_ERR);
			items.push_back(it);
		}
		if (!changed) return;   // leave dynamic lists (devices, modes, ...) alone

		LRESULT sel = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
		// Clear the selection first to avoid CBN_SELCHANGE notifications while
		// the list is rebuilt (they could re-enter the language switch).
		SendMessageW(hCombo, CB_SETCURSEL, (WPARAM)-1, 0);
		SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
		for (size_t i = 0; i < items.size(); i++) {
			ComboAddItemText(hCombo, items[i].text);
			if (items[i].hasData)
				SendMessageW(hCombo, CB_SETITEMDATA, (WPARAM)i, (LPARAM)items[i].data);
		}
		if (sel != CB_ERR)
			SendMessageW(hCombo, CB_SETCURSEL, (WPARAM)sel, 0);
	}
	static void LocalizeWindow(HWND hWnd)
	{
		wchar_t wbuf[2048];
		int len = GetWindowTextW(hWnd, wbuf, 2048);
		if (len <= 0) return;

		std::string current = WideToUtf8(wbuf);
		if (current.empty()) return;

		BuildTables();
		std::string key = NormalizeLineEndings(current);
		std::string target = current;
		if (g_language == Language::Chinese) {
			auto it = g_enToZh.find(key);
			if (it != g_enToZh.end()) target = it->second;
		} else {
			auto it = g_zhToEn.find(key);
			if (it != g_zhToEn.end()) target = it->second;
		}
		if (target == current) return;

		std::wstring wtarget = Utf8ToWide(target.c_str());
		SetWindowTextW(hWnd, wtarget.c_str());
	}

		static BOOL CALLBACK LocalizeChildProc(HWND hWnd, LPARAM lParam)
	{
		wchar_t cls[64];
		GetClassNameW(hWnd, cls, 64);
		if (wcscmp(cls, L"ComboBox") == 0)
			LocalizeComboBox(hWnd);
		else
			LocalizeWindow(hWnd);
		return TRUE;
	}

	static bool g_relocalizing = false;
	bool IsRelocalizing() { return g_relocalizing; }
	void SetRelocalizing(bool active) { g_relocalizing = active; }

	void LocalizeWindowTree(HWND root)
	{
		if (!root) return;
		LocalizeWindow(root);
		EnumChildWindows(root, LocalizeChildProc, 0);
	}

	// ========================================================================
	// Language selector combobox (Launchpad)
	// ========================================================================

	static const wchar_t* kLangNames[2] = { L"English", L"中文（简体）" };

	void InitLanguageCombo(HWND hCombo)
	{
		if (!hCombo) return;
		SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
		for (int i = 0; i < 2; i++)
			SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)kLangNames[i]);
		SyncLanguageCombo(hCombo);
	}

	void SyncLanguageCombo(HWND hCombo)
	{
		if (!hCombo) return;
		SendMessageW(hCombo, CB_SETCURSEL, (WPARAM)(g_language == Language::Chinese ? 1 : 0), 0);
	}

	void ApplyLanguageFromCombo(HWND hCombo)
	{
		if (!hCombo) return;
		LRESULT idx = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
		if (idx == CB_ERR) return;
		SetLanguage(idx == 1 ? Language::Chinese : Language::English, true);
	}

	std::string WideToUtf8String(const wchar_t* w)
	{
		return WideToUtf8(w);
	}

	std::string RetranslateCurrent(const char* currentUtf8)
	{
		if (!currentUtf8) return std::string();
		BuildTables();
		std::string key = NormalizeLineEndings(currentUtf8);
		std::string english = key;
		auto rev = g_zhToEn.find(key);
		if (rev != g_zhToEn.end()) english = rev->second;
		if (g_language == Language::English) return english;
		auto fwd = g_enToZh.find(english);
		return (fwd != g_enToZh.end()) ? fwd->second : english;
	}
} // namespace orbiter
