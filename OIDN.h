#ifndef OIDN_H
#define OIDN_H

#include <OpenImageDenoise/oidn.hpp>
#include <vector>
#include <iostream>
#include "vec3.h"

inline void denoise_with_oidn(std::vector<vec3>& framebuffer, std::vector<vec3>& albedobuffer, std::vector<vec3>& normalbuffer,int width, int height) {
    try {
        oidn::DeviceRef device = oidn::newDevice();
        device.commit();

        oidn::FilterRef filter = device.newFilter("RT");

        // On convertit le framebuffer en float[] tightly packed
        std::vector<float> color_data(width * height * 3);
        std::vector<float> albedo_data(width * height * 3);
        std::vector<float> normal_data(width * height * 3);


        for (int i = 0; i < width * height; ++i) {

            color_data[i * 3 + 0] = framebuffer[i].x();
            color_data[i * 3 + 1] = framebuffer[i].y();
            color_data[i * 3 + 2] = framebuffer[i].z();

            albedo_data[i * 3 + 0] = albedobuffer[i].x();
            albedo_data[i * 3 + 1] = albedobuffer[i].y();
            albedo_data[i * 3 + 2] = albedobuffer[i].z();

            normal_data[i * 3 + 0] = normalbuffer[i].x();
            normal_data[i * 3 + 1] = normalbuffer[i].y();
            normal_data[i * 3 + 2] = normalbuffer[i].z();


        }

        size_t bufferSize = width * height * 3 * sizeof(float);



        oidn::BufferRef colorBuf = device.newBuffer(bufferSize);
        oidn::BufferRef albedoBuf = device.newBuffer(bufferSize);
        oidn::BufferRef normalBuf = device.newBuffer(bufferSize);
        oidn::BufferRef outputBuf = device.newBuffer(bufferSize);


        // Copier CPU → OIDN
        colorBuf.write(0, bufferSize, color_data.data());
        albedoBuf.write(0, bufferSize, albedo_data.data());
        normalBuf.write(0, bufferSize, normal_data.data());


        // Configurer le filtre
        filter.setImage("color", colorBuf, oidn::Format::Float3, width, height);
        filter.setImage("albedo", albedoBuf, oidn::Format::Float3, width, height);
        filter.setImage("normal", normalBuf, oidn::Format::Float3, width, height);
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
        outputBuf.read(0, bufferSize, color_data.data());

        // Recopie vers le framebuffer
        for (int i = 0; i < width * height; ++i) {
            framebuffer[i] = vec3(color_data[i * 3 + 0],
                                  color_data[i * 3 + 1],
                                  color_data[i * 3 + 2]);
        }

        std::cout << "[OIDN] Denoised Successfully\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[OIDN] Exception: " << e.what() << std::endl;
    }
}

#endif