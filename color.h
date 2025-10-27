#ifndef COLOR_H
#define COLOR_H

#include "interval.h"
#include "vec3.h"
#include "tinyexr.h"
#include "OpenColorIO/OpenColorIO.h"
namespace OCIO = OCIO_NAMESPACE;

using color = vec3;

extern bool is_colorSpace_gamma = false;

inline double linear_to_gamma(double linear_component) {
    if (linear_component > 0) {
        return std::sqrt(linear_component);
    }
    else {
        return 0;
    }
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

void write_color(std::ostream& out, color& pixel_color) {

    pixel_color = ocio_transform(pixel_color);

    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    if (is_colorSpace_gamma) {
        pixel_color = color(linear_to_gamma(r), linear_to_gamma(g), linear_to_gamma(b));
    }
    
    static const interval intensity(0.000, 0.999);
    int rbyte = int(256 * intensity.clamp(r));
    int gbyte = int(256 * intensity.clamp(g));
    int bbyte = int(256 * intensity.clamp(b));

    // Write out the pixel color components.
    out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}

inline bool write_exr(const std::string& filename, const std::vector<color>& framebuffer, int width, int height) {
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

#endif