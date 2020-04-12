// Included by all parts

#pragma once

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "src/demolib/demo_def.h"
#include "src/demolib/raster.h"
#include "src/demolib/vertex.h"
#include "src/demolib/drawmesh.h"
#include "src/demomath/tinyrng.h"
#include "src/demomath/fgmath.h"
#include "src/demolib/eventsystem.h"
#include "src/demolib/draw.h"
#include "src/demolib/drawhelper.h"
#include "src/demolib/decomp_lcd.h"
#include "src/demomath/camera.h"
#include "src/demolib/scrollhelper.h"
#include "src/demolib/drawfont.h"
#include "src/demolib/farload.h"
#include "src/demolib/interpolator.h"
#include "src/demolib/music.h"
#include "src/demolib/debugthing.h"
#include "src/demolib/demodebug.h"
#include "src/demomath/fast_hsv2rgb.h"
#include "src/demolib/musicsyncvar.h"

#include "data.h"


using fglcd::types::u8;
using fglcd::types::u16;
using fglcd::types::u32;
using fglcd::types::s8;
using fglcd::types::s16;
using fglcd::types::s32;

#define partdone G.demo.partdone

void DoNextPartIn(unsigned ms);
void WaitPartDone();
void cls();

//demopart partmissing(const char *name_P, uint16_t ms, bool wait);
demopart part_strobo();
demopart part_c64();
demopart part_layerchess();
demopart test_rotozoom();
demopart part_gol();
demopart part_widescroll();
demopart part_twist();
demopart test_showimage();
demopart test_showwaveform();
demopart part_sierpscroll();
demopart part_c64_to_cube();
demopart part_test_3dobj();
demopart part_greet();
demopart part_test_landscape();
demopart part_wolf();
demopart part_megadrivescroll();
demopart part_thissideup();
demopart part_cityscroll();
demopart part_intro1();
demopart part_intro2();
demopart part_end();

#define BEATSYNC_CHANNEL1 1
#define BEATSYNC_CHANNEL2 2
#define BEATSYNC_CHANNEL3 3

#ifdef CFG_USE_DEBUG_THING
#define DEBUG_THING(f, ud) DebugThing _dbg((f), (ud))
#define DEBUG_RETURN_IF_DONE() do { if(partdone) return; } while(0)
#else
#define DEBUG_THING(f, ud)
#define DEBUG_RETURN_IF_DONE() do {} while(0)
#endif

static FORCEINLINE Color gencolor(const u8vec3& v)
{
    return LCD::gencolor(v.x, v.y, v.z);
}

static inline Color gencolorScaled(const u8vec3& v, u8 scale)
{
    return LCD::gencolor(
        scale8(v.r, scale),
        scale8(v.g, scale),
        scale8(v.b, scale)
    );
}
