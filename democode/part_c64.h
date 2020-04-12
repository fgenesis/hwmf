#pragma once

// Draw area inside the plastic frame
enum PartLimits
{
    AREA_X_BEGIN = 31,
    AREA_Y_BEGIN = 32,
    AREA_X_END = 445, // inclusive
    AREA_Y_END = 288,
    AREA_W = AREA_X_END - AREA_X_BEGIN + 1,
    AREA_H = AREA_Y_END - AREA_Y_BEGIN + 1,
};
