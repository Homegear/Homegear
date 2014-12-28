/* Copyright 2013-2015 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef COLOR_H_
#define COLOR_H_

#include "Math.h"

#include <cmath>
#include <string>

namespace BaseLib
{

class Color
{
public:
	class NormalizedRGB;
	class HSV;

	/**
	 * Class defining a RGB color with intensity values for each color between 0 and 255.
	 */
	class RGB
	{
	public:
		RGB() {}

		/**
		 * Constructor taking a RGB string in the formats "#C8C8C8", "200,200,200" or "C8,C8,C8".
		 *
		 * @param rgbString The RGB string to convert.
		 */
		RGB(const std::string& rgbString);

		/**
		 * Constructor taking a normalized RGB class with color intensities between 0 and 1 which are being converted to an intensity between 0 and 255.
		 *
		 * @param rgb The rgb class with normalized color intensities.
		 */
		RGB(NormalizedRGB& rgb) { setRed(rgb.getRed()); setGreen(rgb.getGreen()); setBlue(rgb.getBlue()); }
		RGB(uint8_t red, uint8_t green, uint8_t blue) { setRed(red); setGreen(green); setBlue(blue); }
		virtual ~RGB() {}

		bool opacityDefined() { return _opacityDefined; };
		uint8_t getOpacity() const { return _opacity; }

		/**
		 * Converts a normalized opacity between 0 and 1 to an opacity between 0 and 255 and sets it.
		 *
		 * @param value The opacity between 0 and 1.
		 */
		void setOpacity(double value) { _opacityDefined = true; _opacity = value * 255; }
		void setOpacity(uint8_t value) { _opacityDefined = true; _opacity = value; }

		uint8_t getRed() const { return _red; }

		/**
		 * Converts a normalized intensity between 0 and 1 to an intensity between 0 and 255 and sets the intensity of red.
		 *
		 * @param value The normalized intensity of red between 0 and 1.
		 */
		void setRed(double value) { _red = value * 255; }
		void setRed(uint8_t value) { _red = value; }
		uint8_t getGreen() const { return _green; }

		/**
		 * Converts a normalized intensity between 0 and 1 to an intensity between 0 and 255 and sets the intensity of green.
		 *
		 * @param value The normalized intensity of green between 0 and 1.
		 */
		void setGreen(double value) { _green = value * 255; }
		void setGreen(uint8_t value) { _green = value; }
		uint8_t getBlue() const { return _blue; }

		/**
		 * Converts a normalized intensity between 0 and 1 to an intensity between 0 and 255 and sets the intensity of blue.
		 *
		 * @param value The normalized intensity of blue between 0 and 1.
		 */
		void setBlue(double value) { _blue = value * 255; }
		void setBlue(uint8_t value) { _blue = value; }

		std::string toString();
	protected:
		bool _opacityDefined = false;
		uint8_t _opacity = 255;
		uint8_t _red = 0;
		uint8_t _green = 0;
		uint8_t _blue = 0;
	};

	/**
	 * Class defining a RGB color with intensity values for each color between 0 and 1.
	 */
	class NormalizedRGB
	{
	public:
		NormalizedRGB() {}

		/**
		 * Constructor taking intensities between 0 and 255 which are being converted to the normalized intensity between 0 and 1.
		 *
		 * @param red Intensity of red between 0 and 255.
		 * @param red Intensity of green between 0 and 255.
		 * @param red Intensity of blue between 0 and 255.
		 */
		NormalizedRGB(uint8_t red, uint8_t green, uint8_t blue) { setRed(red); setGreen(green); setBlue(blue); }

		/**
		 * Constructor taking a not normalized RGB class with color intensities between 0 and 255 which are being converted to the normalized intensity between 0 and 1.
		 *
		 * @param rgb The rgb class.
		 */
		NormalizedRGB(RGB& rgb) { setRed(rgb.getRed()); setGreen(rgb.getGreen()); setBlue(rgb.getBlue()); }
		NormalizedRGB(double red, double green, double blue) { setRed(red); setGreen(green); setBlue(blue); }
		virtual ~NormalizedRGB() {}

		double getRed() const { return _red; }

		/**
		 * Converts an intensity between 0 and 255 to the normalized intensity between 0 and 1 and sets the intensity of red.
		 *
		 * @param value The intensity of red between 0 and 255.
		 */
		void setRed(uint8_t value) { _red = ((double)value) / 255; }
		void setRed(double value) { _red = value; if(_red < 0) { _red = 0; } else if(_red > 1) { _red = 1; } }
		double getGreen() const { return _green; }

		/**
		 * Converts an intensity between 0 and 255 to the normalized intensity between 0 and 1 and sets the intensity of green.
		 *
		 * @param value The intensity of green between 0 and 255.
		 */
		void setGreen(uint8_t value) { _green = ((double)value) / 255; }
		void setGreen(double value) { _green = value; if(_green < 0) { _green = 0; } else if(_green > 1) { _green = 1; } }
		double getBlue() const { return _blue; }

		/**
		 * Converts an intensity between 0 and 255 to the normalized intensity between 0 and 1 and sets the intensity of blue.
		 *
		 * @param value The intensity of blue between 0 and 255.
		 */
		void setBlue(uint8_t value) { _blue = ((double)value) / 255; }
		void setBlue(double value) { _blue = value; if(_blue < 0) { _blue = 0; } else if(_blue > 1) { _blue = 1; } }

		/**
		 * Converts this RGB color to HSV/HSB (Hue/Saturation/Value or Hue/Saturation/Brightness).
		 */
		HSV toHSV();
	protected:
		double _red = 0;
		double _green = 0;
		double _blue = 0;
	};

	/**
	 * Class defining a HSV color.
	 */
	class HSV
	{
	public:
		HSV() {}
		HSV(double hue, double saturation, double brightness) { setHue(hue); setSaturation(saturation); setBrightness(brightness); }
		virtual ~HSV() {}

		double getHue() const { return _hue; }
		void setHue(double value) { _hue = value; if(_hue < 0) { _hue = 0; } else { _hue = std::fmod(_hue, 360.0); } }
		double getSaturation() const { return _saturation; }
		void setSaturation(double value) { _saturation = value; if(_saturation < 0) { _saturation = 0; } else if(_saturation > 1) { _saturation = 1; } }
		double getBrightness() const { return _brightness; }
		void setBrightness(double value) { _brightness = value; if(_brightness < 0) { _brightness = 0; } else if(_brightness > 1) { _brightness = 1; } }

		RGB toRGB();
	protected:
		double _hue = 0;
		double _saturation = 0;
		double _brightness = 0;
	};

	/**
	 * Destructor.
	 * Does nothing.
	 */
	virtual ~Color();

	/**
	 * Gets the RGB to XYZ and XYZ to RGB conversion matrix for the specified chromaticity coordinates of an RGB system with D65 as reference white.
	 *
	 * @see <a href="http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html">RGB/XYZ Matrices</a>
	 * @see <a href="http://www.babelcolor.com/download/A%20review%20of%20RGB%20color%20spaces.pdf">A Review of RGB Color Spaces</a>
	 * @param[in] gamut The chromaticity coordinates of an RGB system to get the conversion matrix for. First point is the coordinate for red, the second point for green and the third point for blue.
	 * @param[out] conversionMatrix The RGB to XYZ conversion matrix.
	 * @param[out] inversedConversionMatrix The XYZ to RGB conversion matrix.
	 */
	static void getConversionMatrix(const Math::Triangle& gamut, Math::Matrix3x3& conversionMatrix, Math::Matrix3x3& inversedConversionMatrix);

	/**
	 * Converts a RGB value to it's CIE 1931 color space chromaticity coordinates (x, y).
	 *
	 * @see <a href="https://github.com/mikz/PhilipsHueSDKiOS/blob/master/ApplicationDesignNotes/RGB%20to%20xy%20Color%20conversion.md">iOS Philips hue SDK</a>
	 * @see <a href="http://www.brucelindbloom.com/Eqn_RGB_XYZ_Matrix.html">Some Common RGB Working Space Matrices</a>
	 * @see <a href="http://www.brucelindbloom.com/WorkingSpaceInfo.html">RGB Working Space Information</a>
	 * @param[in] rgb Normalized RGB intensities.
	 * @param[in] conversionMatrix The RGB to XYZ conversion matrix.
	 * @param[in] gamma The gamma of the RGB color space.
	 * @param[out] xy Coordinate in CIE 1931 color space.
	 * @param[out] brightness The brightness between 0 and 1.
	 */
	static void rgbToCie1931Xy(const NormalizedRGB& rgb, const Math::Matrix3x3& conversionMatrix, const double& gamma, Math::Point2D& xy, double& brightness);

	/**
	 * Converts CIE 1931 color space chromaticity coordinates (x, y) to RGB as reference white.
	 *
	 * @see <a href="https://github.com/mikz/PhilipsHueSDKiOS/blob/master/ApplicationDesignNotes/RGB%20to%20xy%20Color%20conversion.md">iOS Philips hue SDK</a>
	 * @see <a href="http://www.brucelindbloom.com/Eqn_RGB_XYZ_Matrix.html">Some Common RGB Working Space Matrices</a>
	 * @see <a href="http://www.brucelindbloom.com/WorkingSpaceInfo.html">RGB Working Space Information</a>
	 * @param[in] xy Coordinate in CIE 1931 color space.
	 * @param[in] brightness The brightness between 0 and 1.
	 * @param[in] conversionMatrix The XYZ to RGB conversion matrix.
	 * @param[in] gamma The gamma of the RGB color space.
	 * @param[out] rgb The resulting normalized RGB intensities.
	 */
	static void cie1931XyToRgb(const Math::Point2D& xy, const double& brightness, const Math::Matrix3x3& conversionMatrix, const double& gamma, NormalizedRGB& rgb);
protected:
	/**
	 * Constructor. It is protected, because the class only contains static methods.
	 * It does nothing.
	 */
	Color();
};

}

#endif
