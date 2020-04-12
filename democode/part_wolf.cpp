#include "demoshared.h"

static void ev_lerpin(void *ud)
{
    interp::FancyInterpolatedValue<u8> *colmul = (interp::FancyInterpolatedValue<u8>*)ud;
    colmul->interpolateTo(255, 800);
}

demopart part_wolf()
{
    loadIsinToScratch2();

    DrawImageHelper<data_wire_wolf_gif> img;
    img.applypal();
    img.draw(0, 0);

    DrawMesh<data_icosahedron_obj> mesh;
    raster::Params rp;
    rp.align = 2;
    rp.alignoffs = 0;
    //rp.face = 1;
    rp.incr = 3;
    //rp.backfaceoffs = 1;
    rp.rubmul = 0;
    rp.glitchmul = 0;

    partdone = 0;
    u16 q = 0;

    const u8vec3 mehv(0x33,0x33,0x66);
    const u8vec3 mehv2(0x0,0x44,0x22);

    raster::ClearHelper<4> clr;
    const ivec2 meshpos(int(img.w)-8, int(img.h /2)-16);

    interp::Interpolator<4> ilp(16);
    MusicSyncVar<u16> syncx(ilp, BEATSYNC_CHANNEL1, 0, 0xC00, 1800);
    MusicSyncVar<u8vec3> synccol1(ilp, BEATSYNC_CHANNEL2, mehv, u8vec3(80,80,80), 3000);
    MusicSyncVar<u8vec3> synccol2(ilp, BEATSYNC_CHANNEL3, u8vec3(99,99,255), u8vec3(255,255,255), 4000);
    interp::FancyInterpolatedValue<u8> colmul(0);
    ilp.add(colmul);
    evs::schedule(1000, ev_lerpin, &colmul);

    u8 exiting = false;
    while(true)
    {
        const u8 cm = colmul;

        // start fading out the thing
        if(!exiting && partdone)
        {
            exiting = true;
            colmul.interpolateTo(0, 1000);
        }

        const Color meh = LCD::gencolor_inl(
            scale8(mehv.r, cm),
            scale8(mehv.g, cm),
            scale8(mehv.b, cm));
        const Color meh2 = LCD::gencolor_inl(
            scale8(mehv2.r, cm),
            scale8(mehv2.g, cm),
            scale8(mehv2.b, cm));
        palsetcolor(0, meh);
        palsetcolor(1, meh2);

        {
            uint16_t plopx = syncx;
            fp1616 scalex = fp1616::raw(17 + uhi8(plopx), ulo8(plopx) << 8);
            mat4rx mrx(q, FSinCosScratch());
            mat4rz mrz(q/2, FSinCosScratch());
            mat4ry mry(q/3, FSinCosScratch());
            mat4s ms(scalex);
            const auto m = ms * mrx * mrz * mry;
            mesh.transform(m, meshpos);
        }

        clr.add(mesh.getAABB(), 3);
        clr.clear(0);

        raster::TriFilterResult filt = mesh.filter();
        mesh.draw(mesh.frontFaceIdxs(), filt.nFront, rp, 0, 0);

        //Color bgcol = LCD::gencolor_inl(80,80,80); // --> lerp to meh
        //Color blue = LCD::gencolor_inl(99,99,255); // --> lerp white to this
        Color bgcol = gencolorScaled(synccol1, cm);
        Color blue = gencolorScaled(synccol2, cm);

        if(bgcol != meh)
        {
            const uint8_t *back = mesh.backFaceIdxs(filt.nBack);
            mesh.drawWireframe(back, filt.nBack, bgcol);
        }

        const uint8_t *front = mesh.frontFaceIdxs();
        mesh.drawWireframe(front, filt.nFront, blue);

        ++q;

        if(!cm && exiting)
            break;
    }


    // move away
    fp1616 d = 14;
    for(u16 x = 0; x < LCD::WIDTH; ++x)
    {
        LCD::setxy(x, 0, x, LCD::YMAX);
        LCD::setColor(0);
        LCD::fastfill_u16(LCD::HEIGHT);
        LCD::set_scroll_pos(x);
        u16 dd = d.intpart();
        while(dd--)
            fglcd::delay_ms(1);
        d *= fp1616(0.99f);
    }

    LCD::set_scroll_pos(0);
}
