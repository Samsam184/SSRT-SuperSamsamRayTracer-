#ifndef RTW_EXR_IMAGE_H
#define RTW_EXR_IMAGE_H

#include "tinyexr.h"
#include "vec3.h"
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

class rtw_exr_image {
public: 
	rtw_exr_image() = default;

	rtw_exr_image(const char* filename) {
		load(filename);
	}

	bool load(const char* filename) {
		const char* err = nullptr;
		float* out; //image data RGBA
		int w, h;

		int ret = LoadEXR(&out, &w, &h, filename, &err);
		if (ret != TINYEXR_SUCCESS) {
			std::cerr << "Failed to load EXR image" << filename << " : " << (err ? err : "unknown error") << std::endl;
			if (err) FreeEXRErrorMessage(err);
			return false;
		}

		width_ = w;
		height_ = h;
		data_.resize(w * h * 3);

		//convert rgba -> rgb
		for (int i = 0; i < w * h; i++) {
			data_[3 * i + 0] = out[4 * i + 0];
			data_[3 * i + 1] = out[4 * i + 1];
			data_[3 * i + 2] = out[4 * i + 2];
		}

		free(out);
		return true;
	}

	int width() const { return width_; }
	int height() const { return height_; }

	const float* pixel_data(int x, int y) const {
		x = std::clamp(x, 0, width_ - 1);
		y = std::clamp(y, 0, height_ - 1);
		return &data_[3 * (y * width_ + x)];
	}

private: 
	int width_ = 0;
	int height_ = 0;
	std::vector<float> data_;
};

#endif