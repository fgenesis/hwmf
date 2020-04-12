#pragma once

#include "decomp.h"
#include "vertex.h"
#include "raster.h"
#include "render.h"

void AddToRegion(uint8_t *p, uint8_t N, int8_t offs);

// Unpack Mesh to RAM
template<typename TMesh>
struct DecompMeshData : public _DecompDataStorage<TMesh::packtype, TMesh>, public TMesh
{
    typedef uint8_t IndexType; // this is hardcoded. don't change. no time to adjust the rest of the code, deadline....
    
    static_assert(!TMesh::blocked, "blocked mesh does not make sense");
    static_assert(TMesh::Ntris && TMesh::Nverts, "mesh must have tris & verts");
    typedef _DecompDataStorage<TMesh::packtype, TMesh> Storage;

    static const unsigned ColorOffs = 0;
    static const unsigned ColorSize = TMesh::Ntris;
    static const unsigned IndexOffs = ColorOffs + ColorSize;
    static const unsigned IndexSize = TMesh::Indexed ? (3 * TMesh::Ntris * sizeof(IndexType)) : 0;
    static const unsigned VertexOffs = IndexOffs + IndexSize;
    static const unsigned VertexSize = TMesh::Nverts * sizeof(svec3);
    static const unsigned ReqSize = VertexOffs + VertexSize;

    static const unsigned _fullsize = TMesh::fullsize;
    static_assert(_fullsize == ReqSize, "size mismatch");
    static_assert(sizeof(Storage) >= ReqSize, "too small"); // possible that storage has extra bytes

    uint8_t _shadelevels, _upshade, _downshade;
    DecompMeshData()
    {
        this->_prepare();
        _shadelevels = 0;
    }

    FORCEINLINE void applyColorOffs(int8_t offs)
    {
        AddToRegion(this->ptr() + ColorOffs, TMesh::Ntris, offs);
    }

    FORCEINLINE void applypal(uint8_t shadelevels, uint8_t mul, uint8_t upshade, uint8_t downshade, uint8_t offs = 0)
    {
        applyPal16_Mesh<TMesh>(shadelevels, mul, offs);
        _shadelevels = shadelevels;
        _upshade = upshade;
        _downshade = downshade;
    }
    FORCEINLINE void applypalNoShade(uint8_t offs = 0)
    {
        applyPal16_Mesh<TMesh>(0, 0, offs);
        _shadelevels = 0;
        _upshade = 0;
        _downshade = 0;
    }
    FORCEINLINE const uint8_t *getColorData() const { return static_cast<const uint8_t*>(this->ptr() + ColorOffs); }
    FORCEINLINE const raster::Tri *getTriData() const { return IndexSize ? reinterpret_cast<const raster::Tri*>(this->ptr() + IndexOffs) : NULL; }
    FORCEINLINE const svec3 *getVertexData() const { return reinterpret_cast<const svec3*>(this->ptr() + VertexOffs); }

    FORCEINLINE uint8_t *getColorData() { return static_cast<uint8_t*>(this->ptr() + ColorOffs); }
    FORCEINLINE raster::Tri *getTriData() { return IndexSize ? reinterpret_cast<raster::Tri*>(this->ptr() + IndexOffs) : NULL; }
    FORCEINLINE svec3 *getVertexData() { return reinterpret_cast<svec3*>(this->ptr() + VertexOffs); }
};

template<typename TMesh, bool clipping>
struct _MeshClipBuf
{
    FORCEINLINE       uint8_t *ptr()       { return NULL; }
    FORCEINLINE const uint8_t *ptr() const { return NULL; }
};

template<typename TMesh>
struct _MeshClipBuf<TMesh, true>
{
    uint8_t clipbuf[TMesh::Nverts];
    FORCEINLINE const uint8_t *ptr() const { return &clipbuf[0]; }
    FORCEINLINE       uint8_t *ptr()       { return &clipbuf[0]; }
};

template<typename TMesh, size_t N, bool usez>
struct _MeshZBuf
{
    FORCEINLINE       vtx::Zval *ptr()       { return NULL; }
    FORCEINLINE const vtx::Zval *ptr() const { return NULL; }
};

template<typename TMesh, size_t N>
struct _MeshZBuf<TMesh, N, true>
{
    vtx::Zval zbuf[N];
    FORCEINLINE const vtx::Zval *ptr() const { return &zbuf[0]; }
    FORCEINLINE       vtx::Zval *ptr()       { return &zbuf[0]; }
};

// TODO: pass initial transform to ctor so we don't have to do this per-frame?

enum DrawMeshFlags // bitmask
{
    DM_NONE = 0,
    DM_CLIP = 1,    // pass to clip during transform and filtering
    DM_STOREZ = 2,  // pass to record vertex z during transform and be able to sort
};

template<typename TMesh, unsigned dm = DM_NONE>
struct DrawMesh : public DecompMeshData<TMesh>
{
    typedef vtx::Zval Zval;
    typedef DecompMeshData<TMesh> Base;
    ivec2 projbuf[TMesh::Nverts]; // projected positions of vertices in screen space
    uint8_t filterlist[TMesh::Ntris]; // indexes of: frontface tris at the start, backface tris at the end

    // Only allocate clipping space if we're actually going to perform clipping. Saves 1 byte per vertex.
    _MeshClipBuf<TMesh, bool(!!(dm & DM_CLIP))> clipbuf;
    // Same for per-vertex Z buffer, another byte
    _MeshZBuf<TMesh, TMesh::Nverts, bool(!!(dm & DM_STOREZ))> zbufvert;

#ifdef _DEBUG
    DrawMesh()
    {
        printf("DrawMesh[tri:%u, vert:%u] size = %u\n", TMesh::Ntris, TMesh::Nverts, unsigned(sizeof(*this)));
    }
#endif
    
    // FIRST STEP
    // transform vertices with model-specific scaling factor, project into screen space
    template<typename M>
    FORCEINLINE void transform(const M& m, const ivec2& viewport = ivec2(0)) // FIXME: remove default viewport
    {
        mat4ds mds(TMesh::scale());
        vtx::transform_and_project(projbuf, clipbuf.ptr(), zbufvert.ptr(), this->getVertexData(), m * mds, TMesh::Nverts, viewport);
    }

    // this is ok to use after transform
    FORCEINLINE raster::AABB getAABB() const
    {
        return raster::getVertexListAABB(projbuf, TMesh::Nverts);
    }

    // SECOND STEP
    // Categorize triangles into front- or backface and does clipping.
    // Use after transform.
    FORCEINLINE raster::TriFilterResult filter()
    {
        return raster::filterTris(filterlist, projbuf, clipbuf.ptr(), this->getTriData(), TMesh::Ntris);
    }
    // then you can get those:
    FORCEINLINE uint8_t *frontFaceIdxs() { return &filterlist[0]; }
    FORCEINLINE uint8_t *backFaceIdxs(uint8_t nBack) { return &filterlist[Countof(filterlist) - nBack]; }
    // or, alternatively, no filter:
    // (this one can be run just once at init but does not do clipping)
    FORCEINLINE raster::TriFilterResult noFilter()
    {
        for(uint16_t i = 0; i < TMesh::Ntris; ++i)
            filterlist[i] = i;
        raster::TriFilterResult res;
        res.nBack = 0;
        res.nFront = TMesh::Ntris;
        return res;
    }

    FORCEINLINE raster::Tri getTri(const uint8_t *triidxs, uint16_t i) const
    {
        return this->getTriData()[triidxs[i]];
    }

    // OPTIONAL THIRD STEP
    FORCEINLINE void sortIndexZ(uint8_t *triidxs, uint16_t n)
    {
#ifdef _DEBUG
        assert(n < 256);
#endif
        raster::sortTrisZ(filterlist, (uint8_t)n, this->getTriData(), TMesh::Ntris, zbufvert.ptr());
    }

    template<typename LESS>
    FORCEINLINE void sortIndex(uint8_t *triidxs, uint16_t n, const LESS& cmp)
    {
#ifdef _DEBUG
        assert(n < 256);
#endif
        raster::sortTris(triidxs, (uint8_t)n, this->getTriData(), cmp);
    }

    // LAST STEP
    FORCEINLINE void draw(const uint8_t *triidxs, uint16_t n, const raster::Params& rp, uint8_t yoffs, uint8_t coloffs) const
    {
        render::ToLCD render(rp.rubmul, rp.glitchmul);
        raster::drawTriangles(render, triidxs, n, projbuf, this->getTriData(), this->getColorData(), zbufvert.ptr(), rp, yoffs, this->_shadelevels, this->_upshade, this->_downshade, coloffs);
    }

    FORCEINLINE void drawToTex(const uint8_t *triidxs, uint16_t n, const raster::Params& rp, uint8_t yoffs, uint8_t coloffs, uint8_t *tex, uint8_t w, uint8_t h) const
    {
        render::ToTex render(tex, w, h);
        raster::drawTriangles(render, triidxs, n, projbuf, this->getTriData(), this->getColorData(), zbufvert.ptr(), rp, yoffs, this->_shadelevels, this->_upshade, this->_downshade, coloffs);
    }

    FORCEINLINE void drawWireframe(const uint8_t *triidxs, uint16_t n, Color col) const
    {
        raster::drawWireframe(triidxs, n, col, this->getTriData(), projbuf);
    }

    FORCEINLINE void drawWireframeZ_DEBUG(const uint8_t *triidxs, uint16_t n)
    {
        for(uint16_t k = 0; k < n; ++k)
        {
            uint8_t ci = k * (256 / (n+1));
            Color cc = LCD::gencolor(ci,ci,ci);
            drawWireframe(triidxs + k, 1, cc);
        }
    }

    FORCEINLINE void drawZ_DEBUG(const uint8_t *triidxs, uint16_t n, const raster::Params& rp, uint8_t yoffs, uint8_t coloffs)
    {
        PalBackup bk;

        for(uint16_t k = 0; k < n; ++k)
        {
            uint8_t ci = k * (256 / (n+1));
            Color cc = LCD::gencolor(ci,ci,ci);
            for(unsigned i = 0; i < 256; ++i)
                palsetcolor(uint8_t(i), cc);
            draw(triidxs + k, 1, rp, yoffs, coloffs);
        }
    }
};
