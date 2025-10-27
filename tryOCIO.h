#ifndef TRYOCIO_H
#define TRYOCIO_H

#include <iostream>
#include "OpenColorIO/OpenColorIO.h"
namespace OCIO = OCIO_NAMESPACE;

void testOCIO() {
    try {
        OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromFile(
            "D:/SSRT/cg-config-v2.2.0_aces-v1.3_ocio-v2.4.ocio");

        OCIO::ConstProcessorRcPtr processor =
            config->getProcessor("ACES - ACEScg", "sRGB - Display");

        std::cout << "Config et processor chargés avec succès !\n";

    }
    catch (OCIO::Exception& e) {
        std::cerr << "Erreur OCIO : " << e.what() << "\n";
    }
}

#endif