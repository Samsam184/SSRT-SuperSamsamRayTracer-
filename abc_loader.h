#ifndef ABC_LOADER_H
#define ABC_LOADER_H

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <memory>
#include <string>
#include <iostream>
#include "mesh.h"
#include "triangle.h"
#include "vec3.h"

using namespace Alembic::Abc;
using namespace Alembic::AbcGeom;

struct TriBuild {
    int i0, i1, i2;   // indices vertex
    vec3 uv0, uv1, uv2;
};

std::shared_ptr<Mesh> LoadABC(const std::string& path, std::shared_ptr<material> mat)
{
    Alembic::AbcCoreFactory::IFactory factory;
    Alembic::AbcCoreFactory::IFactory::CoreType coreType;

    IArchive archive = factory.getArchive(path, coreType);
    if (!archive.valid()) {
        std::cout << "Alembic open failed: " << path << "\n";
        return nullptr;
    }

    std::cout << "Alembic loaded: " << path << "\n";

    IObject root = archive.getTop();

    auto mesh = std::make_shared<Mesh>();
    mesh->mat = mat;
    mesh->object_id = 1 + (uint32_t)(rand() % 0xFFFFFF);

    std::function<void(IObject)> traverse;
    traverse = [&](IObject obj)
        {
            if (!obj.valid()) return;

            if (IPolyMesh::matches(obj.getMetaData()))
            {
                IPolyMesh pm(obj, kWrapExisting);
                IPolyMeshSchema& schema = pm.getSchema();
                IPolyMeshSchema::Sample sample = schema.getValue();

                // --- POSITIONS ---
                auto P = sample.getPositions();
                const auto* pos = P->get();
                size_t Pcount = P->size();

                // --- FACE INDICES ---
                auto FI = sample.getFaceIndices();
                const int32_t* indices = FI->get();

                // --- FACE COUNTS ---
                auto FC = sample.getFaceCounts();
                const int32_t* counts = FC->get();
                size_t Fcount = FC->size();

                // --- UV ---
                IV2fGeomParam uvParam = schema.getUVsParam();
                bool hasUV = uvParam.valid();

                const V2f* uvPtr = nullptr;
                Alembic::AbcGeom::GeometryScope uvScope = kUnknownScope;

                if (hasUV) {
                    auto UVsample = uvParam.getExpandedValue();
                    uvPtr = UVsample.getVals()->get();
                    uvScope = uvParam.getScope();
                }

                // Listes propres pour reconstruire normales
                std::vector<TriBuild> triList;
                triList.reserve(Fcount * 2);

                // Construction triangulation + mapping vertex
                size_t cursor = 0;
                for (size_t f = 0; f < Fcount; f++)
                {
                    int count = counts[f];
                    if (count < 3) { cursor += count; continue; }

                    for (int i = 1; i + 1 < count; i++)
                    {
                        TriBuild T;
                        T.i0 = indices[cursor + 0];
                        T.i1 = indices[cursor + i];
                        T.i2 = indices[cursor + i + 1];

                        // UV if facevarying or vertex
                        if (hasUV)
                        {
                            if (uvScope == kFacevaryingScope) {
                                T.uv0 = vec3(uvPtr[cursor + 0].x, uvPtr[cursor + 0].y, 0);
                                T.uv1 = vec3(uvPtr[cursor + i].x, uvPtr[cursor + i].y, 0);
                                T.uv2 = vec3(uvPtr[cursor + i + 1].x, uvPtr[cursor + i + 1].y, 0);
                            }
                            else { // vertex
                                T.uv0 = vec3(uvPtr[T.i0].x, uvPtr[T.i0].y, 0);
                                T.uv1 = vec3(uvPtr[T.i1].x, uvPtr[T.i1].y, 0);
                                T.uv2 = vec3(uvPtr[T.i2].x, uvPtr[T.i2].y, 0);
                            }
                        }
                        else {
                            T.uv0 = T.uv1 = T.uv2 = vec3(0, 0, 0);
                        }

                        triList.push_back(T);
                    }

                    cursor += count;
                }

                // --- CALCUL DES NORMALES OPTIMISÉ ---
                std::vector<vec3> accumNormals(Pcount, vec3(0, 0, 0));

                for (auto& t : triList)
                {
                    vec3 P0(pos[t.i0].x, pos[t.i0].y, pos[t.i0].z);
                    vec3 P1(pos[t.i1].x, pos[t.i1].y, pos[t.i1].z);
                    vec3 P2(pos[t.i2].x, pos[t.i2].y, pos[t.i2].z);

                    vec3 Ng = unit_vector(cross(P1 - P0, P2 - P0));

                    accumNormals[t.i0] += Ng;
                    accumNormals[t.i1] += Ng;
                    accumNormals[t.i2] += Ng;
                }

                for (auto& n : accumNormals)
                    n = unit_vector(n);

                // --- CRÉATION FINALE DES TRIANGLES ---
                for (auto& t : triList)
                {
                    vec3 P0(pos[t.i0].x, pos[t.i0].y, pos[t.i0].z);
                    vec3 P1(pos[t.i1].x, pos[t.i1].y, pos[t.i1].z);
                    vec3 P2(pos[t.i2].x, pos[t.i2].y, pos[t.i2].z);

                    vec3 N0 = accumNormals[t.i0];
                    vec3 N1 = accumNormals[t.i1];
                    vec3 N2 = accumNormals[t.i2];

                    mesh->triangles.push_back(
                        std::make_shared<triangle>(P0, P1, P2, N0, N1, N2, t.uv0, t.uv1, t.uv2, mat, mesh->object_id)
                    );
                }
            }

            for (size_t i = 0; i < obj.getNumChildren(); i++)
                traverse(obj.getChild(i));
        };

    traverse(root);

    std::cout << "ABC triangles: " << mesh->triangles.size() << "\n";
    mesh->build_bvh();

    return mesh;
}

#endif
