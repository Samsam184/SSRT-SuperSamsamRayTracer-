#ifndef BVH_SAH_H
#define BVH_SAH_H

#include <vector>
#include <algorithm>
#include <limits>
#include <stack>
#include <memory>
#include <iostream>
#include <vector>

#include "hittable.h"
#include "hittable_list.h"
#include "aabb.h"
#include "vec3.h"

using point3 = vec3;
// BVH aplati construit avec SAH (binnings).
// Compatible avec : hittable, hittable_list, aabb (accès aux champs x,y,z).

class BVH_SAH : public hittable {
public:
    // Construire directement depuis un hittable_list
    BVH_SAH(const hittable_list& world, int max_leaf_size = 4, int bins = 16) {
        build(world.objects, max_leaf_size, bins);
    }

    // Pas de default ctor recommandé, mais on fournit un fallback
    BVH_SAH() = default;

    // hittable interface
    inline __forceinline bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept override {
        if (nodes.empty()) return false;

        bool hit_any = false;
        double closest = ray_t.max;

        // pile de parcours (itératif)
        int stack_arr[64];
        int sp = 0;
        stack_arr[sp++] = 0; // start at root

        while (sp > 0) {
            int node_idx = stack_arr[--sp];
            const Node& node = nodes[node_idx];

            // test AABB avec interval limité à closest
            if (!node.bbox.hit(r, interval(ray_t.min, closest)))
                continue;

            if (node.is_leaf()) {
                for (int i = node.start; i < node.start + node.count; ++i) {
                    int prim_idx = prim_indices[i];
                    hit_record tmp;
                    if (objects[prim_idx]->hit(r, interval(ray_t.min, closest), tmp)) {
                        hit_any = true;
                        closest = tmp.t;
                        rec = tmp;
                    }
                }
            }
            else {
                // push children (push right then left so left processed first)
                stack_arr[sp++] = node.right;
                stack_arr[sp++] = node.left;
                // sp should stay small; if big scenes, consider std::vector stack
            }
        }
        return hit_any;
    }

    inline aabb bounding_box() const override {
        if (nodes.empty()) return aabb();
        return nodes[0].bbox;
    }

private:
    struct Node {
        aabb bbox;
        int start = 0;   // index in prim_indices when leaf
        int count = 0;   // >0 => leaf
        int left = -1;   // child idx (internal)
        int right = -1;  // child idx (internal)

        bool is_leaf() const { return count > 0; }
    };

    std::vector<Node> nodes;
    std::vector<const hittable*> objects;    // pointers vers primitives
    std::vector<int> prim_indices;           // indices permutation des objects

    // surface area d'un aabb (pour SA)
    static inline double surface_area(const aabb& b) noexcept {
        const double dx = b.x.size();
        const double dy = b.y.size();
        const double dz = b.z.size();
        return 2.0 * (dx * dy + dx * dz + dy * dz);
    }

    // helper : bbox combine (safe) - si aabb::surrounding_box existe, on utilise
    static inline aabb surround(const aabb& a, const aabb& b) noexcept {
        // on suppose que aabb a.x.min / a.x.max etc sont accessibles
        point3 small(std::fmin(a.x.min, b.x.min),
                     std::fmin(a.y.min, b.y.min),
                     std::fmin(a.z.min, b.z.min));
        point3 big(std::fmax(a.x.max, b.x.max),
                   std::fmax(a.y.max, b.y.max),
                   std::fmax(a.z.max, b.z.max));
        return aabb(small, big);
    }

    // Build public wrapper
    void build(const std::vector<std::shared_ptr<hittable>>& src_objects, int max_leaf_size, int nbins) {
        
        objects.clear();
        objects.reserve(src_objects.size());
        for (const auto& s : src_objects) objects.push_back(s.get());

        const int n = (int)objects.size();
        prim_indices.resize(n);
        for (int i = 0; i < n; ++i) prim_indices[i] = i;

        nodes.clear();
        nodes.reserve(std::max(4, n * 2));

        if (n == 0) return;

        // build recursively with SAH
        build_recursive(0, n, max_leaf_size, nbins);

        // Debug log
        std::clog << "[BVH_SAH] built nodes: " << nodes.size() << " primitives: " << n << "\n";
    }

    // Structure temporaire pour bins
    struct Bin {
        aabb bbox;
        int count = 0;
    };

    // recursive build returns node index
    int build_recursive(int start, int end, int max_leaf_size, int nbins) {
        // create node and compute bbox and centroid bbox
        aabb node_bbox;
        bool first = true;
        aabb centroid_bbox;
        bool first_cent = true;

        for (int i = start; i < end; ++i) {
            int obj_idx = prim_indices[i];
            aabb b = objects[obj_idx]->bounding_box();
            if (first) { node_bbox = b; first = false; }
            else node_bbox = surround(node_bbox, b);

            // centroid
            point3 c(0.5 * (b.x.min + b.x.max),
                0.5 * (b.y.min + b.y.max),
                0.5 * (b.z.min + b.z.max));
            aabb cb(c, c);
            if (first_cent) { centroid_bbox = cb; first_cent = false; }
            else centroid_bbox = surround(centroid_bbox, cb);
        }

        Node node;
        node.bbox = node_bbox;

        int count = end - start;
        if (count <= max_leaf_size) {
            // make leaf
            node.start = start;
            node.count = count;
            int my_idx = (int)nodes.size();
            nodes.push_back(node);
            return my_idx;
        }

        // choose axis by largest centroid extent
        double cdx = centroid_bbox.x.size();
        double cdy = centroid_bbox.y.size();
        double cdz = centroid_bbox.z.size();
        int axis = 0;
        if (cdy > cdx && cdy >= cdz) axis = 1;
        else if (cdz > cdx && cdz >= cdy) axis = 2;

        // if centroids are degenerate (no extent), make leaf
        if ((axis == 0 && cdx <= 1e-12) || (axis == 1 && cdy <= 1e-12) || (axis == 2 && cdz <= 1e-12)) {
            node.start = start;
            node.count = count;
            int my_idx = (int)nodes.size();
            nodes.push_back(node);
            return my_idx;
        }

        // Binning for SAH
        std::vector<Bin> bins(nbins);
        for (int i = start; i < end; ++i) {
            int obj_idx = prim_indices[i];
            aabb b = objects[obj_idx]->bounding_box();
            point3 c(0.5 * (b.x.min + b.x.max),
                0.5 * (b.y.min + b.y.max),
                0.5 * (b.z.min + b.z.max));
            double coord = (axis == 0) ? c.x() : (axis == 1) ? c.y() : c.z();

            double minc = (axis == 0) ? centroid_bbox.x.min : (axis == 1) ? centroid_bbox.y.min : centroid_bbox.z.min;
            double maxc = (axis == 0) ? centroid_bbox.x.max : (axis == 1) ? centroid_bbox.y.max : centroid_bbox.z.max;
            double t = (coord - minc) / (maxc - minc);
            int bidx = std::min(nbins - 1, std::max(0, (int)(t * nbins)));
            Bin& B = bins[bidx];
            if (B.count == 0) B.bbox = b;
            else B.bbox = surround(B.bbox, b);
            B.count++;
        }

        // prefix/suffix sums to compute SAH for splits
        std::vector<int> left_count(nbins - 1), right_count(nbins - 1);
        std::vector<aabb> left_bbox(nbins - 1), right_bbox(nbins - 1);

        // left cumulative
        int acc = 0;
        aabb acc_bbox;
        bool acc_first = true;
        for (int i = 0; i < nbins - 1; ++i) {
            if (bins[i].count > 0) {
                if (acc_first) { acc_bbox = bins[i].bbox; acc_first = false; }
                else acc_bbox = surround(acc_bbox, bins[i].bbox);
            }
            acc += bins[i].count;
            left_count[i] = acc;
            left_bbox[i] = acc_bbox;
        }

        // right cumulative
        acc = 0;
        acc_first = true;
        for (int i = nbins - 1; i >= 1; --i) {
            if (bins[i].count > 0) {
                if (acc_first) { acc_bbox = bins[i].bbox; acc_first = false; }
                else acc_bbox = surround(acc_bbox, bins[i].bbox);
            }
            acc += bins[i].count;
            right_count[i - 1] = acc;
            right_bbox[i - 1] = acc_bbox;
        }

        // evaluate SAH cost for each split
        double best_cost = std::numeric_limits<double>::infinity();
        int best_split = -1;
        double inv_total_area = 1.0 / surface_area(node_bbox);
        for (int i = 0; i < nbins - 1; ++i) {
            if (left_count[i] == 0 || right_count[i] == 0) continue;
            double cost = 0.125 + // traversal cost (heuristic constant)
                (left_count[i] * surface_area(left_bbox[i]) + right_count[i] * surface_area(right_bbox[i])) * inv_total_area;
            if (cost < best_cost) {
                best_cost = cost;
                best_split = i;
            }
        }

        // If no good split found -> fallback median split by centroid
        if (best_split == -1) {
            int mid = (start + end) / 2;
            std::nth_element(prim_indices.begin() + start, prim_indices.begin() + mid, prim_indices.begin() + end,
                [this, axis](int ia, int ib) {
                    aabb ba = objects[ia]->bounding_box();
                    aabb bb = objects[ib]->bounding_box();
                    double ca = 0.5 * ((axis == 0) ? (ba.x.min + ba.x.max) : (axis == 1) ? (ba.y.min + ba.y.max) : (ba.z.min + ba.z.max));
                    double cb = 0.5 * ((axis == 0) ? (bb.x.min + bb.x.max) : (axis == 1) ? (bb.y.min + bb.y.max) : (bb.z.min + bb.z.max));
                    return ca < cb;
                }
            );
            // create internal node and recurse
            int my_idx = (int)nodes.size();
            nodes.push_back(node); // placeholder
            int left = build_recursive(start, (start + end) / 2, max_leaf_size, nbins);
            int right = build_recursive((start + end) / 2, end, max_leaf_size, nbins);
            nodes[my_idx].left = left;
            nodes[my_idx].right = right;
            nodes[my_idx].count = 0;
            nodes[my_idx].bbox = node_bbox;
            return my_idx;
        }

        // partition by best_split: compute split key
        double minc = (axis == 0) ? centroid_bbox.x.min : (axis == 1) ? centroid_bbox.y.min : centroid_bbox.z.min;
        double maxc = (axis == 0) ? centroid_bbox.x.max : (axis == 1) ? centroid_bbox.y.max : centroid_bbox.z.max;
        double split_pos = minc + ((best_split + 1) / double(nbins)) * (maxc - minc);

        // partition prim_indices in place according to split_pos
        auto mid_it = std::partition(prim_indices.begin() + start, prim_indices.begin() + end,
            [this, axis, split_pos](int idx) {
                aabb b = objects[idx]->bounding_box();
                double c = 0.5 * ((axis == 0) ? (b.x.min + b.x.max) : (axis == 1) ? (b.y.min + b.y.max) : (b.z.min + b.z.max));
                return c < split_pos;
            }
        );

        int mid = (int)std::distance(prim_indices.begin(), mid_it);
        if (mid == start || mid == end) {
            // bad partition (all on one side) -> fallback median
            mid = (start + end) / 2;
            std::nth_element(prim_indices.begin() + start, prim_indices.begin() + mid, prim_indices.begin() + end,
                [this, axis](int ia, int ib) {
                    aabb ba = objects[ia]->bounding_box();
                    aabb bb = objects[ib]->bounding_box();
                    double ca = 0.5 * ((axis == 0) ? (ba.x.min + ba.x.max) : (axis == 1) ? (ba.y.min + ba.y.max) : (ba.z.min + ba.z.max));
                    double cb = 0.5 * ((axis == 0) ? (bb.x.min + bb.x.max) : (axis == 1) ? (bb.y.min + bb.y.max) : (bb.z.min + bb.z.max));
                    return ca < cb;
                }
            );
        }

        // create internal node
        int my_idx = (int)nodes.size();
        nodes.push_back(node); // placeholder

        int left_idx = build_recursive(start, mid, max_leaf_size, nbins);
        int right_idx = build_recursive(mid, end, max_leaf_size, nbins);

        nodes[my_idx].left = left_idx;
        nodes[my_idx].right = right_idx;
        nodes[my_idx].count = 0;
        nodes[my_idx].bbox = node_bbox;
        return my_idx;
    }
};

#endif // BVH_SAH_H
