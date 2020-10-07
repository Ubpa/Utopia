#pragma once

#include <UGM/val.h>
#include <UGM/rgb.h>
#include <UGM/rgba.h>

#include <variant>
#include <memory>

namespace Ubpa::Utopia {
	struct Texture2D;
	class TextureCube;

	using ShaderProperty = std::variant<
		bool,                              //  0, bool
		int,                               //  1, int
		unsigned,                          //  2, uint
		float,                             //  3, float
		double,                            //  4, double
		val<bool, 2>,                      //  5, vector<bool, 2>
		val<bool, 3>,                      //  6, vector<bool, 3>
		val<bool, 4>,                      //  7, vector<bool, 4>
		val<int, 2>,                       //  8, vector<int, 2>
		val<int, 3>,                       //  9, vector<int, 3>
		val<int, 4>,                       // 10, vector<int, 4>
		val<unsigned, 2>,                  // 11, vector<unsigned, 2>
		val<unsigned, 3>,                  // 12, vector<unsigned, 3>
		val<unsigned, 4>,                  // 13, vector<unsigned, 4>
		val<float, 2>,                     // 14, vector<float, 2>
		val<float, 3>,                     // 15, vector<float, 3>
		val<float, 4>,                     // 16, vector<float, 4>
		val<double, 2>,                    // 17, vector<double, 2>
		val<double, 3>,                    // 18, vector<double, 3>
		val<double, 4>,                    // 19, vector<double, 4>
		rgbf,                              // 20, color (RGB)
		rgbaf,                             // 21, color (RGBA)
		std::shared_ptr<const Texture2D>,  // 22, Texture 2D
		std::shared_ptr<const TextureCube> // 23, Texture Cube
	>;
}
