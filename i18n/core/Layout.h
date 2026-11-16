// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ============================================================================
// Layout.h - runtime layout data for the Chinese localisation.
//
// The Chinese UI needs a number of layout adjustments (extra offsets for wider
// glyphs, font sizes, column gaps, ...) that are coupled to the official
// drawing code. Historically these values were compiled into the program via
// i18n/*/…Zh.inc data files, which meant that any layout tweak required a full
// rebuild.
//
// This module moves such *values* (not algorithms) into an external, editable
// file that is loaded at run time:
//
//   i18n/layout/zh_CN/layout.ini
//
// The file is optional. Missing sections/keys fall back to the built-in
// defaults in the code, so an installation without the file behaves exactly as
// before (and English mode never reads it). Layout data is only loaded when
// the active language is Chinese.
// ============================================================================
#ifndef __ORBITER_LAYOUT_H
#define __ORBITER_LAYOUT_H

namespace orbiter {

	// Load layout values from an INI-style file ([section] headers, key=value
	// lines, ';' or '#' comments). Values already present are overwritten; the
	// file may be loaded more than once. Returns false if the file could not be
	// read (the defaults stay in effect).
	bool LoadLayoutFile(const char* path);

	// Drop all loaded layout values (used when the language is switched).
	void ClearLayout();

	// Query layout values. section/key are case-sensitive; def is returned for
	// missing entries. Values are parsed on first access and cached.
	int         LayoutInt(const char* section, const char* key, int def);
	double      LayoutFloat(const char* section, const char* key, double def);
	const char* LayoutString(const char* section, const char* key, const char* def);

} // namespace orbiter

#endif // !__ORBITER_LAYOUT_H
