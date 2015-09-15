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

#include "Color.h"
#include "HelperFunctions.h"
#include <iostream>

namespace BaseLib
{

Color::RGB::RGB(const std::string& rgbString)
{
	if((signed)rgbString.find(',') != (signed)std::string::npos)
	{
		std::vector<std::string> arrayElements = HelperFunctions::splitAll(rgbString, ',');
		if(arrayElements.size() == 3)
		{
			_red = Math::getNumber(arrayElements[0]);
			_green = Math::getNumber(arrayElements[1]);
			_blue = Math::getNumber(arrayElements[2]);
		}
		else if(arrayElements.size() == 4)
		{
			_opacityDefined = true;
			_opacity = Math::getNumber(arrayElements[0]);
			_red = Math::getNumber(arrayElements[1]);
			_green = Math::getNumber(arrayElements[2]);
			_blue = Math::getNumber(arrayElements[3]);
		}
	}
	else if((signed)rgbString.find(';') != (signed)std::string::npos)
	{
		std::vector<std::string> arrayElements = HelperFunctions::splitAll(rgbString, ';');
		if(arrayElements.size() == 3)
		{
			_red = Math::getNumber(arrayElements[0]);
			_green = Math::getNumber(arrayElements[1]);
			_blue = Math::getNumber(arrayElements[2]);
		}
		else if(arrayElements.size() == 4)
		{
			_opacityDefined = true;
			_opacity = Math::getNumber(arrayElements[0]);
			_red = Math::getNumber(arrayElements[1]);
			_green = Math::getNumber(arrayElements[2]);
			_blue = Math::getNumber(arrayElements[3]);
		}
	}
	else
	{
		if(rgbString.front() == '#' && rgbString.size() >= 4)
		{
			if(rgbString.size() >= 9)
			{
				_opacityDefined = true;
				try { _opacity = std::stoll(rgbString.substr(1, 2), 0, 16); } catch(...) {}
				try { _red = std::stoll(rgbString.substr(3, 2), 0, 16); } catch(...) {}
				try { _green = std::stoll(rgbString.substr(5, 2), 0, 16); } catch(...) {}
				try { _blue = std::stoll(rgbString.substr(7, 2), 0, 16); } catch(...) {}
			}
			else if(rgbString.size() >= 7)
			{
				try { _red = std::stoll(rgbString.substr(1, 2), 0, 16); } catch(...) {}
				try { _green = std::stoll(rgbString.substr(3, 2), 0, 16); } catch(...) {}
				try { _blue = std::stoll(rgbString.substr(5, 2), 0, 16); } catch(...) {}
			}
			else if(rgbString.size() >= 5)
			{
				_opacityDefined = true;
				try { _opacity = std::stoll(rgbString.substr(1, 1), 0, 16); } catch(...) {}
				try { _red = std::stoll(rgbString.substr(2, 1), 0, 16); } catch(...) {}
				try { _green = std::stoll(rgbString.substr(3, 1), 0, 16); } catch(...) {}
				try { _blue = std::stoll(rgbString.substr(4, 1), 0, 16); } catch(...) {}
			}
			else
			{
				try { _red = std::stoll(rgbString.substr(1, 1), 0, 16); } catch(...) {}
				try { _green = std::stoll(rgbString.substr(2, 1), 0, 16); } catch(...) {}
				try { _blue = std::stoll(rgbString.substr(3, 1), 0, 16); } catch(...) {}
			}
		}
	}
}

std::string Color::RGB::toString()
{
	return std::string("#") + BaseLib::HelperFunctions::getHexString(_red, 2) + BaseLib::HelperFunctions::getHexString(_green, 2) + BaseLib::HelperFunctions::getHexString(_blue, 2);
}

Color::HSV Color::NormalizedRGB::toHSV()
{
	double hue;
	double saturation;
	double brightness;

	double min = std::fmin(_blue, std::fmin(_red, _green));
	double max = std::fmax(_blue, std::fmax(_red, _green));
	brightness = max;

	double delta = max - min;
	if( max != 0 ) saturation = delta / max;
	else
	{
		saturation = 0;
		hue = 0; //undefined
	}

	if(_red == max)	hue = (_green - _blue) / delta;
	else if(_green == max) hue = 2 + (_blue - _red) / delta;
	else hue = 4 + (_red - _green) / delta;

	hue *= 60;
	if(hue < 0) hue += 360;

	return HSV(hue, saturation, brightness);
}

Color::RGB Color::HSV::toRGB()
{
	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;

	if(_saturation == 0)
	{
		red = green = blue = _brightness * 255.0;
		return RGB(red, green, blue);
	}

	double hue = _hue / 60;
	int32_t i = std::lround(std::floor(hue));
	double f = hue - i;
	double p = _brightness * (1 - _saturation);
	double q = _brightness * (1 - _saturation * f);
	double t = _brightness * (1 - _saturation * (1 - f));

	switch(i)
	{
		case 0:
			red = _brightness * 255.0;
			green = t * 255.0;
			blue = p * 255.0;
			break;
		case 1:
			red = q * 255.0;
			green = _brightness * 255.0;
			blue = p * 255.0;
			break;
		case 2:
			red = p * 255.0;
			green = _brightness * 255.0;
			blue = t * 255.0;
			break;
		case 3:
			red = p * 255.0;
			green = q * 255.0;
			blue = _brightness * 255.0;
			break;
		case 4:
			red = t * 255.0;
			green = p * 255.0;
			blue = _brightness * 255.0;
			break;
		default:
			red = _brightness * 255.0;
			green = p * 255.0;
			blue = q * 255.0;
			break;
	}

	return RGB(red, green, blue);
}

Color::Color()
{
}

Color::~Color()
{
}

void Color::getConversionMatrix(const Math::Triangle& gamut, Math::Matrix3x3& conversionMatrix, Math::Matrix3x3& inversedConversionMatrix)
{
	//D65 CIE 1931 reference white
	Math::Vector3D D65(0.95047, 1, 1.08883);

	conversionMatrix.p00 = gamut.getA().x / gamut.getA().y;
	conversionMatrix.p10 = 1;
	conversionMatrix.p20 = (1 - gamut.getA().x - gamut.getA().y) / gamut.getA().y;

	conversionMatrix.p01 = gamut.getB().x / gamut.getB().y;
	conversionMatrix.p11 = 1;
	conversionMatrix.p21 = (1 - gamut.getB().x - gamut.getB().y) / gamut.getB().y;

	conversionMatrix.p02 = gamut.getC().x / gamut.getC().y;
	conversionMatrix.p12 = 1;
	conversionMatrix.p22 = (1 - gamut.getC().x - gamut.getC().y) / gamut.getC().y;

	Math::Matrix3x3 inversedM1;
	conversionMatrix.inverse(inversedM1);

	Math::Vector3D S = inversedM1 * D65;

	conversionMatrix.p00 *= S.x;
	conversionMatrix.p01 *= S.y;
	conversionMatrix.p02 *= S.z;

	conversionMatrix.p10 *= S.x;
	conversionMatrix.p11 *= S.y;
	conversionMatrix.p12 *= S.z;

	conversionMatrix.p20 *= S.x;
	conversionMatrix.p21 *= S.y;
	conversionMatrix.p22 *= S.z;

	conversionMatrix.inverse(inversedConversionMatrix);
}

void Color::rgbToCie1931Xy(const NormalizedRGB& rgb, const Math::Matrix3x3& conversionMatrix, const double& gamma, Math::Point2D& xy, double& brightness)
{
	double red = rgb.getRed();
	double green = rgb.getGreen();
	double blue = rgb.getBlue();

	//First step: Inverse gamma companding
	Math::Vector3D rgbVector(std::pow(red, gamma), std::pow(green, gamma), std::pow(blue, gamma));

	//Second step: Multiply RGB with conversion matrix
	Math::Vector3D XYZ = conversionMatrix * rgbVector;

	//Third step: Convert XYZ to xyY
	double divisor = XYZ.x + XYZ.y + XYZ.z;

	brightness = XYZ.y;
	if(brightness < 0) brightness = 0;
	else if(brightness > 1) brightness = 1;
	if(divisor == 0) //Black
	{
		//Coordinates of reference white.
		xy.x = 0.3127;
		xy.y = 0.3290;
	}
	else
	{
		xy.x = XYZ.x / divisor;
		xy.y = XYZ.y / divisor;
	}
}

void Color::cie1931XyToRgb(const Math::Point2D& xy, const double& brightness, const Math::Matrix3x3& conversionMatrix, const double& gamma, NormalizedRGB& rgb)
{
	//First step: xyY to XYZ
	Math::Vector3D XYZ;
	if(xy.y != 0)
	{
		XYZ.y = brightness;
		if(XYZ.y < 0) XYZ.y = 0; else if(XYZ.y > 1) XYZ.y = 1;
		XYZ.x = (XYZ.y / xy.y) * xy.x;
		XYZ.z = (XYZ.y / xy.y) * (1.0 - xy.x - xy.y);
	}

	//Second step: XZY to RGB
	Math::Vector3D rgbVector = conversionMatrix * XYZ;

	//Third step: Gamma companding
	rgb.setRed(std::pow(rgbVector.x, 1.0 / gamma));
	rgb.setGreen(std::pow(rgbVector.y, 1.0 / gamma));
	rgb.setBlue(std::pow(rgbVector.z, 1.0 / gamma));
}

}
