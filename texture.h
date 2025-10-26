#ifndef TEXTURE_H
#define TEXTURE_H


#include "rtw_stb_image.h"
#include "rtw_exr_image.h"

class texture {
	public:
		virtual ~texture() = default;
		virtual color value(double u, double v, const point3& p) const = 0;
};

class solid_color : public texture {
public:
	solid_color(const color& albedo) : albedo(albedo) {}

	solid_color(double red, double green, double blue) : solid_color(color(red, green, blue)) {}

	color value(double u, double v, const point3& p) const override {
		return albedo;
	}

private: 
	color albedo;
};

class checker_texture : public texture {
public: 
	checker_texture(double scale, shared_ptr<texture> even, shared_ptr<texture> odd):
		inv_scale(1.0/scale), even(even), odd(odd) {}

	checker_texture(double scale, const color& c1, const color& c2):
		checker_texture(scale, make_shared<solid_color>(c1), make_shared<solid_color>(c2)) {}

	color value(double u, double v, const point3& p) const override {
		auto xInteger = int(std::floor(inv_scale * p.x()));
		auto yInteger = int(std::floor(inv_scale * p.y()));
		auto zInteger = int(std::floor(inv_scale * p.z()));

		bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

		return isEven ? even->value(u, v, p) : odd->value(u, v, p);
	}
private:
	double inv_scale;
	shared_ptr<texture> even;
	shared_ptr<texture> odd;
};

class other_image_texture :public texture {
public:
	other_image_texture(const char* filename) : image(filename) {}

	color value(double u, double v, const point3& p) const override {
		if (image.height() <= 0) return color(0, 1, 1);

		u = interval(0, 1).clamp(u);
		v = 1.0 - interval(0, 1).clamp(v);

		auto i = int(u * image.width());
		auto j = int(v * image.height());
		auto pixel = image.pixel_data(i, j);

		auto color_scale = 1.0 / 255.0;
		return color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
	}
private: 
	rtw_image image;
};

class exr_image_texture : public texture {
public: 
	exr_image_texture(const char* filename) : image(filename){}
	color value(double u, double v, const point3& p) const override {
		if (image.height() <= 0) return color(0, 1, 1);

		u = interval(0, 1).clamp(u);
		v = 1.0 - interval(0, 1).clamp(v);

		auto i = int(u * image.width());
		auto j = int(v * image.height());
		auto pixel = image.pixel_data(i, j);

		return color(pixel[0], pixel[1], pixel[2]);
	}
private: 
	rtw_exr_image image;
};

class image_texture : public texture {
public: 
	image_texture(const char* filename) {
		std::string name(filename);
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);

		if (ends_with(name, ".exr") || ends_with(name, ".hdr")) {
			inner_texture = std::make_shared<exr_image_texture>(filename);
		}
		else {
			inner_texture = std::make_shared<other_image_texture>(filename);
		}
	}

	color value(double u, double v, const point3& p) const override {
		if (!inner_texture) return color(1, 0, 1); //magenta pour debug
		return inner_texture->value(u, v, p);
	}
private:
	std::shared_ptr<texture>inner_texture;

	static bool ends_with(const std::string& str, const std::string& suffix) {
		if (suffix.size() > str.size()) return false;
		return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
	}
};
#endif