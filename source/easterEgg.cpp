#include "platform/platform.hpp"
#include <tuple>


using NoteInfo = std::tuple<Note, int, Octave>;

static const NoteInfo B0 {Note::B, 2, -1};
static const NoteInfo AS1 {Note::AS, 2, 0};
static const NoteInfo B1 {Note::B, 2, 0};
static const NoteInfo B1Fast {Note::B, 1, 0};
static const NoteInfo C1 {Note::C, 2, 0};
static const NoteInfo C2 {Note::C, 2, 1};
static const NoteInfo C2Fast {Note::C, 1, 1};
static const NoteInfo A1 {Note::A, 2, 0};
static const NoteInfo D1 {Note::D, 2, 0};
static const NoteInfo D2 {Note::D, 2, 1};
static const NoteInfo D2Fast {Note::D, 1, 1};
static const NoteInfo E1 {Note::E, 2, 0};
static const NoteInfo E2 {Note::E, 2, 1};
static const NoteInfo E2Fast {Note::E, 1, 1};
static const NoteInfo F2 {Note::F, 2, 1};
static const NoteInfo F2Fast {Note::F, 1, 1};
static const NoteInfo G2 {Note::G, 2, 1};
static const NoteInfo F1 {Note::F, 2, 0};
static const NoteInfo GS2 {Note::GS, 2, 1};
static const NoteInfo G2Fast {Note::G, 1, 1};
static const NoteInfo A2 {Note::A, 2, 1};


// My plug in baby Crucifies my enemies...
static const NoteInfo* guitar_riff[] = {
     // clang-format off
     &AS1, &B1, &C2, &A1, &B1, &C2, &D2, &B1,

     &C2, &D2, &E2, &F2Fast, &G2Fast, &F2, &E2, &D2, &C2,

     &B1, &C2, &D2, &B1, &C2, &D2, &E2, &C2,

     &D2, &E2, &F2, &F2Fast, &G2Fast, &F2, &E2, &F2, &E2,

     &F2, &D2, &A1, &F1, &A1, &D2, &F2, &GS2,

     &A2, &GS2, &F2, &E2, &F2, &D2, &C2, &D2,

     &AS1, &B1, &C2, &A1, &B1, &C2, &D2, &B1,

     &C2, &D2, &E2, &F2Fast, &G2Fast, &F2, &E2, &D2, &C2,

     &B1, &C2, &D2, &B1, &C2, &D2, &E2, &C2,

     &D2, &E2, &F2, &F2Fast, &G2Fast, &F2, &E2, &F2, &E2,

     &F2, &D2, &A1, &F1, &A1, &D2, &F2, &GS2,

     &A2, &GS2, &F2, &E2, &F2, &D2, &C2, &D2,

     &AS1, &B1, &C2, &A1, &B1, &C2, &D2, &B1,

     &C2, &D2, &E2, &F2Fast, &G2Fast, &F2, &E2, &D2, &C2,
     // clang-format on
};


static const NoteInfo* base_line[] =
{
     // clang-format off
     0, 0, 0, 0, 0, 0, 0, 0,

     0, 0, 0, 0, 0, 0, 0, 0,

     0, 0, 0, 0, 0, 0, 0, 0,

     0, 0, 0, 0, 0, 0, 0, 0,

     &B0, 0, &B1Fast, &B1Fast, &B0, &B0, &B0, &B1, &B0,

     &B0, 0, &B1Fast, &B1Fast, &B0, &B0, &B0, &B1, &B0,

     &D1, 0, &D2Fast, &D2Fast, &D1, &D1, &D1, &D2, &D1,

     &D1, 0, &D2Fast, &D2Fast, &D1, &D1, &D1, &D2, &D1,

     &E1, 0, &E2Fast, &E2Fast, &E1, &E1, &E1, &E2, &E1,

     &E1, 0, &E2Fast, &E2Fast, &E1, &E1, &E1, &E2, &E1,

     &B0, 0, &B1Fast, &B1Fast, &B0, &B0, &B0, &B1, &B0,

     &B0, 0, &B1Fast, &B1Fast, &B0, &B0, &B0, &B1, &B0,

     &D1, 0, &D2Fast, &D2Fast, &D1, &D1, &D1, &D2, &D1,

     &D1, 0, &D2Fast, &D2Fast, &D1, &D1, &D1, &D2, &D1,
     // clang-format on
};


static Microseconds riff_timer = 0;
static Microseconds baseline_timer = 0;

static size_t riff_pos = 0;
static size_t baseline_pos = 0;


void riff(Platform& pf)
{
    constexpr Microseconds time_step = 16667;

    if (riff_pos < sizeof(guitar_riff) / sizeof(NoteInfo*) and
        baseline_pos < sizeof(base_line) / sizeof(NoteInfo*)) {

        if (riff_timer) {
            riff_timer -= time_step;
        } else {
            auto note = guitar_riff[riff_pos++];
            if (not note) {
                riff_timer = time_step * 5 * 2;
            } else {
                pf.speaker().play(std::get<0>(*note), 4 + std::get<2>(*note), 0);
                riff_timer = time_step * 5 * std::get<1>(*note);
            }
        }

        if (baseline_timer) {
            baseline_timer -= time_step;
        } else {
            auto note = base_line[baseline_pos++];
            if (not note) {
                baseline_timer = time_step * 5 * 2;
            } else {
                pf.speaker().play(std::get<0>(*note), 3 + std::get<2>(*note), 1);
                baseline_timer = time_step * 5 * std::get<1>(*note);
            }
        }
    } else {
        riff_pos = 0;
        baseline_pos = 0;
    }
}
