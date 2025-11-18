#ifndef MESH_H
#define MESH_H

#include "hittable.h"
#include "triangle.h"
#include "material.h"
#include "bvh.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <tuple>
#include <iostream>

class Mesh : public hittable {
public:
    std::vector<std::shared_ptr<hittable>> triangles;   // triangles du mesh
    std::shared_ptr<hittable> accel;                    // BVH interne
    std::shared_ptr<material> mat;
    aabb bbox; 

    uint32_t object_id = 0;

    Mesh() : mat(nullptr), accel(nullptr), bbox() {
        // mesh vide, on push les triangles depuis le loader
    }


    Mesh(const std::string& filename, std::shared_ptr<material> m)
        : mat(m)
    {
        
        object_id = 1 + (uint32_t)(rand() % 0xFFFFFF);

        load_obj(filename);

        if (!triangles.empty()) {
            accel = std::make_shared<bvh_node>(triangles, 0, triangles.size());
            bbox = accel->bounding_box();
            
        }
        else {
            accel = nullptr;
            bbox = aabb();
        }


    }


    bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept override {
        if (!accel) return false;
        return accel->hit(r, ray_t, rec);
    }


    aabb bounding_box() const override {
        return bbox;
    }


    void build_bvh() {
        if (triangles.empty()) {
            accel.reset();
            bbox = aabb();
            return;
        }
        accel = std::make_shared<bvh_node>(triangles, 0, triangles.size());
        bbox = accel->bounding_box();
    }


    void add_triangle(
        const vec3& A, const vec3& B, const vec3& C,
        const vec3& nA, const vec3& nB, const vec3& nC,
        const vec3& uvA, const vec3& uvB, const vec3& uvC, uint32_t object_id
    ) {

        triangles.push_back(
            std::make_shared<triangle>(A, B, C, nA, nB, nC, uvA, uvB, uvC, mat, object_id)
        );
       

    }



private:

    struct Face {
        int p1, p2, p3;
        int t1, t2, t3;
        int n1, n2, n3;
    };
    std::vector<Face> face_list;

    // remplace l'ancienne load_obj par ceci
    void load_obj(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Erreur : impossible d’ouvrir " << path << std::endl;
            return;
        }

        std::vector<vec3> positions;
        std::vector<vec3> normals;
        std::vector<vec3> texcoords;
        std::vector<Face> face_list;

        std::string line;
        while (std::getline(file, line)) {
            // skip empty / comment lines quickly
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v") {
                double x, y, z;
                iss >> x >> y >> z;
                positions.emplace_back(x, y, z);
            }
            else if (prefix == "vn") {
                double x, y, z;
                iss >> x >> y >> z;
                normals.emplace_back(x, y, z);
            }
            else if (prefix == "vt") {
                double u, v;
                iss >> u >> v;
                texcoords.emplace_back(u, v, 0.0);
            }
            else if (prefix == "f") {
                // supporte uniquement triangles (v v/vt v/vt/vn v//vn)
                std::string tok;
                std::vector<std::string> tokens;
                while (iss >> tok) tokens.push_back(tok);
                if (tokens.size() < 3) continue;

                // triangule un éventuel polygon (fan)
                for (size_t k = 1; k + 1 < tokens.size(); ++k) {
                    auto parse_vertex = [&](const std::string& token) {
                        int p = 0, t = -1, n = -1;
                        if (token.find("//") != std::string::npos) {
                            sscanf_s(token.c_str(), "%d//%d", &p, &n);
                        }
                        else if (token.find('/') != std::string::npos) {
                            int count = std::count(token.begin(), token.end(), '/');
                            if (count == 1)
                                sscanf_s(token.c_str(), "%d/%d", &p, &t);
                            else
                                sscanf_s(token.c_str(), "%d/%d/%d", &p, &t, &n);
                        }
                        else {
                            sscanf_s(token.c_str(), "%d", &p);
                        }
                        return std::tuple<int, int, int>(p - 1, t - 1, n - 1);
                        };

                    auto [p1, t1, n1] = parse_vertex(tokens[0]);
                    auto [p2, t2, n2] = parse_vertex(tokens[k]);
                    auto [p3, t3, n3] = parse_vertex(tokens[k + 1]);

                    face_list.push_back({ p1,p2,p3, t1,t2,t3, n1,n2,n3 });
                }
            }
        }

        // si y'a pas de normales dans le fichier, on en calcule des "smooth" (moyenne des faces)
        if (normals.empty()) {
            normals.resize(positions.size(), vec3(0, 0, 0));
            for (auto& f : face_list) {
                // safe-guards (indices valides)
                if (f.p1 < 0 || f.p2 < 0 || f.p3 < 0) continue;
                vec3 A = positions[f.p1];
                vec3 B = positions[f.p2];
                vec3 C = positions[f.p3];
                vec3 Ng = cross(B - A, C - A);
                // si la face est dégénérée, skip
                if (Ng.length() < 1e-12) continue;
                Ng = unit_vector(Ng);
                normals[f.p1] += Ng;
                normals[f.p2] += Ng;
                normals[f.p3] += Ng;
            }
            for (auto& n : normals) {
                if (n.length() > 1e-12) n = unit_vector(n);
                else n = vec3(0, 1, 0);
            }
        }

        // maintenant on crée les triangles à partir de face_list
        triangles.clear();
        triangles.reserve(face_list.size());
        for (auto& f : face_list) {
            // indice safe-guards
            if (f.p1 < 0 || f.p2 < 0 || f.p3 < 0) continue;
            vec3 nA = (f.n1 >= 0 && f.n1 < (int)normals.size()) ? normals[f.n1] : normals[f.p1];
            vec3 nB = (f.n2 >= 0 && f.n2 < (int)normals.size()) ? normals[f.n2] : normals[f.p2];
            vec3 nC = (f.n3 >= 0 && f.n3 < (int)normals.size()) ? normals[f.n3] : normals[f.p3];

            vec3 tA = (f.t1 >= 0 && f.t1 < (int)texcoords.size()) ? texcoords[f.t1] : vec3(0, 0, 0);
            vec3 tB = (f.t2 >= 0 && f.t2 < (int)texcoords.size()) ? texcoords[f.t2] : vec3(0, 0, 0);
            vec3 tC = (f.t3 >= 0 && f.t3 < (int)texcoords.size()) ? texcoords[f.t3] : vec3(0, 0, 0);

            triangles.push_back(std::make_shared<triangle>(
                positions[f.p1], positions[f.p2], positions[f.p3],
                nA, nB, nC,
                tA, tB, tC,
                mat,
                object_id
            ));
        }

        std::cout << "Mesh chargé : " << triangles.size() << " triangles\n";
    }


};



#endif
