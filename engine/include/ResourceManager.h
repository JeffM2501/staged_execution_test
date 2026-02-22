
#pragma once
// ResourceManager.h
// Simple resource manager that loads Image, Music (Wave) and raw File data using a pool of ThreadedProcessor workers.
// - 4 concurrent loader threads
// - resources identified by size_t hash
// - returns std::shared_ptr<ResourceInfo> immediately; ResourceInfo::Ready indicates load completion
// - ResourceInfo keeps a use count; call Release() when done to allow automatic unload
// - caller may provide an optional callback invoked when resource finishes loading
//
// NOTE: This header depends on raylib types (Image, Wave). Include paths must make "raylib.h" available.

#include "ThreadedProcessor.h"
#include "raylib.h"

#include <memory>
#include <string>
#include <functional>
#include <variant>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace ResourceManager
{
    enum class ResourceType
    {
        Image,
        Music,
        File
    };

    // Forward declaration of ResourceInfo
    struct ResourceInfo;
    using ResourceInfoRef = std::shared_ptr<ResourceInfo>;
    using OnLoadedCb = std::function<void(const ResourceInfoRef&)>;

    // Resource data stored in memory (variant)
    using ResourceData = std::variant<std::monostate, Image, Wave, std::vector<unsigned char>>;

    // Resource information returned to callers
    struct ResourceInfo : public std::enable_shared_from_this<ResourceInfo>
    {
        size_t ID = 0;
        ResourceType Type = ResourceType::File;

        // Use count: increment when obtaining the shared pointer by other systems,
        // call Release() to signal the caller is done with the resource.
        std::atomic<int> UseCount{ 1 };

        // True when the resource has been loaded and Data is valid
        std::atomic<bool> Ready{ false };

        // Loaded data (valid only when Ready==true)
        ResourceData Data;

        // Multiple callbacks may be provided by callers; they will be invoked once the resource is loaded
        std::vector<OnLoadedCb> Callbacks;

        // Thread-safety for callbacks and Data mutation
        std::mutex Lock;

        // Public helpers
        void AddRef() { UseCount.fetch_add(1, std::memory_order_acq_rel); }

        // Decrement use count and request unload if it reaches zero.
        // After calling Release() the shared_ptr that called it may continue to exist,
        // but ResourceManager will unload memory once no users remain.
        void Release();

        // Convenience to query loaded state
        bool IsReady() const { return Ready.load(std::memory_order_acquire); }

        // Non-copyable/movable semantics for ResourceInfo (managed by shared_ptr)
        ResourceInfo() = default;
        ResourceInfo(const ResourceInfo&) = delete;
        ResourceInfo& operator=(const ResourceInfo&) = delete;
    };

    // Initialize ResourceManager and start loader pool (call once)
    void Init();

    // Shutdown loader pool and unload all resources (call on shutdown)
    void Shutdown();

    // Poll for completed loads and invoke callbacks (call from main thread regularly)
    void Update();

    // Load or obtain existing resource info by hash and type. Optional onLoaded callback is invoked when load completes.
    // Returns a shared_ptr<ResourceInfo> immediately. Caller should call Release() when finished with the resource.
    ResourceInfoRef LoadResource(size_t hash, ResourceType type, OnLoadedCb onLoaded = nullptr);

    // Internal: called by ResourceInfo::Release to free resource memory and remove from map when not used.
    void ReleaseResourceById(size_t id);
}