#pragma once

enum class GameState
{
    None,
    FrameHead,
    PreUpdate,
    Update,
    FixedUpdate,
    PostUpdate,
    PreDraw,
    Draw,
    Present,
    PostDraw,
    FrameTail,
};

// Operator overloads for GameState to use in for loops
inline GameState& operator++(GameState& state)
{
    state = static_cast<GameState>(static_cast<int>(state) + 1);
    return state;
}

inline GameState operator++(GameState& state, int)
{
    GameState old = state;
    ++state;
    return old;
}

inline bool operator<=(GameState lhs, GameState rhs)
{
    return static_cast<int>(lhs) <= static_cast<int>(rhs);
}

inline bool operator<(GameState lhs, GameState rhs)
{
    return static_cast<int>(lhs) < static_cast<int>(rhs);
}

inline bool operator>=(GameState lhs, GameState rhs)
{
    return static_cast<int>(lhs) >= static_cast<int>(rhs);
}

inline bool operator>(GameState lhs, GameState rhs)
{
    return static_cast<int>(lhs) > static_cast<int>(rhs);
}


static constexpr const char* GetStateName(GameState state)
{
    switch (state)
    {
    case GameState::None: return "None";
    case GameState::FrameHead: return "FrameHead";
    case GameState::PreUpdate: return "PreUpdate";
    case GameState::Update: return "Update";
    case GameState::FixedUpdate: return "FixedUpdate";
    case GameState::PostUpdate: return "PostUpdate";
    case GameState::PreDraw: return "PreDraw";
    case GameState::Draw: return "Draw";
    case GameState::Present: return "Present";
    case GameState::PostDraw: return "PostDraw";
    case GameState::FrameTail: return "FrameTail";
    default: return "Unknown";
    }
}
