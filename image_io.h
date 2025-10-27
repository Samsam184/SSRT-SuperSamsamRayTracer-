#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

#include "tinyexr.h"
#include "vec3.h"
#include "interval.h"

#include "OpenColorIO/OpenColorIO.h"
namespace OCIO = OCIO_NAMESPACE;

#include <algorithm>
#include <cmath>
#include <iostream>

using color = vec3;


bool write_exr(const std::string& filename, const std::vector<color>& framebuffer, int width, int height) {
    EXRHeader header;
    InitEXRHeader(&header);
    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 3;

    std::vector<float> images[3];
    for (int i = 0; i < 3; i++) {
        images[i].resize(width * height);
    }

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int idx = j * width + i;
            const color& c = framebuffer[idx];

            images[0][idx] = c.z();
            images[1][idx] = c.y();
            images[2][idx] = c.x();

        }
    }

    float* image_ptr[3];
    image_ptr[0] = images[0].data();
    image_ptr[1] = images[1].data();
    image_ptr[2] = images[2].data();

    image.images = reinterpret_cast<unsigned char**>(image_ptr);
    image.width = width;
    image.height = height;

    header.num_channels = 3;
    header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * 3);
    strncpy(header.channels[0].name, "B", 255);
    header.channels[0].name[1] = '\0';
    strncpy(header.channels[1].name, "G", 255);
    header.channels[1].name[1] = '\0';
    strncpy(header.channels[2].name, "R", 255);
    header.channels[2].name[1] = '\0';

    header.pixel_types = (int*)malloc(sizeof(int) * 3);
    header.requested_pixel_types = (int*)malloc(sizeof(int) * 3);
    for (int i = 0; i < 3; i++) {
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
    }

    const char* err = nullptr;
    int ret = SaveEXRImageToFile(&image, &header, filename.c_str(), &err);

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);

    if (ret != TINYEXR_SUCCESS) {
        std::cerr << "Failed to save EXR: " << err << std::endl;
        FreeEXRErrorMessage(err);
        return false;
    }

    std::cout << "Saved Linear EXR: " << filename << std::endl;
    return true;
}

inline color ocio_transform(const color& in_col) {

    static OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromFile("D:/SSRT/cg-config-v2.2.0_aces-v1.3_ocio-v2.4.ocio");

    static OCIO::ConstProcessorRcPtr processor = config->getProcessor("ACEScg", "sRGB - Display");

    color out_col = in_col;

    float pixels[3] = { (float)out_col.x(), (float)out_col.y(), (float)out_col.z() };
    OCIO::PackedImageDesc imgDesc(pixels, 1, 1, 3);
    processor->getDefaultCPUProcessor()->apply(imgDesc);

    return color(pixels[0], pixels[1], pixels[2]);

}

inline void save_image(const std::string& filename, std::vector<color>& framebuffer, int width, int height) {
    if (filename.ends_with(".exr")) {
        write_exr(filename, framebuffer, width, height);
        return;
    }

    std::vector<unsigned char> image_data(width * height * 3);

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int idx = j * width + i;
            color& c = framebuffer[idx];

            c = ocio_transform(c);

            auto r = static_cast<unsigned char>(255.999 * std::clamp(c.x(), 0.0, 0.999));
            auto g = static_cast<unsigned char>(255.999 * std::clamp(c.y(), 0.0, 0.999));
            auto b = static_cast<unsigned char>(255.999 * std::clamp(c.z(), 0.0, 0.999));

            image_data[3 * idx + 0] = r;
            image_data[3 * idx + 1] = g;
            image_data[3 * idx + 2] = b;

        }
    }

    if (filename.ends_with(".png")) {
        stbi_write_png(filename.c_str(), width, height, 3, image_data.data(), width * 3);
    } else if (filename.ends_with(".jpg") || filename.ends_with(".jpeg")) {
        stbi_write_jpg(filename.c_str(), width, height, 3, image_data.data(), 95);
    } else if (filename.ends_with(".tga")) {
        stbi_write_tga(filename.c_str(), width, height, 3, image_data.data());
    } else if (filename.ends_with(".bmp")) {
        stbi_write_bmp(filename.c_str(), width, height, 3, image_data.data());
    }
}

#endif