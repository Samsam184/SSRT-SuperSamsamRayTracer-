#ifndef CAMERA_H
#define CAMERA_H 

#include "hittable.h"
#include <ostream>
#include <fstream>
#include <cmath>

class camera {
public: 
    double aspect_ratio = 1;
    int image_width = 100;

    void render(const hittable& world) {
        
        
        initialize();
        //std::ofstream out("image.ppm", std::ios::out | std::ios::binary);
        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
        

        for (int j = 0; j < image_height; j++) {
           
            std::clog << "Scanlines completed: " << j << ' ' << "/" << ' ' << image_height << "\n" << std::flush;
            for (int i = 0; i < image_width; i++) {
                auto pixel_center = pixel00_location + (i * pixel_delta_u) + (j * pixel_delta_v);
                auto ray_direction = pixel_center - center;
                ray r(center, ray_direction);

                color pixel_color = ray_color(r, world);
                write_color(std::cout, pixel_color);
            }
        }
        std::clog << "\nRender Ended Correctly!\n\n";
    }



private: 
    int     image_height;
    point3  center;
    point3  pixel00_location;
    vec3    pixel_delta_u;
    vec3    pixel_delta_v;
    

	void initialize() {
        int image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        center = point3(0, 0, 0);

        auto focal_length = 1.0; 
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (double(image_width) / image_height);
        

        auto viewport_u = vec3(viewport_width, 0, 0);
        auto viewport_v = vec3(0, -viewport_height, 0);

        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        auto viewport_upper_left = center - vec3(0, 0, focal_length) - viewport_u / 2 - viewport_v / 2;
        pixel00_location = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
	}

	color ray_color(const ray& r, const hittable& world) const {
		hit_record rec;
		if (world.hit(r, interval(0, infinity), rec)) {
			return 0.5 * (rec.normal + color(1, 1, 1));
		}

		vec3 unity_direction = unit_vector(r.direction());
		auto a = 0.5 * (unity_direction.y() + 1.0);
		return (1.0 - a) * color(1, 1, 1) + a * color(0.5, 0.7, 1.0);

	}
};
#endif