#pragma once

#include "bulkAllocator.hpp"
#include "tileMap.hpp"
#include <limits>
#include <optional>


//
// NOTE: This pathfinding code is only really suitable for level generation, and
// other infrequent operations. Some systems are simply not fast enough to run
// pathfinding in realtime, although you could perhaps cache a few generic paths
// between evenly distributed positions within a level...
//


// NOTE: Perhaps we could make this even smaller... in practice, a path will
// never run through every possible coordinate in a level, and a level with no
// gaps whatsoever is unlikely to be generated in the first place...
//
// Also, we do not generate tiles for the borders of the map.
static constexpr const auto max_path =
    (TileMap::width - 2) * (TileMap::height - 2) - 4;


using PathCoord = Vec2<u8>;
using PathBuffer = Buffer<PathCoord, max_path>;


std::optional<DynamicMemory<PathBuffer>> find_path(Platform& pfrm,
                                                   TileMap& tiles,
                                                   const PathCoord& start,
                                                   const PathCoord& end);


static constexpr const auto vertex_scratch_buffers = 2;


struct IncrementalPathfinder {
    IncrementalPathfinder(Platform& pfrm,
                          TileMap& tiles,
                          const PathCoord& start,
                          const PathCoord& end);

    std::optional<DynamicMemory<PathBuffer>>
    compute(Platform& pfrm, int max_iters, bool* incomplete);

private:
    struct PathVertexData {
        PathCoord coord_;
        u16 dist_ = std::numeric_limits<u16>::max();
        PathVertexData* prev_ = nullptr;
    };

    using VertexBuf = Buffer<PathVertexData*, max_path>;

    // note: does not include the last row and column of the map grid, in order
    // to save memory, and to make the matrix fit in a 1k allocation for the
    // GBA. Technically, we don't need the first row or column either, but
    // excluding them complicates indexing. We're using the vertex mat to speed
    // up neighbor calculation, otherwise, we have to scan through all of the
    // nodes in the priority q whenever we need to find which nodes are
    // neighbors. One could attach a list of neighbors to every vertex, but I
    // tried doing this, and you end up doing a bunch of unnecessary work (for
    // vertices which we will never visit), and using a lot more memory.
    using VertexMat =
        PathVertexData * [(TileMap::width - 1)][(TileMap::height - 1)];


    Buffer<PathVertexData*, 4> neighbors(PathVertexData* data) const;

    void sort_q();


    BulkAllocator<vertex_scratch_buffers> memory_;
    DynamicMemory<VertexBuf> priority_q_;
    DynamicMemory<VertexMat> map_matrix_;
    PathCoord end_;
};
