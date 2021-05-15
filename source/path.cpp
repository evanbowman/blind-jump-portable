#include "path.hpp"
#include <algorithm>


static constexpr const auto result_scratch_buffers = 1;


IncrementalPathfinder::IncrementalPathfinder(Platform& pfrm,
                                             TileMap& tiles,
                                             const PathCoord& start,
                                             const PathCoord& end)
    : memory_(pfrm), priority_q_(allocate_dynamic<VertexBuf>(pfrm)),
      map_matrix_(allocate_dynamic<VertexMat>(pfrm)), end_(end)
{
    static_assert(sizeof(PathVertexData*) <= 8,
                  "What computer are you running this on?");

    if (not priority_q_) {
        pfrm.fatal("failed to alloc priority q");
    }

    if (not map_matrix_) {
        pfrm.fatal("failed to alloc map matrix");
        while (true)
            ;
    } else {
        for (int x = 0; x < TileMap::width - 1; ++x) {
            for (int y = 0; y < TileMap::height - 1; ++y) {
                (*map_matrix_)[x][y] = nullptr;
            }
        }
    }

    if (pfrm.scratch_buffers_remaining() <
        vertex_scratch_buffers + result_scratch_buffers + 1) {
        pfrm.fatal("cannot alloc vertex buffer");
    }

    bool error_state = false;


    tiles.for_each([&](u8& t, int x, int y) {
        if (error_state) {
            return;
        }

        if (is_walkable(t) and x < TileMap::width - 1 and
            y < TileMap::height - 1) {

            if (auto obj = memory_.alloc<PathVertexData>(pfrm)) {
                obj->coord_ = PathCoord{u8(x), u8(y)};
                static_assert(std::is_trivially_destructible<PathVertexData>());
                if (not priority_q_->push_back(obj.release())) {
                    error(pfrm, "not enough space in path node buffer");
                    error_state = true;
                } else {
                    (*map_matrix_)[x][y] = priority_q_->back();
                }
            } else {
                error_state = true;
            }
        }
    });

    if (error_state) {
        while (true)
            ;
    }

    auto start_v = [&]() -> PathVertexData* {
        for (auto& data : *priority_q_) {
            if (data->coord_ == start) {
                data->dist_ = 0;
                return data;
            }
        }
        return nullptr;
    }();

    if (not start_v) {
        pfrm.fatal("start node not in vertex set");
    }

    sort_q();
}


std::optional<DynamicMemory<PathBuffer>>
IncrementalPathfinder::compute(Platform& pfrm, int max_iters, bool* incomplete)
{
    for (int i = 0; i < max_iters; ++i) {
        if (not priority_q_->empty()) {
            auto min = priority_q_->back();
            // If the top remaining node in the priority queue is inf, then there
            // must be disconnected regions in the graph (right?).
            if (min->dist_ == std::numeric_limits<u16>::max()) {
                return {};
            }
            if (min->coord_ == end_) {
                auto path_mem = allocate_dynamic<PathBuffer>(pfrm);
                if (not path_mem) {
                    return {};
                }

                auto current_v = priority_q_->back();
                while (current_v) {
                    path_mem->push_back(current_v->coord_);
                    current_v = current_v->prev_;
                }
                *incomplete = false;
                return path_mem;
            }
            priority_q_->pop_back();

            for (auto& neighbor : neighbors(min)) {
                auto alt = min->dist_ +
                           manhattan_length(min->coord_, neighbor->coord_);
                if (alt < neighbor->dist_) {
                    neighbor->dist_ = alt;
                    neighbor->prev_ = min;
                }
            }
            sort_q();

        } else {
            *incomplete = false;
            return {};
        }
    }
    return {};
}


Buffer<IncrementalPathfinder::PathVertexData*, 4>
IncrementalPathfinder::neighbors(PathVertexData* data) const
{
    Buffer<PathVertexData*, 4> result;
    if (data->coord_.x > 0) {
        auto n = (*map_matrix_)[data->coord_.x - 1][data->coord_.y];
        if (n) {
            result.push_back(n);
        }
    }
    if (data->coord_.x <
        TileMap::width - 2) { // -2 b/c we don't include the last column
        auto n = (*map_matrix_)[data->coord_.x + 1][data->coord_.y];
        if (n) {
            result.push_back(n);
        }
    }
    if (data->coord_.y > 0) {
        auto n = (*map_matrix_)[data->coord_.x][data->coord_.y - 1];
        if (n) {
            result.push_back(n);
        }
    }
    if (data->coord_.y < TileMap::height - 2) {
        auto n = (*map_matrix_)[data->coord_.x][data->coord_.y + 1];
        if (n) {
            result.push_back(n);
        }
    }
    return result;
}


void IncrementalPathfinder::sort_q()
{
    std::sort(priority_q_->begin(),
              priority_q_->end(),
              [](auto& lhs, auto& rhs) { return lhs->dist_ > rhs->dist_; });
}


std::optional<DynamicMemory<PathBuffer>> find_path(Platform& pfrm,
                                                   TileMap& tiles,
                                                   const PathCoord& start,
                                                   const PathCoord& end)
{
    IncrementalPathfinder p(pfrm, tiles, start, end);

    bool incomplete = true;

    while (incomplete) {
        if (auto result = p.compute(pfrm, 5, &incomplete)) {
            return result;
        }
    }

    return {};
}
