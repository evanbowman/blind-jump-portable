#pragma once

#include "bulkAllocator.hpp"
#include "tileMap.hpp"
#include <optional>


//
// NOTE: This pathfinding code is only really suitable for level generation, and
// other infrequent operations. Some systems are simply not fast enough to run
// pathfinding in realtime, although you could perhaps cache a few generic paths
// between evenly distributed positions within a level...
//


using PathCoord = Vec2<u8>;


// NOTE: Perhaps we could make this even smaller... in practice, a path will
// never run through every possible coordinate in a level, and a level with no
// gaps whatsoever is unlikely to be generated in the first place...
//
// Also, we do not generate tiles for the borders of the map.
static constexpr const auto max_path =
    (TileMap::width - 2) * (TileMap::height - 2);


using PathBuffer = Buffer<PathCoord, max_path>;


// static_assert(sizeof(PathBuffer) + alignof(PathBuffer) <
//               sizeof(Platform::ScratchBuffer::data_));


struct PathData {
    using BackingMemory = ScratchBufferBulkAllocator;

    BackingMemory memory_;
    BackingMemory::Ptr<PathBuffer> path_;
};


std::optional<PathData> find_path(Platform& pfrm,
                                  TileMap& tiles,
                                  const PathCoord& start,
                                  const PathCoord& end);
