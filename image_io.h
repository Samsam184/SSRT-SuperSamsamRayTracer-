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

struct AOVLayer {
    std::string name;
    std::vector <color> data;
};

inline color ocio_transform_raw_to_acescg(const color& in_col) {

    static OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromFile("D:/SSRT/cg-config-v2.2.0_aces-v1.3_ocio-v2.4.ocio");

    static OCIO::ConstProcessorRcPtr processor = config->getProcessor("RAW", "ACEScg");

    color out_col = in_col;

    float pixels[3] = { (float)out_col.x(), (float)out_col.y(), (float)out_col.z() };
    OCIO::PackedImageDesc imgDesc(pixels, 1, 1, 3);
    processor->getDefaultCPUProcessor()->apply(imgDesc);

    return color(pixels[0], pixels[1], pixels[2]);

}

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
            const color& c = ocio_transform_raw_to_acescg(framebuffer[idx]);

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
    strncpy(header.channels[1].name, "G", 255);
    strncpy(header.channels[2].name, "R", 255);

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
        std::cerr << "\nFailed to save EXR: " << err << std::endl;
        FreeEXRErrorMessage(err);
        return false;
    }

    std::cout << "\nSaved Linear EXR: " << filename << std::endl;
    return true;
}

bool write_exr_multilayer(const std::string& filename,
    const std::vector<AOVLayer>& layers,
    int width, int height)
{
    EXRHeader header;
    InitEXRHeader(&header);
    EXRImage image;
    InitEXRImage(&image);

    const int num_layers = static_cast<int>(layers.size());
    const int num_channels = num_layers * 3; // R,G,B par layer

    image.num_channels = num_channels;

    // Buffers qui vivent jusqu’à la fin
    std::vector<std::vector<float>> planar_buffers(num_channels);
    std::vector<float*> image_ptrs(num_channels);
    std::vector<std::string> channel_names(num_channels);

    int ch = 0;
    for (const auto& layer : layers) {
        planar_buffers[ch + 0].resize(width * height); // B
        planar_buffers[ch + 1].resize(width * height); // G
        planar_buffers[ch + 2].resize(width * height); // R

        for (int p = 0; p < width * height; ++p) {
            const color& c = layer.data[p];
            planar_buffers[ch + 0][p] = static_cast<float>(c.z());
            planar_buffers[ch + 1][p] = static_cast<float>(c.y());
            planar_buffers[ch + 2][p] = static_cast<float>(c.x());
        }

        image_ptrs[ch + 0] = planar_buffers[ch + 0].data();
        image_ptrs[ch + 1] = planar_buffers[ch + 1].data();
        image_ptrs[ch + 2] = planar_buffers[ch + 2].data();

        channel_names[ch + 0] = layer.name + ".B";
        channel_names[ch + 1] = layer.name + ".G";
        channel_names[ch + 2] = layer.name + ".R";

        ch += 3;
    }

    image.images = reinterpret_cast<unsigned char**>(image_ptrs.data());
    image.width = width;
    image.height = height;

    header.num_channels = num_channels;
    header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * num_channels);
    header.pixel_types = (int*)malloc(sizeof(int) * num_channels);
    header.requested_pixel_types = (int*)malloc(sizeof(int) * num_channels);

    for (int i = 0; i < num_channels; i++) {
        strncpy(header.channels[i].name, channel_names[i].c_str(), 255);
        header.channels[i].name[255] = '\0';
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
    }

    const char* err = nullptr;
    int ret = SaveEXRImageToFile(&image, &header, filename.c_str(), &err);

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);

    if (ret != TINYEXR_SUCCESS) {
        std::cerr << "Failed to save multi-layer EXR: " << (err ? err : "unknown") << std::endl;
        if (err) FreeEXRErrorMessage(err);
        return false;
    }

    std::cout << "Saved multi-layer EXR: " << filename << std::endl;
    return true;
}



inline color ocio_transform_acescg_to_srgb(const color& in_col) {

    static OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromFile("D:/SSRT/cg-config-v2.2.0_aces-v1.3_ocio-v2.4.ocio");

    static OCIO::ConstProcessorRcPtr processor = config->getProcessor("RAW", "sRGB - Display");

    color out_col = in_col;

    float pixels[3] = { (float)out_col.x(), (float)out_col.y(), (float)out_col.z() };
    OCIO::PackedImageDesc imgDesc(pixels, 1, 1, 3);
    processor->getDefaultCPUProcessor()->apply(imgDesc);

    return color(pixels[0], pixels[1], pixels[2]);

}

inline void save_image(const std::string& filename, std::vector<color> framebuffer, int width, int height) {
    if (filename.ends_with(".exr")) {
        write_exr(filename, framebuffer, width, height);
        return;
    }

    std::vector<unsigned char> image_data(width * height * 3);

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int idx = j * width + i;
            color c = ocio_transform_acescg_to_srgb(framebuffer[idx]);

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