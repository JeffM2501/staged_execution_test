# Project Overview

This project is a modular game framework designed for real-time applications, featuring:

- **TaskManager System**: A flexible, state-driven, multi-threaded task scheduler for managing game loop stages, dependencies, and parallel execution.
- **Entity System**: A high-performance, component-based architecture for managing game entities and their data, supporting efficient queries and parallel processing.
- **Presentation Manager**: An interface for layered rendering, enabling advanced visual composition and effects, with support for custom shaders and flexible layer management.

The framework is built for extensibility, performance, and ease of integration, making it suitable for games and interactive simulations.

---

# TaskManager System

## Overview

The **TaskManager** system provides a flexible, state-driven, and multi-threaded task scheduling framework for games and real-time applications. It enables you to define tasks that execute at specific stages of the game loop, manage dependencies, and leverage parallelism with a thread pool.

## Features

- **State-based execution:** Tasks specify which game state they depend on and which state they block until completion.
- **Dependency management:** Tasks can have child tasks that must also complete.
- **Thread pool support:** Non-main-thread tasks are distributed across worker threads for parallel execution.
- **Main-thread safety:** Tasks that require main-thread execution (e.g., rendering, input) are supported.
- **Performance tracking:** (In debug builds) Tracks execution and blocking times per state.

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

---

# Presentation Manager

The PresentationManager namespace provides an interface for managing layered rendering in a graphics engine, likely using Raylib. It allows for the creation, manipulation, and presentation of multiple rendering layers, each with customizable properties such as alpha, tint, offset, and shaders. The main features include:

- **Initialization and Cleanup**: Init() and Cleanup() handle setup and teardown of the presentation system.
- **Layer Management**:
  - DefineLayer() creates a new layer with options for flipping and scaling.
  - ReleaseLayer() removes a layer.
  - BeginLayer() and EndLayer() mark the start and end of drawing to a specific layer.
- **Layer Properties**:
  - SetLayerAlpha(), SetLayerTint(), and SetLayerOffset() adjust visual properties of layers.
  - SetLayerShader() and ClearLayerShader() manage custom shaders for layers.
- **Presentation**:
  - Present() displays all layers according to their settings.
  - GetCurrentLayerRect() retrieves the rectangle of the currently active layer.

This manager abstracts the complexity of multi-layer rendering, enabling flexible composition and effects for game or application visuals.

---

# Entity System

## Overview

The **Entity System** provides a robust, high-performance framework for managing game entities and their components. Designed for modularity and scalability, it enables flexible data-driven development and efficient runtime operations.

## Features

- **Component-Based Architecture**: Entities are lightweight IDs; all data and behavior are defined through components.
- **Type-Safe and Dynamic APIs**: Access components using C++ templates or runtime type IDs.
- **Efficient Storage**: Each component type is stored in a dedicated table for fast lookup, addition, and removal.
- **Parallel Processing**: Iteration over components supports parallel execution for high-performance scenarios.
- **Automatic Registration**: Components are registered at startup, enabling dynamic extension and modularity.

## How It Works

- **Entities**: Represented by unique integer IDs. Entities themselves hold no data; all information is attached via components.
- **Components**: Each component type inherits from a common base and is registered with the system. Components are stored in contiguous memory for cache efficiency.
- **Component Tables**: Each component type has its own table, mapping entity IDs to component instances. Fast lookup and removal are achieved using a combination of vectors and hash maps.

## API Example

```cpp
// Register a component type
EntitySystem::RegisterComponent<MyComponent>();

// Create a new entity
size_t entityId = EntitySystem::NewEntityId();

// Add a component to an entity
auto* comp = EntitySystem::AddComponent<MyComponent>(entityId, ...);

// Query for a component
if (EntitySystem::EntityHasComponent<MyComponent>(entityId)) {
  auto* comp = EntitySystem::GetEntityComponent<MyComponent>(entityId);
}

// Iterate over all components of a type (optionally in parallel)
EntitySystem::DoForEachComponent<MyComponent>([](MyComponent& c) {
  // ...process component...
}, parallel=true);
```

## Design Highlights

- **Hash-Based Type Identification**: Uses CRC64 hashes for fast, collision-resistant component type IDs.
- **Swap-and-Pop Removal**: Efficiently removes components without leaving gaps in memory.
- **Thread Safety**: Parallel iteration is supported, but user code must ensure thread safety when accessing shared data.

---

The Entity System is ideal for games and simulations requiring modular, high-performance data management.