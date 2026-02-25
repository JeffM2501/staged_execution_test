#pragma once

#include <cstdint>

enum class FrameStage : uint8_t
{
    None = 0,
    FrameHead,
    PreUpdate,
    FixedUpdate,
    Update,
    PostUpdate,
    PreDraw,
    Draw,
    Present,
    PostDraw,
    FrameTail,
    AutoNextState = 255
};

// Operator overloads for GameState to use in for loops
inline FrameStage& operator++(FrameStage& state)
{
    state = static_cast<FrameStage>(static_cast<int>(state) + 1);
    return state;
}

inline FrameStage operator++(FrameStage& state, int)
{
    FrameStage old = state;
    ++state;
    return old;
}

inline bool operator<=(FrameStage lhs, FrameStage rhs)
{
    return static_cast<int>(lhs) <= static_cast<int>(rhs);
}

inline bool operator<(FrameStage lhs, FrameStage rhs)
{
    return static_cast<int>(lhs) < static_cast<int>(rhs);
}

inline bool operator>=(FrameStage lhs, FrameStage rhs)
{
    return static_cast<int>(lhs) >= static_cast<int>(rhs);
}

inline bool operator>(FrameStage lhs, FrameStage rhs)
{
    return static_cast<int>(lhs) > static_cast<int>(rhs);
}

static constexpr const char* GetStageName(FrameStage state)
{
    switch (state)
    {
    case FrameStage::None: return "None";
    case FrameStage::FrameHead: return "FrameHead";
    case FrameStage::PreUpdate: return "PreUpdate";
    case FrameStage::Update: return "Update";
    case FrameStage::FixedUpdate: return "FixedUpdate";
    case FrameStage::PostUpdate: return "PostUpdate";
    case FrameStage::PreDraw: return "PreDraw";
    case FrameStage::Draw: return "Draw";
    case FrameStage::Present: return "Present";
    case FrameStage::PostDraw: return "PostDraw";
    case FrameStage::FrameTail: return "FrameTail";
    case FrameStage::AutoNextState: return "AutoNext";
    default: return "Unknown";
    }
}

static FrameStage GetNextStage(FrameStage state)
{
    switch (state)
    {
    case FrameStage::FrameHead: return FrameStage::PreUpdate;
    case FrameStage::PreUpdate: return FrameStage::Update;
    case FrameStage::Update: return FrameStage::PostUpdate;
    case FrameStage::FixedUpdate: return FrameStage::PostUpdate;
    case FrameStage::PostUpdate: return FrameStage::PreDraw;
    case FrameStage::PreDraw: return FrameStage::Draw;
    case FrameStage::Draw: return FrameStage::Present;
    case FrameStage::Present: return FrameStage::PostDraw;
    case FrameStage::PostDraw: return FrameStage::FrameTail;
    case FrameStage::FrameTail: return FrameStage::FrameHead;
    default: return FrameStage::None;
    }
}

