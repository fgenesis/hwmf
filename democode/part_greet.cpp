#include "demoshared.h"
#include "rotozoom.h"


// fake struct that decompresses to nothing
// this is required to instantiate the DrawMesh that provides the space for the triangle soup generated from data_revisionflat_obj
struct data_revisionbroken_obj_FAKE
{
static constexpr inline fp1616 scale() { return data_revisionflat_obj::scale(); }
static const unsigned Ntris = 95; // data_revisionflat_obj::Ntris;
// ^ NOTE: DO NOT INCREASE! This is the hard limit to fit into RAM without problems.
static const unsigned Nverts = Ntris * 3;
static const unsigned paloffs = 0;
static const bool blocked = false;
static const unsigned npal = 1;
static const unsigned short pal[1] PROGMEM_FAR;
static const PackType packtype = PK_RLE;
static const unsigned windowsize = 0;
static const unsigned ndata = 1;
static const unsigned char data[1] PROGMEM_FAR;
static const unsigned packedsize = 1;
static const unsigned fullsize =
    Ntris // color indices
    + Nverts * sizeof(svec3); // per-tri color + per-vertex coord, no index data
static const bool Indexed = false;
};

const unsigned char data_revisionbroken_obj_FAKE::data[1] = { 0 };
const unsigned short data_revisionbroken_obj_FAKE::pal[1] = { 0 };

struct GreetBaseState
{
    raster::Params rp;
    raster::ClearHelper<4> clr;
    Camera::Matrix camMat;

    NOINLINE GreetBaseState()
    {
        rp.align = 2;
        rp.alignoffs = 0;
        //rp.face = 2;
        rp.incr = 2;
        //rp.backfaceoffs = 1;
        //rp.backfaceColorOffs = 0;
        rp.rubmul = 255;
        rp.glitchmul = 0;

        // camera doesn't change, so this can be done here and cached
        Camera cam(8);
        cam.pos(vec3(0, 0, -15));
        cam.lookAt(vec3(0));
        camMat = cam.calcMatrix();
    }
};

struct BrokenState : GreetBaseState
{
    DrawMesh<data_revisionbroken_obj_FAKE, DM_NONE> mesh;
    svec2 originalVerts2D[data_revisionbroken_obj_FAKE::Nverts]; // don't have to save Z because it's known to be 0 (saves precious space!)

    raster::TriFilterResult filt;
    interp::Interpolator<2> interpolator;
    interp::FancyInterpolatedValue<u16> brokenness;
    interp::FancyInterpolatedValue<u8> icol;
    volatile u8 done;
    volatile bool mustRecalcVerts;
    volatile bool isRepaired;
    //u8 repairedTriIdx;

    static FORCEINLINE svec2 to2d(const svec3& v)
    {
        return svec2(v.x, v.y);
    }

    static void ev_recalc(void *ud)
    {
        BrokenState *self = (BrokenState*)ud;
        if(self->done)
        {
            self->done = 2;
            return;
        }
        self->mustRecalcVerts = true;
        if(self->brokenness)
            evs::schedule(80, ev_recalc, ud);
        else
        {
            self->isRepaired = true;
            self->done = 2;
        }
    }

    static void ev_start_shrink(void *ud)
    {
        BrokenState *self = (BrokenState*)ud;
        self->brokenness.interpolateTo(0, 300);
    }

    ~BrokenState()
    {
        if(done < 2)
        {
            done = 1;
            while(done == 1) {}
        }
    }

    BrokenState()
        : brokenness(20000)
        , icol(0)
        , done(0)
        , mustRecalcVerts(true)
        , isRepaired(false)
        //, repairedTriIdx(0)
    {
#ifdef _DEBUG
        printf("BrokenState total size = %u\n", unsigned(sizeof(*this)));
#endif
        // save original vertex data right after decompressing
        // HACK: since we don't need the triangle soup mesh data here,
        // and there is abolutely no space to spare:
        // just plow over the storage for now
        typedef DecompMeshData<data_revisionflat_obj> Original;
        static_assert(sizeof(Original) < sizeof(mesh), "won't fit");
        Original *orig = new(&mesh) Original; // this decompresses the data in place

        const raster::Tri * const tris = orig->getTriData();
        const svec3 * const vtxIn = orig->getVertexData();
        svec2 *vtxOut = &originalVerts2D[0];

        // skip the first few verts for which there's not enough RAM anymore.
        // gives the entire thing a nice, broken feel too
        for(u8 i = Original::Ntris - mesh.Ntris; i < Original::Ntris; ++i)
        {
            raster::Tri tri = tris[i];
            *vtxOut++ = to2d(vtxIn[tri.a]);
            *vtxOut++ = to2d(vtxIn[tri.b]);
            *vtxOut++ = to2d(vtxIn[tri.c]);
        }
        FGLCD_ASSERT(vtxOut - &originalVerts2D[0] == mesh.Nverts, "brkvtx");

        // re-init the actual storage
        new(&mesh) decltype(mesh);
        filt = mesh.noFilter();

        interpolator.add(brokenness);
        interpolator.add(icol);
        icol.interpolateTo(0xff, 300);
        brokenness.interpolateTo(60000, 600);
        interpolator.start(16);

        evs::schedule(4000, ev_start_shrink, this);
        ev_recalc(this);
    }

    void recalcVerts() // recalc vertex positions: use brokenness to translate away by some factor
    {
        TinyRng16 rng(34863);
        const u16 scale = brokenness;
        u8 k = 0;
        s16 /*tx, ty,*/ tz; //randomized + scaled by scale factor
        for(u16 i = 0; i < Countof(originalVerts2D); ++i, --k)
        {
            // offset verts in groups of 3
            if(!k)
            {
                k = 3;
                //tx = scale16(rng() & 0x0fff, scale);
                //ty = scale16(rng() & 0x0fff, scale);
                tz = scale16(rng() & 0x7fff, scale);
                u8 r = (u8)rng();
                /*if(r & 1)
                    tx = -tx;
                if(r & 2)
                    ty = -ty;*/
                if(r & 4)
                    tz = -tz;
            }
            svec2 v2 = originalVerts2D[i];
            svec3 v = svec3(v2.x, v2.y, fp88::raw(tz) );
            /*v.x.f.i += tx;
            v.y.f.i += ty;*/
            mesh.getVertexData()[i] = v;
        }
    }

    bool update(u16 c)
    {
        if(mustRecalcVerts)
        {
            recalcVerts();
            mustRecalcVerts = false;
        }
        u8 col = icol;
        palsetcolor(0, LCD::gencolor(col,col,col));

        mat4rz mrz(c+c/4, FSinCosScratch());
        mat4ry mry(c, FSinCosScratch());
        const auto m = camMat * mry * mrz;
        mesh.transform(m, ivec2(LCD::WIDTH / 2, LCD::HEIGHT / 2 - 40));

        clr.add(mesh.getAABB(), 6);
        clr.clear(0);

        mesh.draw(mesh.frontFaceIdxs(), filt.nFront, rp, 1, 0);

        return (ulo8(c) & 0x7f) == 0x3f && isRepaired; // true when it's orthogonal to camera and not visible
    }
};

struct RvnState : GreetBaseState
{
    DrawMesh<data_revisionflat_obj, DM_NONE> mesh;

    raster::TriFilterResult filt;
    interp::Interpolator<1> interpolator;
    interp::FancyInterpolatedValue<u8> icol;


    static void ev_fadeout(void *ud)
    {
        RvnState *rvn = (RvnState*)ud;
        rvn->icol.interpolateTo(0, 333);
        rvn->interpolator.add(rvn->icol);
        rvn->interpolator.start(16);
    }

    RvnState()
        : icol(0xff)
    {
        mesh.applyColorOffs(s8(0xff)); // uses only 1 color, make that the last one (sofa starts at color 0 so the first few are taken)

        filt = mesh.noFilter();
        palsetcolor(0xff, 0xffff);

        evs::schedule(7000, ev_fadeout, this);
    }

    bool update(u16 c)
    {
        const u8 col = icol;
        palsetcolor(0xff, LCD::gencolor(col,col,col));

        mat4rz mrz(c+c/4, FSinCosScratch());
        mat4ry mry(c, FSinCosScratch());
        const auto m = camMat * mry * mrz;
        mesh.transform(m, ivec2(LCD::WIDTH / 2, LCD::HEIGHT / 2 - 40));

        clr.add(mesh.getAABB(), 6);
        clr.clear(0);
        mesh.draw(mesh.frontFaceIdxs(), filt.nFront, rp, 1, 0);

        return col == 0;
    }
};

struct SofaState : GreetBaseState
{
    DrawMesh<data_sofa_obj, DM_STOREZ> sofa;
    u8 oldrotval;
    interp::FancyInterpolatedValue<u8> isub;
    interp::Interpolator<1> interpolator;
    u8 oldsub;
    u8 exiting;

    SofaState()
        : oldrotval(0)
        , isub(0xff)
        , oldsub(0)
        , exiting(false)
    {
        sofa.applypalNoShade(sofa.npal); // backup colors

        Camera cam(8);
        cam.pos(vec3(0, 4.4f, -15));
        cam.lookAt(vec3(0));
        camMat = cam.calcMatrix();
        //rp.align = 1;
        //rp.incr = 1;

        isub.interpolateTo(0, 533);
        interpolator.add(isub);
        interpolator.start(16);
    }

    /*FORCEINLINE void forceNextRedraw()
    {
        oldrotval ^= 0xff; // good enough
    }*/

    void update(u16 c)
    {
        const u8 ci = (u8)scale16by8(c, 0x74);
        const u8 rotval = ISIN8FAST(ci) >> 2;
        const u8 sub = isub;

        if(sub != oldsub)
        {
            oldsub = sub;
            dampenColors(0, sub, sofa.npal);
            goto redraw;
        }

        if(rotval != oldrotval)
        {
redraw:
            oldrotval = rotval;
            mat4ry mry(127 + rotval, FSinCosScratch());
            const auto m = camMat * mry;
            sofa.transform(m, ivec2(LCD::WIDTH / 2, LCD::HEIGHT / 2 + 120));

            clr.add(sofa.getAABB(), 1);
            clr.clear(0);

            raster::TriFilterResult sofafilt = sofa.filter();
            sofa.sortIndexZ(sofa.frontFaceIdxs(), sofafilt.nFront);
            sofa.draw(sofa.frontFaceIdxs(), sofafilt.nFront, rp, 1, 0);

            rp.alignoffs = 1 - rp.alignoffs;
        }
    }

    bool finish()
    {
        if(!exiting)
        {
            exiting = true;
            isub.interpolateTo(255, 1000);
        }
        return !isub.isInterpolating();
    }
};

struct CreditsThing
{
    typedef DrawFont<data_vga_glf> Font;

    char text[256]; // enuff
    Font font;
    FontTyper<Font> typer;
    u8 exiting;
    volatile u8 isdone;

    static uint16_t callback(char& c, LCD::PixelPos pp, void *ud)
    {
        if(c == '\n')
            return 400;
        return c ? 50 : 0;
    }

    CreditsThing()
        : font(scratch3)
        , typer(font, &text[0], callback, this)
        , exiting(false)
        , isdone(false)
    {
        // unpack first
        decompressRAM<PK_EXO, sizeof(text), fglcd::ProgmemFar>(&text[0], fglcd_get_farptr(data_exo_credits), sizeof(data_exo_credits));

        // font needs the scratch page that the depacker clobbers
        font.rebuildIndex();

        typer.setPos(LCD::WIDTH / 4 + 40, 30);

        typer.start(500);
    }

    static void ev_setdone(void *ud)
    {
        CreditsThing *self = (CreditsThing*)ud;
        self->isdone = 1;
    }

    bool update()
    {
        if(!exiting)
        {
            typer.update();
            if(typer.done())
            {
                exiting = true;
                evs::schedule(3000, ev_setdone, this);
            }
        }

        return isdone && partdone;
    }


    FORCEINLINE uint8_t done() const
    {
        return typer.done();
    }
};

struct RotozoomThing
{
    rotozoom::Helper<data_pcb64_png> impl;
    interp::Interpolator<2> inp;
    interp::FancyInterpolatedValue<u16> fade, yclr;
    u8 exiting;
    u16 _y;

    RotozoomThing()
        : inp(16), fade(255), yclr(0), exiting(0), _y(0)
    {
        impl.img.applypal(impl.img.npal); // backup palette
        fade.interpolateTo(0, 900);
        yclr.interpolateTo(LCD::HEIGHT / 2, 200);
        inp.add(fade);
        inp.add(yclr);
    }

    bool update(u16 c)
    {
        u8 f = fade;
        if(f)
            dampenColors(0, f, impl.img.npal);
        u16 yy = yclr;
        u16 y = _y;
        if(y < yy)
        {
            do
            {
                LCD::setxy(0, y, LCD::XMAX, y);
                LCD::setColor(0);
                LCD::fastfill_u16(LCD::WIDTH);
                ++y;
            }
            while(y < yy);
            _y = y;
        }

        impl.draw(c);

        if(exiting)
        {
            return f == 255;
        }
        else if(partdone) // set by music sync
        {
            exiting = 1;
            fade.interpolateTo(255, 2000);
        }

        return false;
    }
};


// part1: BrokenState
// part2: SofaState + RvnState
// part3: SofaState + CreditsThing
struct GreetShared
{
    GreetShared()
        : c(0)
        , stateId(0)

    {
        new(&part0.brk) BrokenState();
    }

    ~GreetShared() {}

    u16 c;
    u8 stateId;

    union
    {
        struct
        {
            BrokenState brk;
        } part0;

        struct
        {
            SofaState sofa;
            RvnState rvn;
        } part1;

        struct
        {
            SofaState sofa;
            CreditsThing credits;
        } part2;

        struct
        {
            RotozoomThing roto;
        } part3;
    };

    void update()
    {
        switch(stateId)
        {
        case 0:
            if(part0.brk.update(c++))
                next(); // and fall through
            else
                break;
        case 1:
            part1.sofa.update(c++);
            if(part1.rvn.update(c))
                next(); // and fall through
            else
                break;
        case 2:
            part2.sofa.update(c++);
            if(part2.credits.update() && part2.sofa.finish())
                next(); // and fall through
            else
                break;
        case 3:
            if(part3.roto.update(c += 66))
                next(); // and fall through
            else
                break;
        }
    }

    void next()
    {
        switch(stateId)
        {
        case 0:
            part0.brk.~BrokenState();
            new(&part1.rvn) RvnState;
            new(&part1.sofa) SofaState;
            break;
        case 1:
            part1.rvn.~RvnState();
            new (&part2.credits) CreditsThing;
            // sofa stays as it is
            break;
        case 2:
            part2.credits.~CreditsThing();
            part2.sofa.~SofaState();
            new (&part3.roto) RotozoomThing;
            partdone = false;
            break;
        case 3:
            part3.roto.~RotozoomThing();
        }

        ++stateId;
    }
};

demopart part_greet()
{
    loadIsinToScratch2();
    //loadUsinToScratch3();

    GreetShared state;
    DEBUG_PRINTF("sizeof(GreetShared) = %u\n", unsigned(sizeof(GreetShared)));
    DEBUG_PRINTF("sizeof(SofaState) = %u\n", unsigned(sizeof(SofaState)));

    partdone = false;
    do
        state.update();
    while(state.stateId < 4);
    partdone = false;
}
