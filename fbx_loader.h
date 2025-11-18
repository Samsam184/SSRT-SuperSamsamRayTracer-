#ifndef FBX_LOADER_H
#define FBX_LOADER_H

#include <fbxsdk.h>
#include <memory>
#include <string>
#include "mesh.h"
#include "vec3.h"
#include "triangle.h"
#include <iostream>

std::shared_ptr<Mesh> LoadFBX(const std::string& path, std::shared_ptr<material> mat);


static vec3 ConvertFbxVector(const FbxVector4& v) {
    return vec3((double)v[0], (double)v[1], (double)v[2]);
}

static vec3 ConvertFbxUV(const FbxVector2& v) {
    return vec3((double)v[0], (double)v[1], 0.0);
}

static void ProcessMesh(FbxMesh* fbxMesh, const FbxAMatrix& transform, std::shared_ptr<Mesh> mesh, std::shared_ptr<material> mat)
{
    fbxMesh->RemoveBadPolygons();
    FbxGeometryConverter converter(fbxMesh->GetFbxManager());
    converter.Triangulate(fbxMesh, true);

    int polyCount = fbxMesh->GetPolygonCount();
    FbxVector4* ctrlPoints = fbxMesh->GetControlPoints();

    FbxStringList uvSetNameList;
    fbxMesh->GetUVSetNames(uvSetNameList);

    const char* uvName = uvSetNameList.GetCount() > 0 ? uvSetNameList[0] : nullptr;

    for (int poly = 0; poly < polyCount; poly++) {

        if (fbxMesh->GetPolygonSize(poly) != 3)
            continue;

        vec3 P[3], N[3], UV[3];

        for (int v = 0; v < 3; v++) {
            int ctrlIndex = fbxMesh->GetPolygonVertex(poly, v);

            // Position
            FbxVector4 pos = ctrlPoints[ctrlIndex];
            pos = transform.MultT(pos);
            P[v] = ConvertFbxVector(pos);

            // Normal
            FbxVector4 n;
            fbxMesh->GetPolygonVertexNormal(poly, v, n);
            N[v] = unit_vector(ConvertFbxVector(n));

            // UV
            if (uvName) {
                bool unmapped;
                FbxVector2 uv;
                fbxMesh->GetPolygonVertexUV(poly, v, uvName, uv, unmapped);
                UV[v] = ConvertFbxUV(uv);
            }
            else {
                UV[v] = vec3(0, 0, 0);
            }
        }

        mesh->triangles.push_back(
            std::make_shared<triangle>(
                P[0], P[1], P[2],
                N[0], N[1], N[2],
                UV[0], UV[1], UV[2],
                mat,
                mesh->object_id  // correct : on réutilise l'ID du mesh
            )
        );
    }
}

static void TraverseNode(FbxNode* node, std::shared_ptr<Mesh> mesh, std::shared_ptr<material> mat)
{
    FbxNodeAttribute* att = node->GetNodeAttribute();
    FbxAMatrix globalTransform = node->EvaluateGlobalTransform();

    if (att && att->GetAttributeType() == FbxNodeAttribute::eMesh) {
        ProcessMesh((FbxMesh*)att, globalTransform, mesh, mat);
    }

    for (int i = 0; i < node->GetChildCount(); i++) {
        TraverseNode(node->GetChild(i), mesh, mat);
    }
}

std::shared_ptr<Mesh> LoadFBX(const std::string& path, std::shared_ptr<material> mat)
{
    FbxManager* manager = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);

    FbxImporter* importer = FbxImporter::Create(manager, "");

    if (!importer->Initialize(path.c_str(), -1, manager->GetIOSettings())) {
        std::cout << "FBX Import failed: "
            << importer->GetStatus().GetErrorString() << std::endl;
        return nullptr;
    }

    FbxScene* scene = FbxScene::Create(manager, "scene");
    importer->Import(scene);
    importer->Destroy();

    auto mesh = std::make_shared<Mesh>();
    mesh->mat = mat;

    // ✔ ASSIGNER L’ID UNE SEULE FOIS
    mesh->object_id = (uint64_t)(1 + rand() % 0XFFFFFF);

    TraverseNode(scene->GetRootNode(), mesh, mat);

    std::cout << "[FBX] Loaded: " << mesh->triangles.size() << " triangles\n";

    mesh->build_bvh();

    manager->Destroy();
    return mesh;
}

#endif
