// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


#include <stdint.h>
#ifdef _WIN32
// Windows / D3D11
#include <d3d11.h>
#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include "IUnityGraphicsD3D11.h"
#include <vector>
static IUnityInterfaces* s_UnityInterfaces = nullptr;
static IUnityGraphicsD3D11* s_D3D11 = nullptr;
static ID3D11Device* g_Device = nullptr;
extern "C" {



    UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
    {
        s_UnityInterfaces = unityInterfaces;
        IUnityGraphics* graphics = s_UnityInterfaces->Get<IUnityGraphics>();
        s_D3D11 = s_UnityInterfaces->Get<IUnityGraphicsD3D11>();

        g_Device = s_D3D11->GetDevice();
    }

    UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginUnload() {}

    
    // Upload texture to GPU
    __declspec(dllexport)
        ID3D11Texture2D* UploadTextureToGPU(const uint8_t* pixelData, int width, int height)
    {
        if (!pixelData || width <= 0 || height <= 0 || !g_Device)
            return nullptr;
    
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = pixelData;
        initData.SysMemPitch = width * 4;
    
        ID3D11Texture2D* texture = nullptr;
        g_Device->CreateTexture2D(&desc, &initData, &texture);
        return texture;
    }

    __declspec(dllexport)
        void* UploadToGPU_RGB(const uint8_t* rgbData, int width, int height)
    {
        if (!rgbData || !g_Device) return nullptr;

        static std::vector<uint8_t> rgbaData;
        if (rgbaData.size() < width * height * 4)
            rgbaData.resize(width * height * 4);

        uint32_t* dst = reinterpret_cast<uint32_t*>(rgbaData.data());
        const uint8_t* src = rgbData;
        int pixelCount = width * height;

        for (int i = 0; i < pixelCount; ++i)
        {
            dst[i] = 0xFF000000 | (src[0] << 16) | (src[1] << 8) | src[2];
            src += 3;
        }

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.SampleDesc.Count = 1;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = rgbaData.data();
        initData.SysMemPitch = width * 4;

        ID3D11Texture2D* texture = nullptr;
        if (FAILED(g_Device->CreateTexture2D(&desc, &initData, &texture)))
            return nullptr;

        return texture;
    }

} // extern "C"

#elif defined(__ANDROID__)
// Android / OpenGL ES 3.0+
#include <GLES3/gl3.h>

extern "C" {

    //__attribute__((visibility("default")))
        GLuint UploadTextureToGPU(const uint8_t* pixelData, int width, int height)
    {
        if (!pixelData || width <= 0 || height <= 0)
            return 0;

        GLuint texID = 0;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D,
            0, GL_RGBA,
            width, height,
            0, GL_RGBA, GL_UNSIGNED_BYTE,
            pixelData);

        glBindTexture(GL_TEXTURE_2D, 0);
        return texID;
    }

    //__attribute__((visibility("default")))
        void UpdateTextureGPU(GLuint texID, const uint8_t* pixelData, int width, int height)
    {
        if (!pixelData || texID == 0)
            return;

        glBindTexture(GL_TEXTURE_2D, texID);
        glTexSubImage2D(GL_TEXTURE_2D,
            0, 0, 0,
            width, height,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixelData);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

} // extern "C"

#endif
