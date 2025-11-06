#include <stdint.h>

#ifdef _WIN32
// Windows / D3D11
#include <d3d11.h>
#include "IUnityGraphicsD3D11.h"

static ID3D11Device* g_Device = nullptr;

extern "C" {

    __declspec(dllexport)
        void SetUnityD3D11Device(IUnityGraphicsD3D11Device* device)
    {
        g_Device = device;
    }

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

} // extern "C"

#elif defined(__ANDROID__)
// Android / OpenGL ES 3.0+
#include <GLES3/gl3.h>

extern "C" {

    __attribute__((visibility("default")))
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

    __attribute__((visibility("default")))
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