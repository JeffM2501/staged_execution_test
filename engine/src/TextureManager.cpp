#include "TextureManager.h"

#include "GLFW/glfw3.h"

#include "CRC64.h"
#include <unordered_map>
#include <deque>
#include <thread>
#include <mutex>


namespace TextureManager
{
    GLFWwindow* LoaderWindow = nullptr;
    std::thread LoaderThread;

    std::condition_variable Trigger;
    std::mutex  Lock;

    std::atomic_bool Running = true;

    std::unordered_map<size_t, TextureReference> LoadedTextures;

    struct PendingTextureLoad
    {
        Texture GPUTexture = { 0 };
        size_t ID = 0;
        std::string ResourceFile; // todo, replace with a reference to the resource manager
    };

    std::mutex PendingLock;
    std::mutex CompletedLock;
    std::deque<PendingTextureLoad> PendingLoads;
    std::deque<PendingTextureLoad> CompletedLoads;

    Texture DefaultTexture = { 0 };

    bool HasPending()
    {
        std::lock_guard<std::mutex> lock(PendingLock);
        return !PendingLoads.empty();
    }

    PendingTextureLoad PopPending()
    {
        std::lock_guard<std::mutex> lock(PendingLock);

        auto pending = PendingLoads.front();
        PendingLoads.pop_front();
        return pending;
    }

    void PushPending(PendingTextureLoad pending)
    {
        {
            std::lock_guard<std::mutex> lock(Lock);
            PendingLoads.push_back(pending);
        }
        Trigger.notify_one();
    }

    PendingTextureLoad PopCompleted()
    {
        std::lock_guard<std::mutex> lock(CompletedLock);

        auto completed = CompletedLoads.front();
        CompletedLoads.pop_front();
        return completed;
    }

    void PushCompleted(PendingTextureLoad compelted)
    {
        std::lock_guard<std::mutex> lock(CompletedLock);
        CompletedLoads.push_back(compelted);
    }

    void LoadTextureInThread()
    {
        if (!LoaderWindow)
            return;

        glfwMakeContextCurrent(LoaderWindow);

        while (true)
        {
            std::unique_lock<std::mutex> lock(Lock);
            Trigger.wait(lock, []() { return !Running || !PendingLoads.empty(); });
            if (!Running && PendingLoads.empty())
                break;

            auto pending = PopPending();

            pending.GPUTexture = LoadTexture(pending.ResourceFile.c_str());
            glFlush();
            PushCompleted(pending);
        }

        glfwDestroyWindow(LoaderWindow);
    }

    void Init()
    {
        Image img = GenImageChecked(128, 128, 16, 16, RAYWHITE, LIGHTGRAY);
        DefaultTexture = LoadTextureFromImage(img);
        UnloadImage(img);

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);     // Window initially hidden
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);     // Window initially hidden
        LoaderWindow = glfwCreateWindow(1, 1, "TempWindow", NULL, glfwGetCurrentContext());
        glfwHideWindow(LoaderWindow);
        LoaderThread = std::thread(LoadTextureInThread);
    }

    void Update()
    {
        std::lock_guard<std::mutex> lock(CompletedLock);
        for (auto& completed : CompletedLoads)
        {
            auto texture = LoadedTextures.find(completed.ID);
            if (texture == LoadedTextures.end())
            {
                UnloadTexture(completed.GPUTexture);
                TraceLog(LOG_ERROR, "Texture ID %zu loaded but not found", completed.ID);
                continue;
            }
 
            texture->second->ID = completed.GPUTexture;
            texture->second->Ready = true;
            texture->second->Bounds = Rectangle{ 0,0, float(completed.GPUTexture.width), float(completed.GPUTexture.height) };
        }
    }

    void Shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(Lock);
            Running.store(false);
        }
        Trigger.notify_all();

        if (LoaderThread.joinable())
            LoaderThread.join();

        for (auto& [id, texture] : LoadedTextures)
        {
            if (texture->Ready)
                UnloadTexture(texture->ID);

            texture->Ready = false;
        }

        LoadedTextures.clear();

        for (auto& completed : CompletedLoads)
            UnloadTexture(completed.GPUTexture);

        CompletedLoads.clear();
        PendingLoads.clear();

        UnloadTexture(DefaultTexture);
    }

    TextureReference GetTexture(size_t hash)
    {
        auto texture = LoadedTextures.find(hash);

        if (texture != LoadedTextures.end())
            return texture->second;

        TextureReference ref = std::make_shared<TextureInfo>();
        ref->ID = DefaultTexture;
        ref->Bounds = Rectangle{ 0,0, float(DefaultTexture.width), float(DefaultTexture.height) };
        ref->Ready = false;

        PendingTextureLoad pending;
        pending.ID = hash;
        pending.ResourceFile = TextFormat("resources/textures/%zu.png", hash);

        LoadedTextures.insert_or_assign(hash, ref);
        PushPending(pending);

        return ref;
    }
}