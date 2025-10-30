#ifndef CAMERAMT_H
#define CAMERAMT_H 

#include "hittable.h"
#include <ostream>
#include <fstream>
#include <algorithm>
#include <Windows.h>
#include "material.h"
#include <thread>
#include <atomic>
#include <random>
#include <mutex>
#include "image_io.h"
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


        initialize();

        //std::ofstream out("image.ppm", std::ios::out | std::ios::binary);
        //out << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        std::vector<color> framebuffer(image_width * image_height);
        std::vector<color> albedobuffer(image_width * image_height);
        std::vector<color> normalbuffer(image_width * image_height);

        std::atomic<int> next_row{ 0 };
        std::mutex cout_mutex;

        

        auto worker = [&](int thread_id) {
            std::mt19937 rng(std::random_device{}() + thread_id);
            std::uniform_real_distribution<double> dist(0.0, 1.0);

            while (true) {
                int j = next_row.fetch_add(1);
                if (j >= image_height) break;

                for (int i = 0; i < image_width; i++) {

                    color pixel_color(0, 0, 0);
                    vec3 pixel_albedo(0, 0, 0);
                    vec3 pixel_normal(0, 0, 0);

                    for (int sample = 0; sample < samples_per_pixel; sample++) {
                        ray r = get_ray(i, j);
                        
                        vec3 alb(0,0,0), nrm(0,0,0);
                        color sample_color = ray_color(r, max_depth, world, alb, nrm);
                        
                        pixel_color += sample_color;
                        pixel_albedo += alb;
                        pixel_normal += nrm;
                    }
                    
                    pixel_color *= pixel_samples_scale;
                    pixel_albedo *= pixel_samples_scale;
                    pixel_normal = unit_vector(pixel_normal);
                    
                    framebuffer[j * image_width + i] = pixel_color * pixel_samples_scale;
                    albedobuffer[j * image_width + i] = pixel_albedo;
                    normalbuffer[j * image_width + i] = pixel_normal;
                
                }   
                    

                if (j % 10 == 0) {
                    std::lock_guard<std::mutex> lk(cout_mutex);
                    std::clog << "\rScanlines remaining: " << (image_height - j) << ' ';
                }
            }
        };

        int nthreads = std::thread::hardware_concurrency();
        if (nthreads == 0) {
            nthreads = 4; //ca veux dire fallback PROBLEME FAUT PANIQUER
        }
        std::vector<std::thread> threads;

        for (int t = 0; t < nthreads; t++) {
            threads.emplace_back(worker, t);
        }

        for (auto& th : threads) {
            th.join();
        }


        //if (output_format == OutputFormat::PPM) {
        //    for (int j = 0; j < image_height; j++) {
        //        for (int i = 0; i < image_width; i++) {
        //            write_color(out, framebuffer[j * image_width + i]);
        //        }
        //    }
        //}
        //else if(output_format == OutputFormat::EXR)
        //{
        //    write_exr("image_linear_ACEScg.exr", framebuffer, image_width, image_height);
        //}
        /*
       
        */
        
        if (use_denoiser) {

            std::clog << "\n[OIDN] Denoising in progress.. Please wait.\n";
            denoise_with_oidn(framebuffer, albedobuffer, normalbuffer, image_width, image_height);
            save_image("renders/SSRT_Linear_v001_denoised.exr", framebuffer, image_width, image_height);
        }
        else {
            save_image("renders/SSRT_Linear_v001_denoised.exr", framebuffer, image_width, image_height);
        }
        
        std::clog << "\nRender Ended Correctly!! \n\n";
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




    ray get_ray(int i, int j) const {
        auto offset = sample_square();
        auto pixel_sample = pixel00_location + ((i + offset.x()) * pixel_delta_u) + ((j + offset.y()) * pixel_delta_v);
        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;
        auto ray_time = random_double();
        return ray(ray_origin, ray_direction, ray_time);
    }

    vec3 sample_square() const {
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    point3 defocus_disk_sample() const {
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

	color ray_color(const ray& r, int depth,  const hittable& world, vec3& out_albedo, vec3& out_normal) const {
		
        if (depth <= 0) {
            return color(0, 0, 0);
        }
        
        hit_record rec;
		
        
        if (!world.hit(r, interval(0.001, infinity), rec)) {
            out_albedo = vec3(0, 0, 0);
            out_normal = vec3(0, 0, 0);
            return background;
        }

        out_normal = unit_vector(rec.normal);

        if (auto lambert = dynamic_cast<const lambertian*>(rec.mat.get())) {
            out_albedo = lambert->tex->value(rec.u, rec.v, rec.p);
        }
        else {
            out_albedo = vec3(.5, .5, .5);
        }
        

        ray scattered;
        color attenuation;
        color color_from_emission = rec.mat->emitted(rec.u, rec.v, rec.p);

        if (!rec.mat->scatter(r, rec, attenuation, scattered))
            return color_from_emission;

        color color_from_scatter = attenuation * ray_color(scattered, depth - 1, world, out_albedo, out_normal);

        return color_from_emission + color_from_scatter;

	}

};
#endif