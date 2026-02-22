#include "TextureManager.h"

#include "GLFW/glfw3.h"

#include <unordered_map>
#include <deque>
#include <thread>
#include <mutex>

#include "ThreadedProcessor.h"


namespace TextureManager
{
    GLFWwindow* LoaderWindow = nullptr;
    Texture DefaultTexture = { 0 };
    std::thread LoaderThread;

    std::unordered_map<size_t, TextureReference> LoadedTextures;

    struct PendingTextureLoad
    {
        Texture GPUTexture = { 0 };
        size_t ID = 0;
        std::string ResourceFile; // todo, replace with a reference to the resource manager
    };

    ThreadedProcessor<PendingTextureLoad> TextureLoaderThread;

    void Init()
    {
        Image img = GenImageChecked(128, 128, 16, 16, RAYWHITE, LIGHTGRAY);
        DefaultTexture = LoadTextureFromImage(img);
        UnloadImage(img);

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);     // Window initially hidden
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);     // Window initially hidden
        LoaderWindow = glfwCreateWindow(1, 1, "TempWindow", NULL, glfwGetCurrentContext());
        glfwHideWindow(LoaderWindow);

        TextureLoaderThread.SetOnThreadStart([]() {
            glfwMakeContextCurrent(LoaderWindow);
            });

        TextureLoaderThread.SetOnThreadStop([]() {
            glfwDestroyWindow(LoaderWindow);
            });

        TextureLoaderThread.SetProcessorAndStart([](PendingTextureLoad pending) -> PendingTextureLoad {
            pending.GPUTexture = LoadTexture(pending.ResourceFile.c_str());
            glFlush();
            return pending;
        });
    }

    void Update()
    {
        PendingTextureLoad completed;
        while (TextureLoaderThread.PopCompleted(completed))
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
        TextureLoaderThread.Stop();

        for (auto& [id, texture] : LoadedTextures)
        {
            if (texture->Ready)
                UnloadTexture(texture->ID);
        
            texture->Ready = false;
        }

        PendingTextureLoad completed;
        while (TextureLoaderThread.PopCompleted(completed))
        {
            UnloadTexture(completed.GPUTexture);
        }

        LoadedTextures.clear();

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
        TextureLoaderThread.PushPending(pending);

        return ref;
    }
}