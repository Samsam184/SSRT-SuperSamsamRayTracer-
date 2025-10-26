#include "rtweekend.h"
#include "bvh.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"
#include "cameraMT.h"
#include "material.h"
#include "texture.h"
#include <iostream>
#include <fstream>
#include "quad.h"

void static_spheres() {
    
    hittable_list world;

    //On définis les matériaux 
    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
    auto checker = make_shared<checker_texture>(.32, color(.2, .3, .1), color(.9, .9, .9));
    //On ajoute les objets au world en disant ce que c'est, avec world.add et dedans make_shared<ce que tu veux dedans wsh>....
    world.add(make_shared<sphere>(point3(0.0, -100000.5, -1.0), 100000, make_shared<lambertian>(checker)));

    for (int a = -48; a < 48; a++) {
        for (int b = -48; b < 48; b++) {
            auto choose_mat = random_double();
            point3 center(a + .9 * random_double(), -.4, b + .9 * random_double());
            shared_ptr<material> sphere_material_diffuse;
            shared_ptr<material> sphere_material_glass;

            if (choose_mat < .8) {
                auto albedo = color::random() * color::random();
                sphere_material_diffuse = make_shared<lambertian>(albedo);
                world.add(make_shared<sphere>(center, .2, sphere_material_diffuse));
            }
            else {
                auto albedo = color::random() * color::random();
                sphere_material_glass = make_shared<dielectric>(1.55);
                world.add(make_shared<sphere>(center, .2, sphere_material_glass));
            }

        }
    }
            
        
    

    /*
    auto center = vec3(0, 0, 0);
    auto center2 = center + vec3(0, random_double(0, 2), 0);
    world.add(make_shared<sphere>(center, center2, 0.2, material_center));
    */

    world = hittable_list(make_shared<bvh_node>(world));

    camera cam;

    std::cout << "entrez la hauteur de l'image svp : ";
    std::cin >> cam.image_width;
    std::cout << "entrez le ratio d'aspect de votre image (ex: 1.77, 1.33, 2.35) : ";
    std::cin >> cam.aspect_ratio;
    std::cout << "entrez le field of view (en degrés) : ";
    std::cin >> cam.vfov;

    cam.lookfrom = point3(-2, 1, 1);
    cam.lookat = point3(0, 0, -1);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.focus_dist = 3.4;

    cam.samples_per_pixel = 100;
    cam.max_depth = 1000;
    is_colorSpace_gamma = true;

    

    cam.render(world);
    
}

void checkered_spheres() {
    hittable_list world;

    auto checker = make_shared<checker_texture>(.32, color(.2, .3, .1), color(.9, .9, .9));

    world.add(make_shared<sphere>(point3(0, -10, 0), 10, make_shared<lambertian>(checker)));
    world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.vfov = 20;
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.render(world);

}

void earth() {
    auto earth_texture = make_shared<image_texture>("D:/SSRT/tex/earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<sphere>(point3(0, 0, 0), 2, earth_surface);

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.vfov = 20;
    cam.lookfrom = point3(0, 0, 12);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.render(hittable_list(globe));

    is_colorSpace_gamma = false;
}

void perlin_spheres() {

    hittable_list world;

    auto pertex = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertex)));
    world.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertex)));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 500;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.vfov = 20;
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.render(world);

    is_colorSpace_gamma = false;
}

void quads() {
    hittable_list world;

    auto left_red = make_shared<lambertian>(color(1.0, .2, .2));
    auto back_green = make_shared<lambertian>(color(.2, 1.0, .2));
    auto right_blue = make_shared<lambertian>(color(.2, .2, 1.0));
    auto upper_orange = make_shared<lambertian>(color(1.0, .5, 0.0));
    auto lower_teal = make_shared<lambertian>(color(.2, .8, .8));

    world.add(make_shared<quad>(point3(-3, -2, 5), vec3(0, 0, -4), vec3(0, 4, 0), left_red));
    world.add(make_shared<quad>(point3(-2, -2, 0), vec3(4, 0, 0), vec3(0, 4, 0), back_green));
    world.add(make_shared<quad>(point3(3, -2, 1), vec3(0, 0, 4), vec3(0, 4, 0), right_blue));
    world.add(make_shared<quad>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4), upper_orange));
    world.add(make_shared<quad>(point3(-2, -3, 5), vec3(4, 0, 0), vec3(0, 0, -4), lower_teal));

    camera cam;

    cam.aspect_ratio = 1.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.vfov = 80;
    cam.lookfrom = point3(0, 0, 9);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.render(world);
}

int main() {
    
    switch (5) {
    case 1: static_spheres(); break;
    case 2: checkered_spheres(); break;
    case 3: earth(); break;
    case 4: perlin_spheres(); break;
    case 5: quads(); break;
    }
    
}