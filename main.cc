/*
/////////////////////////////////// PREREQUIS //////////////////////////////////////////

Installer vcpkg

avoir les bibliothèques suivantes :

    - OpenImageIO
    - TinyEXR
    - OIDN

    ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
    Pour OIDN, j'ai pas encore trouvé comment download via VCPKG... Donc pour le moment, je télécharge OIDN depuis le site de Intel, je décompresse dans le dossier SSRT
    Et je link dans le CmakeLists.txt a la main, avec ces commandes la :

    include_directories("D:/SSRT/external/oidn-2.3.3.x64.windows/include") ---------------------------------> Pour définir ce qui se trouve dans le dossier include (les fichiers .h et .hpp)
    link_directories("D:/SSRT/external/oidn-2.3.3.x64.windows/lib") ----------------------------------------> Pour définir ce qui se trouve dans le dossier lib (le fichier .lib)
    target_link_libraries(SSRT PRIVATE "D:/SSRT/external/oidn-2.3.3.x64.windows/lib/OpenImageDenoise.lib") -> Pour link ce qui se trouve dans le dossier lib (le fichier .lib)

    Turbo problème, c'est qu'en gros j'dois dire a l'utilisateur de changer la ou se trouve son dossier OIDN, relou mais pour l'instant ca fonctionne haha

    Problème aussi, faut copier a la main tout les fichiers .dll de OIDN dans le dossier build
    Comme on l'installe pas via VCPKG, tout les liens doivent être fait a la main...
    ---------------------------------------------------------------------------------------------------------------------------------------------------------------------

    ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
    Pour OpenImageIO, faut faire gaffe a choper le fichier config.ocio qui correspond a la version de OIIO qu'on a installé, pke sinon la conversion ACEScg -> sRGB fonctionnera pas
    (Pour faire simple c'est juste qu'entre les versions, ces bouffons ont renommé leur pipe, il est bcp plus claire oui, mais plus compatible avec les autres versions un peut moins récentes...)

    A l'heure ou j'écris ces lignes, la version de OIIO c'est : 2.4.2

    Le fichier config.ocio c'est le suivant pour cette version : cg-config-v2.2.0_aces-v1.3_ocio-v2.4.ocio
    ---------------------------------------------------------------------------------------------------------------------------------------------------------------------


Bien vérifier d'avoir installé vcpkg dans le dossier "C:/users/'nom d'utilisateurs'/", pke si on l'installe ailleurs, ou même a la racine, dans le C:/, bah ca marche pas, jsp pk et ca me soule ptdrrr

Quand une lib est pas reconnue..... d'avoir redémarré VSCode a résolu le problème...


Les commandes pour build :

pointer vers le dossier ou se trouve le projet : en l'occurence chez moi "D:/SSRT/"

    cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/Users/assam/vcpkg/scripts/buildsystems/vcpkg.cmake ----------------------> créer le dossier build avec tout dedans, non pas avec vcpkg qui proviens de VSCode, mais avec le vcpkg custom qui nous a servi a download toute les libs (a faire qu'une seule fois)
    Copier a la mano tout les dll de OIDN (a faire qu'une seule fois)
    cmake --build build  ---------------------------------------------------------------------------------------------> fabriquer le .exe a partir de tout ce qu'on a build juste avant
    build\Debug\SSRT.exe ---------------------------------------------------------------------------------------------> lancer le .exe


/////////////////////////////////// PREREQUIS //////////////////////////////////////////
*/


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
#include <algorithm>
#include "quad.h"
#include "constant_medium.h"

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
    cam.background = color(0.70, 0.80, 1.00);

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
    cam.background = color(0.70, 0.80, 1.00);

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
    cam.background = color(0.70, 0.80, 1.00);

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
    cam.background = color(0.70, 0.80, 1.00);

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
    cam.background = color(0,1,1);

    cam.vfov = 80;
    cam.lookfrom = point3(0, 0, 9);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.render(world);
}

void simple_lights() {
    hittable_list world;

    auto pertex = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertex)));
    world.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertex)));


    auto difflight = make_shared<diffuse_light>(color(5, 5, 5));
    world.add(make_shared<quad>(point3(3, 1, -2), vec3(2, 0, 0), vec3(0, 2, 0), difflight));
    world.add(make_shared<sphere>(point3(0, 7, 0), 1.5, difflight));

    world = hittable_list(make_shared<bvh_node>(world));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 1280;
    cam.samples_per_pixel = 10;
    cam.max_depth = 50;
    cam.background = color(0, 0, 0);

    cam.vfov = 20;
    cam.lookfrom = point3(26, 3, 6);
    cam.lookat = point3(0, 2, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.render(world);

    is_colorSpace_gamma = true;
}

void cornell_box() {

    // testOCIO();

        hittable_list world;

        //Materials Lists
        auto red = make_shared<lambertian>(color(.65, .05, .05));
        auto white = make_shared<lambertian>(color(.73, .73, .73));
        auto green = make_shared<lambertian>(color(.12, .45, .15));
        auto light = make_shared<diffuse_light>(color(15, 15, 15));


        //Cornell Box
        world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
        world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
        world.add(make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light));
        world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
        world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
        world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

        //Box 1
        shared_ptr<hittable> box1 = box(point3(0, 0, 0), point3(165, 330, 165), white);
        box1 = make_shared<rotate_y>(box1, 25);
        box1 = make_shared<translate>(box1, vec3(265, 0, 295));
        world.add(box1);

        //Box 2
        shared_ptr<hittable> box2 = box(point3(0, 0, 0), point3(165, 165, 165), white);
        box2 = make_shared<rotate_y>(box2, -18);
        box2 = make_shared<translate>(box2, vec3(130, 0, 65));
        world.add(box2);

        //BVH Optimisation settings
        world = hittable_list(make_shared<bvh_node>(world));


        //Camera and Rendering
        camera cam;

        cam.aspect_ratio = 1.0;
        cam.image_width = 1280;
        cam.samples_per_pixel = 20;
        cam.max_depth = 50;
        cam.background = color(0, 0, 0);

        cam.vfov = 40;
        cam.lookfrom = point3(278, 278, -800);
        cam.lookat = point3(278, 278, 0);
        cam.vup = vec3(0, 1, 0);

        cam.defocus_angle = 0;

        cam.render(world);

        is_colorSpace_gamma = true;
    
}

void smoke_cornell_box() {

    // testOCIO();

    hittable_list world;

    //Materials Lists
    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));


    //Cornell Box
    world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
    world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
    world.add(make_shared<quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light));
    world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
    world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
    world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

    //Box 1
    shared_ptr<hittable> box1 = box(point3(0, 0, 0), point3(165, 330, 165), white);
    box1 = make_shared<rotate_y>(box1, 25);
    box1 = make_shared<translate>(box1, vec3(265, 0, 295));
    //Add Box 1 entant que smoke
    world.add(make_shared<constant_medium>(box1, 0.01, color(0, 0, 0)));

    //Box 2
    shared_ptr<hittable> box2 = box(point3(0, 0, 0), point3(165, 165, 165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130, 0, 65));
    //Add Box 2 entant que smoke
    world.add(make_shared<constant_medium>(box2, 0.01, color(1, 1, 1)));


    
    

    //BVH Optimisation settings
    world = hittable_list(make_shared<bvh_node>(world));


    //Camera and Rendering
    camera cam;

    cam.aspect_ratio = 1.0;
    cam.image_width = 1280;
    cam.samples_per_pixel = 40;
    cam.max_depth = 50;
    cam.background = color(0, 0, 0);

    cam.vfov = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat = point3(278, 278, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.render(world);

    is_colorSpace_gamma = true;

}

void all_feature_cornell_box() {

    // testOCIO();

    hittable_list world;

    //Materials Lists
    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto metallic_blue = make_shared<metal>(color(0,.2,1), .1);
    auto glass = make_shared<dielectric>(1.33);
    auto light = make_shared<diffuse_light>(color(5, 5, 5));


    //Cornell Box
    world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
    world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
    world.add(make_shared<quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light));
    world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
    world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
    world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

    //Box 1
    shared_ptr<hittable> box1 = box(point3(0, 0, 0), point3(165, 330, 165), white);
    box1 = make_shared<rotate_y>(box1, 25);
    box1 = make_shared<translate>(box1, vec3(265, 0, 295));
    world.add(make_shared<constant_medium>(box1,.03, color(0, .05, 1)));

    //Box 2
    shared_ptr<hittable> box2 = box(point3(0, 0, 0), point3(165, 165, 165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130, 0, 65));
    world.add(box2);

    //Volume Env
    shared_ptr<hittable> boxVolumeEnv = box(point3(0, 0, 0), point3(555, 555, 555), white);
    boxVolumeEnv = make_shared<translate>(boxVolumeEnv, vec3(0, 0, 0));
    world.add(make_shared<constant_medium>(boxVolumeEnv, 0, color(1, 1, 1)));

    world.add(make_shared<sphere>(point3(190, 380, 220), 100, glass));
    world.add(make_shared<sphere>(point3(400, 100, 120), 85, metallic_blue));

    //BVH Optimisation settings
    world = hittable_list(make_shared<bvh_node>(world));


    //Camera and Rendering
    camera cam;


    std::cout << "entrez la hauteur de l'image svp : ";
    std::cin >> cam.image_width;
    std::cout << "entrez le ratio d'aspect de votre image (ex: 1.77, 1.33, 2.35) : ";
    std::cin >> cam.aspect_ratio;
    std::cout << "entrez le field of view (en degrés) : ";
    std::cin >> cam.vfov;
    
    
    cam.samples_per_pixel = 5;
    cam.max_depth = 50;
    cam.background = color(0, 0, 0);

    cam.lookfrom = point3(278, 278, -800);
    cam.lookat = point3(278, 278, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.use_denoiser = true;
    
    cam.render(world);

    is_colorSpace_gamma = true;

}

int main() {
    
    switch (9) {
    case 1: static_spheres(); break;
    case 2: checkered_spheres(); break;
    case 3: earth(); break;
    case 4: perlin_spheres(); break;
    case 5: quads(); break;
    case 6: simple_lights(); break;
    case 7: cornell_box(); break;
    case 8: smoke_cornell_box(); break;
    case 9: all_feature_cornell_box(); break;
    }
    
}