#include "demoshared.h"

static const u8 BGW = 0x40;

static const uint16_t pal_cube[] PROGMEM_FAR =
{
    // fg
    LCD::gencolor_inl(0xff, 0x30, 0), // from part_c64
    LCD::gencolor_inl(0xaf, 0x35, 0),
    LCD::gencolor_inl(0xcf, 0x45, 0),
    // bg
    LCD::gencolor_inl(0x4f, BGW, BGW),
    LCD::gencolor_inl(0x8f, BGW, BGW),
    LCD::gencolor_inl(0xdf, BGW, BGW)
};

struct RasterState
{
    RasterState()
    {
        needDone = 2;
    }
    ~RasterState()
    {
        ++done;
        while(done < needDone) {}
    }
    raster::Params rp;
    fp1616 scale;
    u8 part;
    u8 tickrate;
    volatile u16 c;
    volatile u8 done;
    volatile u8 needDone;
};

static void ev_tick(void *ud)
{
    RasterState& s = *(RasterState*)ud;
    if(s.done)
    {
        ++s.done;
        return;
    }
    ++s.c;
    evs::schedule(s.tickrate, ev_tick, ud);
}

static void ev_raster(void *ud)
{
    RasterState& s = *(RasterState*)ud;
    if(s.done)
    {
        ++s.done;
        return;
    }

    u16 tm = 10;
    const u8 oldpart = s.part;

    if(s.part < 10)
    {
        if(s.part < 3)
        {
            if(s.scale < 330)
                s.scale *= fp1616(1.02f);
            else
                s.part = 3;

            if(s.part == 0 && s.scale > 40)
            {
                s.part = 1;

                s.rp.align = 4;
                //s.rp.face = 2;
                s.rp.incr = 4;
                //s.rp.backfaceoffs = 2;
            }

            if(s.part == 1 && s.scale > 80)
            {
                s.rp.incr = 4;
                s.part = 2;
                ++s.needDone; // ticker goes now, another thread to wait for when quitting
                ev_tick(ud);
            }
        }
        else if(s.part == 3)
        {
            tm = 16;
            constexpr fp1616 lim = 76;
            if(s.scale > lim)
                s.scale *= fp1616(0.993f);
            else
            {
                s.part = 10;
                s.scale = lim;
            }
        }
    }
    else
    {
        if(s.part == 10)
        {
            tm = 4444;
            ++s.part;
        }
        else if(s.part == 11)
        {
            if(s.rp.rubmul < 32)
            {
                tm = 90;
                ++s.rp.rubmul;
            }
            else
            {
                ++s.part;
                tm = 3333;
            }
        }
        else if(s.part == 12)
        {
            tm = 8;
            /*if(s.rp.glitchmul < 0xff)
                ++s.rp.glitchmul;
            else*/
                s.part = 13;
        }
        else if(s.part == 13)
        {
            if(s.rp.rubmul < 90)
            {
                tm = 50;
                ++s.rp.rubmul;
                s.scale *= fp1616(1.005f);
            }
            else
            {
                ++s.part;
                //tm = 500;
                //DoNextPartIn(tm);
                --s.needDone; // one thread less
#ifdef MCU_IS_PC
                for(u8 kk = 128; kk < 255; ++kk)
                    palsetcolor(kk, LCD::gencolor_inl(0xff, 0, 0xff));
#endif
                return;
            }
        }
    }
    //printf("part = %u, rubmul = %u, glitchmul = %u\n", oldpart, s.rp.rubmul, s.rp.glitchmul);
    evs::schedule(tm, ev_raster, ud);
}

demopart part_c64_to_cube()
{
    clearpal(0);

    fglcd::FarPtr palpf = fglcd_get_farptr(pal_cube);
    applyPal16_PF(palpf, Countof(pal_cube), 0);
    applyPal16_PF(palpf, Countof(pal_cube), Countof(pal_cube)); // backup

    loadIsinToScratch2();
    loadUsinToScratch3();

    RasterState rs;
    rs.part = 0;
    rs.done = 0;
    rs.scale = 5;
    rs.c = 0;
    rs.tickrate = 20;

    rs.rp.align = 1;
    rs.rp.alignoffs = 0;
    //rs.rp.face = 1;
    rs.rp.incr = 1;
    //rs.rp.backfaceoffs = 1; // now passed to draw()
    rs.rp.rubmul = 0;
    rs.rp.glitchmul = 0;
    //rs.rp.backfaceColorOffs = 3;

    interp::Interpolator<2> inp(16);
    MusicSyncVar<u8> glitchbeat(inp, BEATSYNC_CHANNEL1, 0, 255, 3900);
    interp::FancyInterpolatedValue<u8> fadeout(0);
    inp.add(fadeout);

    evs::schedule(200, ev_raster, &rs);

    DrawMesh<data_cube_obj> cube;
    raster::ClearHelper<8> clr;

    bool exiting = false;
    partdone = false;
    while(true)
    {
        u8 glitch = glitchbeat;
        brightenColors(0, glitch/2, Countof(pal_cube));

        if(partdone && !exiting)
        {
            exiting = true;
            fadeout.interpolateTo(255, 2400);
        }

        if(exiting)
        {
            u8 fo = fadeout;
            if(fo == 255)
                break;
            dampenColors(0, fo, Countof(pal_cube));
        }

        const u16 c = rs.c;
        s16 bx = ISIN8FAST(c);
        bx += bx/4;
        s16 tx = (bx + (LCD::WIDTH / 2) - 8);
        s16 ty = ((s16)ISIN8FAST(c / 2u)/2 + (LCD::HEIGHT / 2) + 4);

        mat4rz mrz(c + 32, FSinCosScratch());
        mat4rx mrx(c + (c/2), FSinCosScratch());
        mat4t mt(tx, ty, 0);
        //mat4s ms(105.0f + 5 * fcos8((c/2) + (c/4)));
        mat4s ms(rs.scale);
        const auto m = mt * ms * mrx * mrz;
        mat4ry mry(c*2, FSinCosScratch());
        const auto mm = m * mry;

        rs.rp.glitchmul = glitch;

        cube.transform(mm);

        if(rs.part >= 1)
            clr.clear(0);

        raster::TriFilterResult filt = cube.filter();
        cube.draw(cube.frontFaceIdxs(), filt.nFront, rs.rp, 0, 0);
        cube.draw(cube.backFaceIdxs(filt.nBack), filt.nBack, rs.rp, 1, 3);

        u8 pad = 8 + (rs.rp.rubmul >> u8(1u));
        if(rs.part >= 2 && rs.part <= 3)
            pad += 13;
        clr.add(cube.getAABB(), pad);
    }
    partdone = false;
}

#if 0

demopart part_test_3dobj()
{
    fuckpal();
    loadIsinToScratch2();

    DrawMesh<data_cube_obj, DM_NONE> mesh;
    mesh.applypal(0,0,0,0);

    DrawFont<data_vga_glf> font(scratch3);

    raster::Params rp;
    rp.align = 2;
    rp.alignoffs = 0;
    //rp.face = face;
    rp.incr = 2;
    //rp.backfaceoffs = 0;
    //rp.backfaceColorOffs = 0;
    rp.rubmul = 0;
    rp.glitchmul = 0;

    Camera cam(8);
    cam.pos(vec3(0, 0, -10));
    cam.lookAt(vec3(0));

    const fp1616 test = invtan88slow(8);
    char buf[500];
    snprintf_P(buf, sizeof(buf), PSTR("DBG: %d:%u/%f, %f, %f, %f"),
        test.intpart(), test.mantissa(), test.tofloat(),
        cam.projection._dyn[0].tofloat(),
        cam.projection._dyn[1].tofloat(),
        cam.projection._dyn[2].tofloat()
    );
    LCD::PixelPos pp(0,0);
    font.template drawStr<fglcd::RAM>(pp, buf, 0xffff, 0);

    s16 c = 0;

    partdone = false;
    while(!partdone)
    {
        mat4rz mrz(c + 32, FSinCosScratch());
        mat4rx mrx(c + (c/2), FSinCosScratch());
        mat4ry mry(c*2, FSinCosScratch());
        mat4s ms(3.0f);

        const auto model = ms * mrx * mrz * mry;
        const auto m = cam.calcMatrix() * model;

        mesh.transform(m, ivec2(LCD::WIDTH / 2, LCD::HEIGHT / 2));
        raster::TriFilterResult filt = mesh.filter();
        uint8_t *front = mesh.frontFaceIdxs();
        mesh.draw(front, filt.nFront, rp, 0, 0);

        ++c;
    }
}
#endif
