#ifndef OIDN_H
#define OIDN_H

#include <OpenImageDenoise/oidn.hpp>
#include <vector>
#include <iostream>
#include "vec3.h"

inline void denoise_with_oidn(std::vector<vec3>& framebuffer, int width, int height) {
    try {
        oidn::DeviceRef device = oidn::newDevice();
        device.commit();

        oidn::FilterRef filter = device.newFilter("RT");

        // On convertit le framebuffer en float[] tightly packed
        std::vector<float> pixels(width * height * 3);
        for (int i = 0; i < width * height; ++i) {
            pixels[i * 3 + 0] = framebuffer[i].x();
            pixels[i * 3 + 1] = framebuffer[i].y();
            pixels[i * 3 + 2] = framebuffer[i].z();
        }

        size_t bufferSize = pixels.size() * sizeof(float);
        oidn::BufferRef colorBuf = device.newBuffer(bufferSize);
        oidn::BufferRef outputBuf = device.newBuffer(bufferSize);

        // Copier CPU → OIDN
        colorBuf.write(0, bufferSize, pixels.data());

        // Configurer le filtre
        filter.setImage("color", colorBuf, oidn::Format::Float3, width, height);
        filter.setImage("output", outputBuf, oidn::Format::Float3, width, height);
        filter.set("hdr", true);   // image HDR linéaire
        filter.set("srgb", false); // on n’est pas en sRGB
        filter.commit();

        // Exécution du filtre
        filter.execute();

        // Vérification d’erreur
        const char* errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None)
            std::cerr << "[OIDN] Error: " << errorMessage << std::endl;

        // Lecture résultat
        outputBuf.read(0, bufferSize, pixels.data());

        // Recopie vers le framebuffer
        for (int i = 0; i < width * height; ++i) {
            framebuffer[i] = vec3(pixels[i * 3 + 0],
                pixels[i * 3 + 1],
                pixels[i * 3 + 2]);
        }

        std::cout << "[OIDN] Denoised Successfully\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[OIDN] Exception: " << e.what() << std::endl;
    }
}

#endif