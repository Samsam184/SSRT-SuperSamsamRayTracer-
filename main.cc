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

                    ////////////// en mode debug ///////////////

pointer vers le dossier ou se trouve le projet : en l'occurence chez moi "D:/SSRT/"

    cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/Users/assam/vcpkg/scripts/buildsystems/vcpkg.cmake ----------------------> créer le dossier build avec tout dedans, non pas avec vcpkg qui proviens de VSCode, mais avec le vcpkg custom qui nous a servi a download toute les libs (a faire qu'une seule fois)
    Copier a la mano tout les dll de OIDN (a faire qu'une seule fois)
    cmake --build build  ---------------------------------------------------------------------------------------------> fabriquer le .exe a partir de tout ce qu'on a build juste avant
    build\Debug\SSRT.exe ---------------------------------------------------------------------------------------------> lancer le .exe

                    /////////////// en mode release ////////////

    cmake --build build --config Release -j 
    build\Release\SSRT.exe
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
#include "mesh.h"


void all_feature_cornell_box() {

    // testOCIO();

    hittable_list world;

    //Materials Lists
    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto glass = make_shared<dielectric>(1.33);
    auto light = make_shared<diffuse_light>(color(2, 2, 2));
    auto mat = make_shared<lambertian>(color(1, 1, 1));

    //Cornell Box
    world.add(make_shared<quad>(vec3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
    world.add(make_shared<quad>(vec3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
    world.add(make_shared<quad>(vec3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light));
    world.add(make_shared<quad>(vec3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
    world.add(make_shared<quad>(vec3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
    world.add(make_shared<quad>(vec3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));


    auto teapod = make_shared<Mesh>("asset/teapod.obj", mat);
    auto scaled_teapod = make_shared<scale>(teapod, vec3(80, 80, 80));
    auto translated_teapod = make_shared<translate>(scaled_teapod, vec3(260, 20, 200));
    world.add(translated_teapod);

    world = hittable_list(make_shared<bvh_node>(world));

    camera cam;


    //std::cout << "Set Image Height : ";
    //std::cin >> 
        cam.image_width = 1920;
    //std::cout << "Set Aspect Ratio : ";
    //std::cin >> 
        cam.aspect_ratio = 1;
    //std::cout << "Set FOV (in degrees) : ";
    //std::cin >> 
        cam.vfov = 35;

    cam.samples_per_pixel = 8;
    cam.max_depth = 50;
    cam.background = color(0, 0, 0);

    cam.lookfrom = vec3(278, 278, -800);
    cam.lookat = vec3(278, 278, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
  
    cam.use_denoiser = true;

    cam.render(world);


}

int main() {

    /*
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
    */

    std::ofstream log("benchmark_values/benchmark_results_v006.txt", std::ios::out);
    if (!log) {
        std::cerr << "Erreur : impossible de créer benchmark_results.txt\n";
        return 1;
    }

    const int num_runs = 1;
    double total_time = 0.0;

    for (int i = 0; i < num_runs; i++) {
        std::clog << "\n--- Rendu " << (i + 1) << " / " << num_runs << " ---\n";

        auto t_start = std::chrono::high_resolution_clock::now();

        all_feature_cornell_box();  // ta fonction de rendu principale

        auto t_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = t_end - t_start;
        total_time += elapsed.count();

        if (elapsed.count() > 60) {
            std::clog << "Durée du rendu " << (i + 1) << " : " << elapsed.count() / 60 << " m\n";
        }
        else {
            std::clog << "Durée du rendu " << (i + 1) << " : " << elapsed.count() << " s\n";
        }
        
    }

    double avg_time = total_time / num_runs;
    log << "\nMoyenne sur " << num_runs << " rendus : " << avg_time << " secondes\n";
    std::clog << "\nMoyenne : " << avg_time << " secondes\n";

    log.close();
    std::clog << "\nBenchmark terminé ! Résultats enregistrés dans benchmark_results.txt\n";

    return 0;
}
