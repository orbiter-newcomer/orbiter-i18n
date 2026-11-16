// ============================================================================
// TextCodec.inc - shared text encoding helpers for the D3D9Client render paths.
//
// Single place for conversions between UTF-8 (the program-internal encoding),
// UTF-16 (Unicode Win32 APIs / wide text path) and the single-byte
// ISO-8859-1/Windows-1252 path used by the legacy font renderer.
//
// Do not add ad-hoc MultiByteToWideChar / WideCharToMultiByte calls elsewhere
// in this module: extend this file instead. The corresponding core-side
// conversions live in i18n/core/Localization.cpp (orbiter::LocalToWide etc.).
// ============================================================================
#ifndef __D3D9CLIENT_TEXTCODEC_INC
#define __D3D9CLIENT_TEXTCODEC_INC

#include <windows.h>
#include <string>
#include <string.h>

// UTF-8 -> ISO-8859-1/Windows-1252 bytes for the single-byte text path.
// The input is returned unchanged when it is not valid UTF-8 (legacy plugins
// pass Windows-1252 strings directly).
static inline std::string Utf8ToCp1252 (const char *utf8, int ulen)
{
	if (!utf8) return std::string();
	if (ulen < 0) ulen = (int)strlen(utf8);
	if (ulen == 0) return std::string();
	int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, ulen, nullptr, 0);
	if (wlen <= 0) return std::string(utf8);   // not UTF-8: assume Windows-1252
	std::wstring w(size_t(wlen), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, utf8, ulen, &w[0], wlen);
	int n = WideCharToMultiByte(28591, 0, w.c_str(), int(w.size()), nullptr, 0, nullptr, nullptr);
	if (n <= 0) return std::string(utf8);
	std::string s(size_t(n), '\0');
	WideCharToMultiByte(28591, 0, w.c_str(), int(w.size()), &s[0], n, nullptr, nullptr);
	return s;
}

// UTF-8 -> UTF-16; falls back to the system ANSI code page when the input is
// not valid UTF-8 (legacy plugins provide ANSI strings).
static inline std::wstring Utf8ToWideLocal (const char *utf8, int ulen = -1)
{
	if (!utf8) return std::wstring();
	if (ulen < 0) ulen = (int)strlen(utf8);
	if (ulen == 0) return std::wstring();
	int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, ulen, nullptr, 0);
	UINT cp = CP_UTF8;
	if (wlen <= 0) {
		cp = CP_ACP;
		wlen = MultiByteToWideChar(CP_ACP, 0, utf8, ulen, nullptr, 0);
	}
	if (wlen <= 0) return std::wstring();
	std::wstring w(size_t(wlen), L'\0');
	MultiByteToWideChar(cp, 0, utf8, ulen, &w[0], wlen);
	return w;
}

// True if the text contains characters that the single-byte font path cannot
// represent (e.g. CJK characters); such text must use the wide text path.
static inline bool NeedsUnicodeText (const std::wstring &w)
{
	for (size_t i = 0; i < w.size(); i++)
		if ((unsigned int)w[i] > 0xFF) return true;
	return false;
}

#endif // !__D3D9CLIENT_TEXTCODEC_INC
