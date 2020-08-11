#include "path.hpp"
#include <algorithm>
#include <iostream>


struct PathVertexData {
    PathCoord coord_;
    Float dist_ = std::numeric_limits<Float>::infinity();
    PathVertexData* prev_ = nullptr;
};


std::optional<PathData> find_path(Platform& pfrm,
                                  TileMap& tiles,
                                  const PathCoord& start,
                                  const PathCoord& end)
{
    static_assert(sizeof(PathVertexData*) <= 8,
                  "What computer are you running this on?");

    // Technically, we don't always use all six buffers. On embedded
    // microprocessors, or on 32 bit systems, pointers are half as large, so
    // PathVertexData doesn't take up as much space...
    constexpr auto vertex_scratch_buffers = 6;
    constexpr auto result_scratch_buffers = 1;

    if (pfrm.scratch_buffers_remaining() <
        vertex_scratch_buffers + result_scratch_buffers) {
        error(pfrm, "not enough memory to compute path...");
        return {};
    }

    Buffer<ScratchBufferBulkAllocator, vertex_scratch_buffers> regions;
    regions.emplace_back(pfrm);

    // NOTE: our game does not use the edge tiles of the map, so we can save
    // some space...
    using VertexBuf =
        Buffer<PathVertexData*, (TileMap::width - 2) * (TileMap::height - 2)>;
    VertexBuf priority_q;
    VertexBuf all_vertices;

    bool error_state = false;

    tiles.for_each([&](Tile& t, int x, int y) {
        if (error_state)
            return;
        if (is_walkable__precise(t)) {
        RETRY:
            if (auto obj = regions.back().alloc<PathVertexData>()) {
                obj->coord_ = PathCoord{u8(x), u8(y)};
                static_assert(std::is_trivially_destructible<PathVertexData>());
                if (not all_vertices.push_back(obj.release())) {
                    error(pfrm, "not enough space in path node buffer");
                    error_state = true;
                }
            } else {
                if (regions.full()) {
                    error(pfrm, "out of memory when generating path!");
                    error_state = true;
                } else {
                    regions.emplace_back(pfrm);
                    goto RETRY;
                }
            }
        }
    });

    if (error_state) {
        return {};
    }

    std::copy(all_vertices.begin(),
              all_vertices.end(),
              std::back_inserter(priority_q));

    auto start_v = [&]() -> PathVertexData* {
        for (auto& data : priority_q) {
            if (data->coord_ == start) {
                data->dist_ = 0;
                return data;
            }
        }
        return nullptr;
    }();

    if (not start_v) {
        error(pfrm, "start node does not exist in vertex set");
        return {};
    }

    auto sort_q = [&] {
        std::sort(priority_q.begin(),
                  priority_q.end(),
                  [](auto& lhs, auto& rhs) { return lhs->dist_ > rhs->dist_; });
    };

    sort_q();

    // The implementation of this function is a serious bottleneck. But we're
    // working with limited memory here, and our pathfinding function must
    // sacrifice cpu cycles for memory usage.
    auto neighbors = [&](PathVertexData* data) -> Buffer<PathVertexData*, 4> {
        Buffer<PathVertexData*, 4> result;
        for (auto& elem : all_vertices) {
            if (elem->coord_.x == data->coord_.x) {
                if (elem->coord_.y not_eq data->coord_.y and
                    abs(elem->coord_.y - data->coord_.y) < 2) {
                    result.push_back(elem);
                }
            } else if (elem->coord_.y == data->coord_.y) {
                if (abs(elem->coord_.x - data->coord_.x) < 2) {
                    result.push_back(elem);
                }
            }
        }
        return result;
    };

    while (not priority_q.empty()) {
        auto min = priority_q.back();
        // If the top remaining node in the priority queue is inf, then there
        // must be disconnected regions in the graph (right?).
        if (min->dist_ == std::numeric_limits<Float>::infinity()) {
            return {};
        }
        if (min->coord_ == end) {
            ScratchBufferBulkAllocator path_mem(pfrm);
            auto path = path_mem.alloc<PathBuffer>();
            if (not path) {
                return {};
            }

            auto current_v = priority_q.back();
            while (current_v) {
                path->push_back(current_v->coord_);
                current_v = current_v->prev_;
            }

            return {PathData{path_mem, std::move(path)}};
        }
        priority_q.pop_back();

        for (auto& neighbor : neighbors(min)) {
            auto alt = min->dist_ + distance(min->coord_.cast<Float>(),
                                             neighbor->coord_.cast<Float>());
            if (alt < neighbor->dist_) {
                neighbor->dist_ = alt;
                neighbor->prev_ = min;
            }
        }
        sort_q();
    }

    return {};
}
