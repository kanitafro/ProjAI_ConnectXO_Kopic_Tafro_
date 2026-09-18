#pragma once

// Centralized theme indices to avoid magic numbers across the codebase.
enum class ThemeIndex : int
{
    Classic = 0,
    Nature = 1,
    Strawberry = 2,
    Beachy = 3,
    Dark = 4,
};

inline ThemeIndex clampThemeIndex(int idx)
{
    if (idx < static_cast<int>(ThemeIndex::Classic)) return ThemeIndex::Classic;
    if (idx > static_cast<int>(ThemeIndex::Dark)) return ThemeIndex::Dark;
    return static_cast<ThemeIndex>(idx);
}

inline int to_int(ThemeIndex t) { return static_cast<int>(t); }