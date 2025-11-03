#ifndef CAMERAMT_H
#define CAMERAMT_H 

#include "hittable.h"
#include <ostream>
#include <fstream>
#include <algorithm>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "material.h"
#include <thread>
#include <atomic>
#include <random>
#include <mutex>
#include "image_io.h"
#include <chrono>
#include "xoshiro128.h"
#include "fast_rng.h"
#include "thread_pool.h"
#include "morton2D.h"
#include "OIDN.h"

class camera {
public: 
    //variables
    bool use_denoiser = true;
    double aspect_ratio = 16.0/9.0;
    int image_width = 100;
    int samples_per_pixel = 10;
    int max_depth = 10;
    color background;
    double vfov = 90;
    point3 lookfrom = point3(0, 0, 0);
    point3 lookat = point3(0, 0, 0);
    vec3 vup = vec3(0, 1, 0);
    double defocus_angle = 0;
    double focus_dist = 10;
    //variables


    void hideCursor() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hOut, &cursorInfo);
        cursorInfo.bVisible = FALSE; // cacher le curseur
        SetConsoleCursorInfo(hOut, &cursorInfo);
    }
    void showCursor() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hOut, &cursorInfo);
        cursorInfo.bVisible = TRUE; // montrer le curseur
        SetConsoleCursorInfo(hOut, &cursorInfo);
    }


    void render(const hittable& world) {

#ifdef _WIN32 
        DWORD_PTR mask = 0xFF;
        SetThreadAffinityMask(GetCurrentThread(), mask);
#endif

        int nthreads = std::thread::hardware_concurrency();
        if (nthreads == 0) nthreads = 4;

        ThreadPool pool(nthreads);
        int tileSize = 128; // taille d’une tuile : 32x32 pixels

        //auto t_start = std::chrono::high_resolution_clock::now();

        initialize();

        std::vector<color> framebuffer(image_width * image_height);
        std::vector<color> albedobuffer(image_width * image_height);
        std::vector<color> normalbuffer(image_width * image_height);

        std::mutex cout_mutex;

        std::vector<std::pair<uint32_t, std::pair<int, int>>> tiles;

        for (int y = 0; y < image_height; y += tileSize) {
            for (int x = 0; x < image_width; x += tileSize) {
                uint32_t morton = morton2D(x / tileSize, y / tileSize);
                tiles.push_back({ morton, {x,y} });
            }
        }

        std::sort(tiles.begin(), tiles.end(), [](auto& a, auto& b) {return a.first < b.first; });


        // --- Boucle principale : envoi des tâches au ThreadPool ---
#pragma omp parallel for schedule(dynamic)
        for (int tile_idx = 0; tile_idx < (int)tiles.size(); ++tile_idx) {
            const auto& [code, xy] = tiles[tile_idx];
            int x = xy.first;
            int y = xy.second;

            uint64_t rng_state = 1337u + y * image_width + x;

            for (int j = y; j < std::min(y + tileSize, image_height); ++j) {
                for (int i = x; i < std::min(x + tileSize, image_width); ++i) {

                    color pixel_color(0, 0, 0);
                    vec3 pixel_albedo(0, 0, 0);
                    vec3 pixel_normal(0, 0, 0);
                    const point3 pixel_base = pixel00_location + i * pixel_delta_u + j * pixel_delta_v;

                    // --- Échantillonnage multiple pour AA ---
#pragma omp simd
                    for (int sample = 0; sample < samples_per_pixel; sample++) {
                        float rx = randf(rng_state);
                        float ry = randf(rng_state);

                        const vec3 pixel_sample =
                            pixel_base + (rx - 0.5f) * pixel_delta_u + (ry - 0.5f) * pixel_delta_v;

                        const point3 ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
                        const vec3 ray_direction = pixel_sample - ray_origin;

                        ray r(ray_origin, ray_direction);

                        vec3 alb(0, 0, 0), nrm(0, 0, 0);
                        color sample_color = ray_color(r, max_depth, world);

                        pixel_color += sample_color;
                        pixel_albedo += alb;
                        pixel_normal += nrm;
                    }

                    pixel_color *= pixel_samples_scale;
                    pixel_albedo *= pixel_samples_scale;
                    pixel_normal = unit_vector(pixel_normal);

                    framebuffer[j * image_width + i] = pixel_color;
                    albedobuffer[j * image_width + i] = pixel_albedo;
                    normalbuffer[j * image_width + i] = pixel_normal;
                }
            }

#pragma omp critical
            {
                static int tiles_done = 0;
                int done = ++tiles_done;
                if (done % 10 == 0) {
                    double progress = 100.0 * done / ((image_height / tileSize) * (image_width / tileSize));
                    std::clog << "\rProgress: " << (int)progress << "% ";
                }
            }
        }

        // Attendre que toutes les tuiles soient calculées
        pool.wait();
       

        if (use_denoiser) {
            std::cout << "\n\n[OIDN] Denoising in progress... Please wait.\n";
            denoise_with_oidn(framebuffer, albedobuffer, normalbuffer, image_width, image_height);
            save_image("renders/SSRT_Linear_denoised_v001.exr", framebuffer, image_width, image_height);
            std::clog << "\nRender ended correctly!\n\n";

        }
        else {
            save_image("renders/SSRT_Linear_v001.exr", framebuffer, image_width, image_height);
            std::clog << "\nRender ended correctly!\n\n";
        }
        /*
        auto t_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> total = t_end - t_start;

        if (total.count() > 60) {
            std::clog << "\nTotal render time : " << total.count() / 60.0 << " minutes\n";
        }
        else {
            std::clog << "\nTotal render time : " << total.count() << " secondes\n";
        }
        */
    }

    

private: 
    //variables
    int    image_height;   
    double pixel_samples_scale;
    point3 center;       
    point3 pixel00_location;    
    vec3   pixel_delta_u;  
    vec3   pixel_delta_v;  
    vec3   u, v, w;
    vec3 defocus_disk_u;
    vec3 defocus_disk_v;
    //variables



    void initialize() {

        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;


        pixel_samples_scale = 1.0 / samples_per_pixel;
        center = lookfrom;

      
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        auto viewport_height = 2*h*focus_dist;
        auto viewport_width = viewport_height * (double(image_width) / image_height);

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        auto viewport_u = viewport_width * u;
        auto viewport_v = viewport_height * -v;

        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
        pixel00_location = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    vec3 sample_square() const {
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    point3 defocus_disk_sample() const {
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

	inline __forceinline color ray_color(const ray& r, int depth,  const hittable& world) const noexcept {
		
        if (depth <= 0) return color(0, 0, 0);
        
        hit_record rec;
		
        
        if (!world.hit(r, interval(0.001, infinity), rec)) {
            return background;
        }

        ray scattered;
        color attenuation;
        color color_from_emission = rec.mat->emitted(rec.u, rec.v, rec.p);

        if (!rec.mat->scatter(r, rec, attenuation, scattered))
            return color_from_emission;

        color color_from_scatter = attenuation * ray_color(scattered, depth - 1, world);

        return color_from_emission + color_from_scatter;

	}

};
#endif