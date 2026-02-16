# TaskManager System

## Overview

The **TaskManager** system provides a flexible, state-driven, and multi-threaded task scheduling framework for games and real-time applications. It enables you to define tasks that execute at specific stages of the game loop, manage dependencies, and leverage parallelism with a thread pool.

---

## Features

- **State-based execution:** Tasks specify which game state they depend on and which state they block until completion.
- **Dependency management:** Tasks can have child tasks that must also complete.
- **Thread pool support:** Non-main-thread tasks are distributed across worker threads for parallel execution.
- **Main-thread safety:** Tasks that require main-thread execution (e.g., rendering, input) are supported.
- **Performance tracking:** (In debug builds) Tracks execution and blocking times per state.

---

## Key Concepts

### Game States

The game loop is divided into a sequence of states (e.g., `FrameHead`, `PreUpdate`, `Update`, `FixedUpdate`, `Draw`, `FrameTail`). Each task specifies:
- `DependsOnState`: The state at which it should start.
- `BlocksState`: The state that cannot advance until the task is complete.

### Tasks

A **Task** is a unit of work. Each task:
- Inherits from the `Task` base class.
- Implements the `Tick()` method for its logic.
- Can specify dependencies (child tasks).
- Can be set to run on the main thread or a worker thread.

### Thread Pool

- The system creates a pool of worker threads (one per CPU core).
- Tasks not requiring the main thread are distributed to these threads.
- Each thread manages its own queue of tasks.

### Blocking and Advancement

- States only advance when all tasks blocking that state are complete.
- The system checks for blocking tasks before moving to the next state.
