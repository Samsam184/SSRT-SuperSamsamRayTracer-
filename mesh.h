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

class Mesh : public hittable {
public:
    std::vector<std::shared_ptr<hittable>> triangles;   // triangles bruts
    std::shared_ptr<hittable> accel;                    // BVH interne
    std::shared_ptr<material> mat;

    Mesh(const std::string& filename, std::shared_ptr<material> m) : mat(m) {
        load_obj(filename);

        if (!triangles.empty()) {
            accel = std::make_shared<bvh_node>(triangles, 0, triangles.size());   // BVH interne
        }
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept override {
        if (!accel) return false;
        return accel->hit(r, ray_t, rec);
    }

    aabb bounding_box() const override {
        if (!accel) return aabb();
        return accel->bounding_box();
    }

private:

    void load_obj(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Erreur : impossible d’ouvrir " << path << std::endl;
            return;
        }

        std::vector<vec3> positions;
        std::vector<vec3> normals;
        std::vector<vec3> texcoords;

        std::string line;
        while (std::getline(file, line)) {

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
                texcoords.emplace_back(u, v, 0);
            }
            else if (prefix == "f") {

                std::string v1, v2, v3;
                iss >> v1 >> v2 >> v3;

                auto parse_vertex = [&](const std::string& token) {
                    int p = 0, t = -1, n = -1;

                    if (token.find("//") != std::string::npos) {
                        sscanf_s(token.c_str(), "%d//%d", &p, &n);
                    }
                    else if (token.find('/') != std::string::npos) {
                        char* dup = _strdup(token.c_str());
                        int count = std::count(token.begin(), token.end(), '/');

                        if (count == 1)
                            sscanf_s(dup, "%d/%d", &p, &t);
                        else
                            sscanf_s(dup, "%d/%d/%d", &p, &t, &n);

                        free(dup);
                    }
                    else {
                        sscanf_s(token.c_str(), "%d", &p);
                    }

                    return std::tuple<int, int, int>(p - 1, t - 1, n - 1);
                    };

                auto [p1, t1, n1] = parse_vertex(v1);
                auto [p2, t2, n2] = parse_vertex(v2);
                auto [p3, t3, n3] = parse_vertex(v3);

                vec3 nA = (n1 >= 0 && n1 < (int)normals.size()) ? normals[n1] : vec3(0, 1, 0);
                vec3 nB = (n2 >= 0 && n2 < (int)normals.size()) ? normals[n2] : vec3(0, 1, 0);
                vec3 nC = (n3 >= 0 && n3 < (int)normals.size()) ? normals[n3] : vec3(0, 1, 0);

                vec3 tA = (t1 >= 0 && t1 < (int)texcoords.size()) ? texcoords[t1] : vec3(0, 0, 0);
                vec3 tB = (t2 >= 0 && t2 < (int)texcoords.size()) ? texcoords[t2] : vec3(0, 0, 0);
                vec3 tC = (t3 >= 0 && t3 < (int)texcoords.size()) ? texcoords[t3] : vec3(0, 0, 0);

                triangles.push_back(std::make_shared<triangle>(
                    positions[p1], positions[p2], positions[p3],
                    nA, nB, nC,
                    tA, tB, tC,
                    mat
                ));
            }
        }

        std::cout << "Mesh chargé : " << triangles.size() << " triangles\n";
    }
};

#endif
