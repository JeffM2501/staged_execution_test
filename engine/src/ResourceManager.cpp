#include "ResourceManager.h"

#include <fstream>
#include <atomic>
#include <sstream>

#include <filesystem>
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>

using namespace ResourceManager;

namespace
{
    struct PendingLoad
    {
        size_t ID = 0;
        ResourceType Type = ResourceType::File;
        std::string Path;
        ResourceInfoRef Info; // pointer back to ResourceInfo so main thread can attach results
        ResourceData Data;    // filled by loader thread
    };

    // 4 loader threads
    static constexpr int LoaderThreadCount = 4;
    static std::vector<std::unique_ptr<ThreadedProcessor<PendingLoad>>> Loaders;
    static std::atomic<size_t> RoundRobinIndex{ 0 };

    // Map of active resources
    static std::unordered_map<size_t, ResourceInfoRef> Resources;
    static std::mutex ResourcesMutex;

    // Helper: compute resource file path from id + type (adjust as needed)
    static std::string BuildPath(size_t id, ResourceType type)
    {
        switch (type)
        {
        case ResourceType::Image:
            return TextFormat("resources/images/%zu.png", id);
        case ResourceType::Music:
            return TextFormat("resources/music/%zu.wav", id);
        case ResourceType::File:
        default:
            return TextFormat("resources/files/%zu.bin", id);
        }
    }

    // Helper: read arbitrary file into vector<unsigned char>
    static std::vector<unsigned char> ReadFileToVector(const std::string& path)
    {
        std::vector<unsigned char> out;
        std::ifstream ifs(path, std::ios::binary | std::ios::ate);
        if (!ifs) return out;
        std::ifstream::pos_type size = ifs.tellg();
        if (size <= 0) return out;
        out.resize(static_cast<size_t>(size));
        ifs.seekg(0);
        ifs.read(reinterpret_cast<char*>(out.data()), size);
        return out;
    }

    // Loader function executed inside loader threads
    static PendingLoad LoaderFunction(PendingLoad pending)
    {
        // Attempt to load based on type. Any failures leave Data as monostate.
        try
        {
            switch (pending.Type)
            {
            case ResourceType::Image:
            {
                // Load image (CPU memory)
                Image img = LoadImage(pending.Path.c_str());
                pending.Data = std::move(img);
                break;
            }
            case ResourceType::Music:
            {
                Wave w = LoadWave(pending.Path.c_str());
                pending.Data = std::move(w);
                break;
            }
            case ResourceType::File:
            default:
            {
                // Use std::ifstream to read raw bytes
                auto bytes = ReadFileToVector(pending.Path);
                pending.Data = std::move(bytes);
                break;
            }
            }
        }
        catch (...)
        {
            // swallow exceptions; leave pending.Data empty (monostate)
        }
        return pending;
    }

    // Unload resource data (must be called from main thread if raylib unloading functions used)
    static void UnloadResourceData(ResourceInfoRef& info)
    {
        std::lock_guard<std::mutex> lk(info->Lock);
        if (!info->Ready) return;

        std::visit([&](auto&& value)
        {
            using V = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<V, Image>)
            {
                UnloadImage(value);
            }
            else if constexpr (std::is_same_v<V, Wave>)
            {
                UnloadWave(value);
            }
            else if constexpr (std::is_same_v<V, std::vector<unsigned char>>)
            {
                value.clear();
                value.shrink_to_fit();
            }
            else
            {
                // monostate -> nothing
            }
        }, info->Data);

        info->Data = ResourceData{}; // reset
        info->Ready.store(false, std::memory_order_release);
    }
}

// Public API

void ResourceManager::Init()
{
    std::lock_guard<std::mutex> lk(ResourcesMutex);
    if (!Loaders.empty()) return; // already initialized

    Loaders.resize(LoaderThreadCount);
    for (int i = 0; i < LoaderThreadCount; ++i)
    {
        Loaders[i] = std::make_unique<ThreadedProcessor<PendingLoad>>();
        Loaders[i]->SetProcessorAndStart(&LoaderFunction);
    }
}

void ResourceManager::Shutdown()
{
    // Stop loaders
    for (auto& loader : Loaders)
    {
        if (loader) loader->Stop();
    }

    // Drain completed items so we can properly free memory
    Update();

    // Unload and clear resources
    std::lock_guard<std::mutex> lk(ResourcesMutex);
    for (auto& kv : Resources)
    {
        auto& info = kv.second;
        if (info)
        {
            UnloadResourceData(info);
        }
    }
    Resources.clear();

    Loaders.clear();
}

void ResourceManager::Update()
{
    // Poll completed queues from all loaders
    for (auto& loader : Loaders)
    {
        if (!loader) continue;
        PendingLoad completed;
        while (loader->PopCompleted(completed))
        {
            ResourceInfoRef info = completed.Info;
            if (!info) continue;

            {
                std::lock_guard<std::mutex> lk(info->Lock);
                // Move loaded data into ResourceInfo
                info->Data = std::move(completed.Data);
                info->Ready.store(true, std::memory_order_release);
            }

            // Invoke callbacks outside lock
            std::vector<OnLoadedCb> callbacks;
            {
                std::lock_guard<std::mutex> lk(info->Lock);
                callbacks.swap(info->Callbacks); // move callbacks out
            }

            for (auto& cb : callbacks)
            {
                if (cb) cb(info);
            }
        }
    }
}

ResourceInfoRef ResourceManager::LoadResource(size_t hash, ResourceType type, OnLoadedCb onLoaded)
{
    {
        std::lock_guard<std::mutex> lk(ResourcesMutex);
        auto it = Resources.find(hash);
        if (it != Resources.end())
        {
            auto info = it->second;
            // increment use count and attach callback if requested
            info->AddRef();
            if (onLoaded)
            {
                // if already ready, call immediately (call outside of ResourcesMutex to avoid deadlock)
                bool ready = info->IsReady();
                if (ready)
                {
                    // call outside lock
                   // lk.unlock() // no-op placeholder to indicate careful ordering (we will call outside)
                }
                // attach callback under info lock
                std::lock_guard<std::mutex> lk2(info->Lock);
                info->Callbacks.push_back(onLoaded);
            }
            // If already ready and callback was provided, call it now (outside locks)
            if (onLoaded && info->IsReady())
            {
                onLoaded(info);
            }
            return info;
        }
    }

    // Create a new ResourceInfo
    auto info = std::make_shared<ResourceInfo>();
    info->ID = hash;
    info->Type = type;
    info->Ready.store(false);
    info->UseCount.store(1);

    if (onLoaded)
    {
        std::lock_guard<std::mutex> lk(info->Lock);
        info->Callbacks.push_back(onLoaded);
    }

    {
        std::lock_guard<std::mutex> lk(ResourcesMutex);
        Resources.emplace(hash, info);
    }

    // Build pending load and assign to a loader using round-robin
    PendingLoad pending;
    pending.ID = hash;
    pending.Type = type;
    pending.Path = BuildPath(hash, type);
    pending.Info = info;

    size_t idx = RoundRobinIndex.fetch_add(1, std::memory_order_relaxed);
    idx %= Loaders.size();
    Loaders[idx]->PushPending(std::move(pending));

    return info;
}

void ResourceManager::ReleaseResourceById(size_t id)
{
    ResourceInfoRef info;
    {
        std::lock_guard<std::mutex> lk(ResourcesMutex);
        auto it = Resources.find(id);
        if (it == Resources.end()) return;
        info = it->second;
    }

    // If still has users, nothing to do
    int cur = info->UseCount.load(std::memory_order_acquire);
    if (cur > 0) return;

    // Unload memory on main thread
    UnloadResourceData(info);

    // Remove from map
    {
        std::lock_guard<std::mutex> lk(ResourcesMutex);
        // Only erase if the entry still points to the same info and UseCount == 0 and not Ready (we unloaded)
        auto it = Resources.find(id);
        if (it != Resources.end() && it->second == info)
        {
            Resources.erase(it);
        }
    }
}

// ResourceInfo::Release implementation
void ResourceInfo::Release()
{
    int prev = UseCount.fetch_sub(1, std::memory_order_acq_rel);
    if (prev <= 1)
    {
        // no more users -> request unload
        ResourceManager::ReleaseResourceById(ID);
    }
}




namespace UnitTest
{

    using namespace ResourceManager;
    namespace fs = std::filesystem;

    int ResourceUnitTest()
    {
        // Prepare test environment
        const size_t testId = 12345;
        const std::string baseDir = "resources";
        const std::string filesDir = baseDir + "/files";
        const std::string testFile = filesDir + "/" + std::to_string(testId) + ".bin";

        // Ensure directories exist
        fs::create_directories(filesDir);

        // Write test file with known contents
        const std::string payload = "HelloResourceManager";
        {
            std::ofstream ofs(testFile, std::ios::binary);
            if (!ofs)
            {
                std::cerr << "Failed to create test file: " << testFile << "\n";
                return 1;
            }
            ofs.write(payload.data(), static_cast<std::streamsize>(payload.size()));
        }

        // Initialize ResourceManager (starts loader threads)
        ResourceManager::Init();

        bool callbackInvoked = false;
        ResourceInfoRef firstInfo;

        // Request load with callback
        firstInfo = ResourceManager::LoadResource(testId, ResourceType::File,
            [&](const ResourceInfoRef& info)
            {
                callbackInvoked = true;
                // Ensure the info pointer matches the one returned to us (it should)
                assert(info == firstInfo);
            });

        // Poll Update() until callback invoked or timeout
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (!callbackInvoked && std::chrono::steady_clock::now() < deadline)
        {
            ResourceManager::Update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (!callbackInvoked)
        {
            std::cerr << "Timeout waiting for resource to load.\n";
            ResourceManager::Shutdown();
            return 2;
        }

        // Verify resource marked ready and data content matches
        assert(firstInfo->IsReady() && "Resource should be ready after load");
        // Data should be a vector<unsigned char> for File resources
        bool matched = false;
        {
            std::lock_guard<std::mutex> lk(firstInfo->Lock);
            if (auto vec = std::get_if<std::vector<unsigned char>>(&firstInfo->Data))
            {
                std::string loaded(vec->begin(), vec->end());
                matched = (loaded == payload);
            }
        }
        assert(matched && "Loaded file contents must match written payload");

        // Test Release() causes resource to be removed and a subsequent LoadResource returns a different ResourceInfoRef
        firstInfo->Release();

        // Load again - should create a fresh ResourceInfo (different shared_ptr)
        ResourceInfoRef secondInfo = ResourceManager::LoadResource(testId, ResourceType::File);
        assert(secondInfo && "Second load returned valid ResourceInfoRef");
        assert(secondInfo != firstInfo && "After Release the resource should have been removed; new LoadResource should return a different info object");

        // Clean up: release second and shutdown
        secondInfo->Release();

        // Drain any remaining completed loads and shutdown
        for (int i = 0; i < 10; ++i) // a few updates to give loaders time if needed
        {
            ResourceManager::Update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        ResourceManager::Shutdown();

        // Remove test files (best-effort)
        std::error_code ec;
        fs::remove(testFile, ec);
        fs::remove(filesDir, ec);
        fs::remove(baseDir, ec);

        std::cout << "ResourceManager unit test passed.\n";
        return 0;
    }
}