#pragma once

enum ELevelTick
{
    LEVELTICK_TimeOnly = 0,         // Update the level time only.
    LEVELTICK_ViewportsOnly = 1,    // Update time and viewports.
    LEVELTICK_All = 2,              // Update all.
    LEVELTICK_PauseTick = 3,        // Delta time is zero, we are paused. Components don't tick.
};