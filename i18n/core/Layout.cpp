// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ============================================================================
// Layout.cpp - runtime layout data for the Chinese localisation.
// See Layout.h for the file format and the design rationale.
// ============================================================================
#include "Layout.h"
#include <windows.h>
#include <map>
#include <string>
#include <fstream>
#include <stdlib.h>

namespace orbiter {

	// section -> (key -> raw value)
	typedef std::map<std::string, std::map<std::string, std::string> > LayoutMap;
	static LayoutMap g_layout;

	static std::string Trim(const std::string& s)
	{
		size_t b = s.find_first_not_of(" \t\r\n");
		if (b == std::string::npos) return std::string();
		size_t e = s.find_last_not_of(" \t\r\n");
		return s.substr(b, e - b + 1);
	}

	bool LoadLayoutFile(const char* path)
	{
		if (!path) return false;
		std::ifstream ifs(path, std::ios::binary);
		if (!ifs) return false;

		std::string section, line;
		while (std::getline(ifs, line)) {
			std::string t = Trim(line);
			if (t.empty() || t[0] == ';' || t[0] == '#') continue;
			if (t[0] == '[') {
				size_t e = t.find(']');
				if (e != std::string::npos) section = Trim(t.substr(1, e - 1));
				continue;
			}
			size_t eq = t.find('=');
			if (eq == std::string::npos) continue;
			std::string key = Trim(t.substr(0, eq));
			std::string val = Trim(t.substr(eq + 1));
			if (key.empty()) continue;
			if (val.size() >= 2 && ((val[0] == '"' && val[val.size() - 1] == '"')
				|| (val[0] == '\'' && val[val.size() - 1] == '\'')))
				val = val.substr(1, val.size() - 2);
			g_layout[section][key] = val;
		}
		return true;
	}

	void ClearLayout()
	{
		g_layout.clear();
	}

	static const std::string* FindValue(const char* section, const char* key)
	{
		if (!section || !key) return 0;
		LayoutMap::const_iterator it = g_layout.find(section);
		if (it == g_layout.end()) return 0;
		std::map<std::string, std::string>::const_iterator jt = it->second.find(key);
		if (jt == it->second.end()) return 0;
		return &jt->second;
	}

	int LayoutInt(const char* section, const char* key, int def)
	{
		const std::string* v = FindValue(section, key);
		if (!v) return def;
		char* end = 0;
		long r = strtol(v->c_str(), &end, 10);
		if (end == v->c_str()) return def;
		return (int)r;
	}

	double LayoutFloat(const char* section, const char* key, double def)
	{
		const std::string* v = FindValue(section, key);
		if (!v) return def;
		char* end = 0;
		double r = strtod(v->c_str(), &end);
		if (end == v->c_str()) return def;
		return r;
	}

	const char* LayoutString(const char* section, const char* key, const char* def)
	{
		const std::string* v = FindValue(section, key);
		return v ? v->c_str() : def;
	}

} // namespace orbiter
