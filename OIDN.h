#ifndef OIDN_H
#define OIDN_H

#include "vec3.h"
#include <OpenImageDenoise/oidn.hpp>
#include <iostream>
#include <vector>

inline void denoise_with_oidn(
    std::vector<vec3>& colorBuffer,
    const std::vector<vec3>& albedoBuffer,
    const std::vector<vec3>& normalBuffer,
    int width,
    int height)
{
    using namespace oidn;

    try {
        DeviceRef device = newDevice(DeviceType::Default);
        device.commit();

        FilterRef filter = device.newFilter("RT");

        size_t numPixels = static_cast<size_t>(width) * static_cast<size_t>(height);

        // --- Convertir vec3 → float[3] contigu
        std::vector<float> colorData(numPixels * 3);
        std::vector<float> albedoData(numPixels * 3);
        std::vector<float> normalData(numPixels * 3);

        for (size_t i = 0; i < numPixels; ++i) {
            colorData[i * 3 + 0] = static_cast<float>(colorBuffer[i].x());
            colorData[i * 3 + 1] = static_cast<float>(colorBuffer[i].y());
            colorData[i * 3 + 2] = static_cast<float>(colorBuffer[i].z());

            if (!albedoBuffer.empty()) {
                albedoData[i * 3 + 0] = static_cast<float>(albedoBuffer[i].x());
                albedoData[i * 3 + 1] = static_cast<float>(albedoBuffer[i].y());
                albedoData[i * 3 + 2] = static_cast<float>(albedoBuffer[i].z());
            }

            if (!normalBuffer.empty()) {
                normalData[i * 3 + 0] = static_cast<float>(normalBuffer[i].x());
                normalData[i * 3 + 1] = static_cast<float>(normalBuffer[i].y());
                normalData[i * 3 + 2] = static_cast<float>(normalBuffer[i].z());
            }
        }

        // --- Buffers OIDN
        BufferRef colorBuf = device.newBuffer(colorData.size() * sizeof(float));
        colorBuf.write(0, colorData.size() * sizeof(float), colorData.data());

        BufferRef outputBuf = device.newBuffer(colorData.size() * sizeof(float));

        filter.setImage("color", colorBuf, Format::Float3, width, height);

        if (!albedoBuffer.empty()) {
            BufferRef albedoBuf = device.newBuffer(albedoData.size() * sizeof(float));
            albedoBuf.write(0, albedoData.size() * sizeof(float), albedoData.data());
            filter.setImage("albedo", albedoBuf, Format::Float3, width, height);
        }

        if (!normalBuffer.empty()) {
            BufferRef normalBuf = device.newBuffer(normalData.size() * sizeof(float));
            normalBuf.write(0, normalData.size() * sizeof(float), normalData.data());
            filter.setImage("normal", normalBuf, Format::Float3, width, height);
        }

        filter.setImage("output", outputBuf, Format::Float3, width, height);
        filter.set("hdr", true);
        filter.set("srgb", false);
        filter.commit();

        filter.execute();

        const char* errorMessage;
        if (device.getError(errorMessage) != Error::None) {
            std::cerr << "[OIDN] Error : " << errorMessage << std::endl;
        }
        else {
            std::clog << "[OIDN] Denoising ended correctly!.\n";
        }

        // --- Récupération du buffer débruité
        std::vector<float> denoisedData(colorData.size());
        outputBuf.read(0, denoisedData.size() * sizeof(float), denoisedData.data());

        for (size_t i = 0; i < numPixels; ++i) {
            colorBuffer[i].e[0] = denoisedData[i * 3 + 0];
            colorBuffer[i].e[1] = denoisedData[i * 3 + 1];
            colorBuffer[i].e[2] = denoisedData[i * 3 + 2];
        }

    }
    catch (const std::exception& e) {
        std::cerr << "[OIDN] Exception : " << e.what() << std::endl;
    }
}

#endif
