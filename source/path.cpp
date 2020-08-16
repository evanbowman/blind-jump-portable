#include "path.hpp"
#include <algorithm>


static constexpr const auto result_scratch_buffers = 1;


IncrementalPathfinder::IncrementalPathfinder(Platform& pfrm,
                                             TileMap& tiles,
                                             const PathCoord& start,
                                             const PathCoord& end)
    : memory_(pfrm), priority_q_(allocate_dynamic<VertexBuf>(pfrm)), end_(end)
{
    static_assert(sizeof(PathVertexData*) <= 8,
                  "What computer are you running this on?");


    if (not priority_q_.obj_) {
        error(pfrm,
              "failed to alloc priority q (does not fit in scratch buffer");
        while (true)
            ;
    }

    if (pfrm.scratch_buffers_remaining() <
        vertex_scratch_buffers + result_scratch_buffers) {
        error(pfrm, "not enough memory to compute path...");
        while (true)
            ;
    }

    bool error_state = false;


    tiles.for_each([&](Tile& t, int x, int y) {
        if (error_state)
            return;
        if (is_walkable__precise(t)) {
            if (auto obj = memory_.alloc<PathVertexData>(pfrm)) {
                obj->coord_ = PathCoord{u8(x), u8(y)};
                static_assert(std::is_trivially_destructible<PathVertexData>());
                if (not priority_q_.obj_->push_back(obj.release())) {
                    error(pfrm, "not enough space in path node buffer");
                    error_state = true;
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
        for (auto& data : *priority_q_.obj_) {
            if (data->coord_ == start) {
                data->dist_ = 0;
                return data;
            }
        }
        return nullptr;
    }();

    if (not start_v) {
        error(pfrm, "start node does not exist in vertex set");
        while (true)
            ;
    }

    sort_q();
}


std::optional<PathData>
IncrementalPathfinder::compute(Platform& pfrm, int max_iters, bool* incomplete)
{
    for (int i = 0; i < max_iters; ++i) {
        if (not priority_q_.obj_->empty()) {
            auto min = priority_q_.obj_->back();
            // If the top remaining node in the priority queue is inf, then there
            // must be disconnected regions in the graph (right?).
            if (min->dist_ == std::numeric_limits<u16>::max()) {
                return {};
            }
            if (min->coord_ == end_) {
                ScratchBufferBulkAllocator path_mem(pfrm);
                auto path = path_mem.alloc<PathBuffer>();
                if (not path) {
                    return {};
                }

                auto current_v = priority_q_.obj_->back();
                while (current_v) {
                    path->push_back(current_v->coord_);
                    current_v = current_v->prev_;
                }
                *incomplete = false;
                return {PathData{path_mem, std::move(path)}};
            }
            priority_q_.obj_->pop_back();

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
    for (auto& elem : *priority_q_.obj_) {
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
}


void IncrementalPathfinder::sort_q()
{
    std::sort(priority_q_.obj_->begin(),
              priority_q_.obj_->end(),
              [](auto& lhs, auto& rhs) { return lhs->dist_ > rhs->dist_; });
}


std::optional<PathData> find_path(Platform& pfrm,
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
