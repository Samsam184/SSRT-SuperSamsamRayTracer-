#ifndef CAMERA_H
#define CAMERA_H 

#include "hittable.h"
#include <ostream>
#include <fstream>
#include <algorithm>
#include <Windows.h>
#include "material.h"


class camera {
public: 

    //variables
    double aspect_ratio = 1;
    int image_width = 100;
    int samples_per_pixel = 10;
    int max_depth = 10;
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
        
        std::ofstream out("image.ppm", std::ios::out | std::ios::binary);
        out << "P3\n" << image_width << ' ' << image_height << "\n255\n";
        
        for (int j = 0; j < image_height; j++) {
            
            hideCursor();
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ';
            std::cout.flush();

            for (int i = 0; i < image_width; i++) {
                color pixel_color(0, 0, 0);                
                for (int sample = 0; sample < samples_per_pixel; sample++) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                write_color(out, pixel_samples_scale * pixel_color);
            }
        }
        std::clog << "\nRender Ended Correctly!\n\n";
        showCursor();
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

        image_height = image_width / aspect_ratio;
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
        return ray(ray_origin, ray_direction);
    }

    vec3 sample_square() const {
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    point3 defocus_disk_sample() const {
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

	color ray_color(const ray& r, int depth,  const hittable& world) const {
		
        if (depth <= 0) {
            return color(0, 0, 0);
        }
        
        hit_record rec;
		
        
        if (world.hit(r, interval(0.001, infinity), rec)) {
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered)) {
                return attenuation * ray_color(scattered, depth - 1, world);
            }
            return color(0, 0, 0);
		}

		vec3 unity_direction = unit_vector(r.direction());
		auto a = 0.5 * (unity_direction.y() + 1.0);
		return (1.0 - a) * color(1, 1, 1) + a * color(0.5, 0.7, 1.0);

	}



};
#endif