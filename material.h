#ifndef MATERIAL_H
#define MATERIAL_H

#pragma once

#include "hittable.h"
#include "texture.h"
#include "onb.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <memory>

class material {
public: 
	virtual ~material() = default;

	virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const noexcept {
		return false;
	}

	virtual color emitted(double u, double v, const point3& p) const {
		return color(0, 0, 0);
	}

	virtual color get_base_color(double u, double v, const point3& p) const {
		return color(1, 1, 1);
	}

	double metallic = 0.0;
	double roughness = .5;

};

class lambertian : public material {
public: 

	shared_ptr<texture> tex;

	lambertian(const color& albedo) : tex(make_shared<solid_color>(albedo)) {
		metallic = 0.0;
		roughness = 1.0;
	}
	lambertian(shared_ptr<texture> tex) : tex(tex) {}

	inline __forceinline bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const noexcept override {
		auto scatter_direction = rec.normal + random_unit_vector();
		if (scatter_direction.near_zero()) {
			scatter_direction = rec.normal;
		}
		scattered = ray(rec.p, scatter_direction, r_in.time());
		attenuation = tex->value(rec.u, rec.v, rec.p);
		return true;
	}

	color get_base_color(double u, double v, const point3& p) const override {
		return tex ? tex->value(u, v, p) : color(1,1,1);
	}



private: 
	color albedo;
};

class metal : public material {
public:

	double fuzz;

	metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {
		metallic = 1.0;
		roughness = fuzz;
	}

	inline __forceinline bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const noexcept override {
		vec3 reflected = reflect(r_in.direction(), rec.normal);
		reflected = unit_vector(reflected) + (roughness * random_unit_vector());
		scattered = ray(rec.p, reflected, r_in.time());
		attenuation = albedo;
		return (dot(scattered.direction(), rec.normal) > 0);
	}

	color get_base_color(double u, double v, const point3& p) const override {
		return albedo;
	}


private: 
	color albedo;
	
};

class dielectric : public material {
public: 

	mutable bool force_scatter_false = false;

	dielectric(double refraction_index) : refraction_index(refraction_index) {
		metallic = 0.0;
		roughness = 0.0;
	}

	inline __forceinline bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const noexcept override {
		
		if (force_scatter_false) return false;
		
		attenuation = color(1.0, 1.0, 1.0);
		double ri = rec.front_face ? (1.0 / refraction_index) : refraction_index;

		vec3 unit_direction = unit_vector(r_in.direction());
		double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
		double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

		bool cannot_refract = ri * sin_theta > 1.0;
		vec3 direction;

		if (cannot_refract || reflectance(cos_theta, ri) > random_double()) {
			direction = reflect(unit_direction, rec.normal);
		}
		else {
			direction = refract(unit_direction, rec.normal, ri);
		}

		scattered = ray(rec.p, direction, r_in.time());
		return true;
	}

	color get_base_color(double, double, const point3&) const override {
		return color(1, 1, 1); 
	}


private: 
	double refraction_index;

	static double reflectance(double cosine, double refraction_index) {
		auto r0 = (1 - refraction_index) / (1 + refraction_index);
		r0 = r0 * r0;
		return r0 + (1 - r0) * std::pow((1 - cosine), 5);
	}
};

class diffuse_light : public material {
public: 
	diffuse_light(shared_ptr<texture> tex) : tex(tex){}
	diffuse_light(const color& emit) : tex(make_shared<solid_color>(emit)){}

	color emitted(double u, double v, const point3& p) const override {
		return tex->value(u, v, p);
	}

	color get_base_color(double u, double v, const point3& p) const override {
		return tex ? tex->value(u, v, p) : color(1, 1, 1);
	}


private: 
	shared_ptr<texture> tex;
};

class isotropic : public material {
public:
	isotropic(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}
	isotropic(shared_ptr<texture> tex) : tex(tex) {}

	inline __forceinline bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const noexcept override{
		scattered = ray(rec.p, random_unit_vector(), r_in.time());
		attenuation = tex->value(rec.u, rec.v, rec.p);
		return true;
	}

	color get_base_color(double u, double v, const point3& p) const override {
		return tex ? tex->value(u, v, p) : color(1, 1, 1);
	}

private: 
	shared_ptr<texture> tex;
};

class coat : public material {
public:
	std::shared_ptr<material> base;  // matériau sous-jacent (ex: lambertian, metal, etc.)
	double ior;                      // indice de réfraction de la couche
	color coat_tint;                 // teinte du vernis (blanc = neutre)
	double roughness;                // optionnel : micro-aspérité

	coat(std::shared_ptr<material> base, double ior, color tint = color(1, 1, 1), double roughness = 0.0)
		: base(base), ior(ior), coat_tint(tint), roughness(roughness) {
	}

	inline bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const noexcept override {
		// Fresnel reflectance
		double cos_theta = std::fmin(dot(-r_in.direction(), rec.normal), 1.0);
		double R0 = (1 - ior) / (1 + ior);
		R0 = R0 * R0;
		double reflect_prob = R0 + (1 - R0) * std::pow(1 - cos_theta, 5);

		if (random_double() < reflect_prob) {
			// Réflexion spéculaire (le vernis)
			vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
			scattered = ray(rec.p, reflected + roughness * random_unit_vector());
			attenuation = coat_tint;  // couleur du vernis
			return true;
		}

		// Sinon : diffusion du matériau sous-jacent
		return base->scatter(r_in, rec, attenuation, scattered);
	}

	color get_base_color(double u, double v, const point3& p) const override {
		return base ? base->get_base_color(u, v, p) : coat_tint;
	}


};

class sheen : public material {
public:
	sheen(const std::shared_ptr<material>& base, const color& tint = color(1,1,1), double roughness = .5)
		: base(base), tint(tint), roughness(roughness) { }

	inline bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const noexcept override {
		color base_atten;
		ray base_scattered;

		if (!base->scatter(r_in, rec, base_atten, base_scattered)) {
			return false;
		}

		vec3 N = rec.normal;
		vec3 V = -unit_vector(r_in.direction());
		vec3 H = unit_vector(V + random_unit_vector());
		double NoH = std::max(dot(N, H), 0.0);

		double sheen_term = std::pow(1.0 - NoH, 5.0);
		color sheen_color = tint * sheen_term * (1.0 - roughness);

		attenuation = base_atten + sheen_color;

		scattered = base_scattered;
		return true;
	}

	color get_base_color(double u, double v, const point3& p) const override {
		return base ? base->get_base_color(u, v, p) : tint;
	}


private:
	std::shared_ptr<material> base;
	color tint;
	double roughness;
};

class subsurface : public material {
public:
	subsurface(const color& albedo, double scattering_distance, double absorption = 0.1)
		: albedo(albedo), scatter_dist(scattering_distance), absorption(absorption) {
	}

	inline bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const noexcept override {
		// Vecteur de base orthonormée pour générer une direction aléatoire sous la surface
		onb uvw;
		uvw.build_from_w(rec.normal);

		// Simulation simple : direction aléatoire dans l’hémisphère *intérieure*
		vec3 dir = uvw.local(random_unit_vector());

		// Calcul d’une profondeur aléatoire (distance parcourue dans le matériau)
		double t = -scatter_dist * std::log(random_double());  // Beer-Lambert law

		// Origine du rayon légèrement *à l’intérieur* de la surface
		point3 p_inside = rec.p - rec.normal * 0.001;

		// Rayon de sortie : on considère qu’il ressort plus loin dans la même direction
		scattered = ray(p_inside + dir * t, dir, r_in.time());

		// Atténuation basée sur la distance parcourue dans le matériau
		double attenuation_factor = std::exp(-absorption * t);
		attenuation = albedo * attenuation_factor;

		return true;
	}

	color emitted(double, double, const point3&) const override {
		return color(0, 0, 0);
	}

	color get_base_color(double u, double v, const point3& p) const override {
		return albedo;
	}


private:
	color albedo;          // Couleur du matériau
	double scatter_dist;   // Distance moyenne de diffusion (ex : 0.2 -> cire, 1.0 -> lait)
	double absorption;     // Facteur d’absorption de la lumière interne
};

class thin_film : public material {
public:
	shared_ptr<material> base;
	double thickness; // en nanomètres (200–1000 nm)
	double ior_film;

	thin_film(shared_ptr<material> base, double thickness_nm, double ior_film)
		: base(base), thickness(thickness_nm * 1e-9), ior_film(ior_film) {
	}

	inline __forceinline color interference_color(double cos_theta) const noexcept {
		// Constantes optiques
		const double lambda_min = 380e-9;
		const double lambda_max = 750e-9;
		const int samples = 8;
		const double delta = (lambda_max - lambda_min) / samples;

		color result(0, 0, 0);
		const double PI = std::acos(-1.0);

		for (int i = 0; i < samples; i++) {
			double lambda = lambda_min + i * delta;
			double opd = 2.0 * ior_film * thickness * cos_theta;
			double phase = fmod(opd / lambda, 1.0);
			double intensity = 0.5 * (1.0 + cos(2 * PI * phase));
			color rgb = wavelength_to_rgb(lambda);
			result += intensity * rgb;
		}

		result[0] = std::clamp(static_cast<double>(result[0]), 0.0, 1.0);
		result[1] = std::clamp(static_cast<double>(result[1]), 0.0, 1.0);
		result[2] = std::clamp(static_cast<double>(result[2]), 0.0, 1.0);

		return result;
	}

	inline bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const noexcept override {
		// D’abord, on scatter le matériau de base
		color base_att;
		ray base_scattered;
		if (!base->scatter(r_in, rec, base_att, base_scattered))
			return false;

		// Appliquer la modulation par le film
		double cos_theta = fabs(dot(rec.normal, unit_vector(r_in.direction())));
		color film_color = interference_color(cos_theta);

		attenuation = base_att * film_color;
		scattered = base_scattered;
		return true;
	}

	color get_base_color(double u, double v, const point3& p) const override {
		return base ? base->get_base_color(u, v, p) : color(1, 1, 1);
	}


private:
	// conversion wavelength (m) -> rgb approximatif (very simple)
	static inline color wavelength_to_rgb(double lambda) noexcept {
		const double PI = std::acos(-1.0);
		double t = (lambda - 380e-9) / (750e-9 - 380e-9);

		auto s1 = std::sin(PI * t);
		auto s2 = std::sin(PI * (t + 0.33));
		auto s3 = std::sin(PI * (t + 0.66));

		double r = std::max(0.0, static_cast<double>(s1));
		double g = std::max(0.0, static_cast<double>(s2));
		double b = std::max(0.0, static_cast<double>(s3));

		return color(r, g, b);
	}

};

class opacity : public material {
public:
	shared_ptr<material> base;
	shared_ptr<texture> alpha_tex;
	double alpha_value;

	opacity(shared_ptr<material> base, double alpha = 1.0)
		: base(base), alpha_tex(nullptr), alpha_value(alpha) {
	}

	opacity(shared_ptr<material> base, shared_ptr<texture> alpha_tex)
		: base(base), alpha_tex(alpha_tex), alpha_value(1.0) {
	}

	inline bool scatter(const ray& r_in, const hit_record& rec,
		color& attenuation, ray& scattered) const noexcept override {

		double alpha = alpha_tex ? alpha_tex->value(rec.u, rec.v, rec.p).x() : alpha_value;
		alpha = std::clamp(alpha, 0.0, 1.0);

		// Test de transparence
		if (random_double() > alpha) {
			scattered = ray(rec.p, r_in.direction(), r_in.time());
			attenuation = color(1.0, 1.0, 1.0); // transparent
			return true;
		}

		return base->scatter(r_in, rec, attenuation, scattered);
	}

	color get_base_color(double u, double v, const point3& p) const override {
		return base ? base->get_base_color(u, v, p) : color(1, 1, 1);
	}


};

class bump_normal : public material {
public:
	shared_ptr<material> base;
	shared_ptr<texture> bump_tex;
	double scale;

	bump_normal(shared_ptr<material> base, shared_ptr<texture> bump_tex, double scale = 0.05)
		: base(base), bump_tex(bump_tex), scale(scale) {
	}

	inline bool scatter(const ray& r_in, const hit_record& rec,
		color& attenuation, ray& scattered) const noexcept override {
		hit_record mod_rec = rec;

		// Échantillonnage du bump map (grayscale)
		auto bump_center = bump_tex->value(rec.u, rec.v, rec.p).x();
		auto bump_u = bump_tex->value(rec.u + 0.001, rec.v, rec.p).x();
		auto bump_v = bump_tex->value(rec.u, rec.v + 0.001, rec.p).x();

		// Dérivées locales
		double dU = (bump_u - bump_center) / 0.001;
		double dV = (bump_v - bump_center) / 0.001;

		// Approximation du frame tangent (TBN)
		vec3 N = rec.normal;
		vec3 T, B;
		if (fabs(N.x()) > 0.01)
			T = unit_vector(cross(N, vec3(0, 1, 0)));
		else
			T = unit_vector(cross(N, vec3(1, 0, 0)));
		B = cross(N, T);

		// Calcul de la nouvelle normale perturbée
		vec3 perturbed = unit_vector(N + scale * (dU * T + dV * B));
		mod_rec.normal = perturbed;
		mod_rec.set_face_normal(r_in, mod_rec.normal);

		// Déléguer à la matière de base
		return base->scatter(r_in, mod_rec, attenuation, scattered);
	}

	color get_base_color(double u, double v, const point3& p) const override {
		return base ? base->get_base_color(u, v, p) : color(1, 1, 1);
	}

};

#endif 