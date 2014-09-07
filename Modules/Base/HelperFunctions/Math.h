/* Copyright 2013-2014 Sathya Laufer
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

#ifndef HOMEGEARMATH_H_
#define HOMEGEARMATH_H_

#include <string>
#include <map>

namespace BaseLib
{
class Math
{
public:
	/**
	 * Class defining a point in 2D space with numbers of type double.
	 */
	class Point2D
	{
	public:
		Point2D() {}
		/**
		 * Constructor converting a coordinate string with coordinates seperated by ";" (i. e. "0.3613;0.162").
		 */
		Point2D(const std::string& s);
		Point2D(double x, double y) { this->x = x; this->y = y; }
		virtual ~Point2D() {}

		std::string toString() { return std::to_string(x) + ';' + std::to_string(y); }

		double x = 0;
		double y = 0;
	};

	typedef Point2D Vector2D;

	class Point3D
	{
	public:
		Point3D() {}
		/**
		 * Constructor converting a coordinate string with coordinates seperated by ";" (i. e. "0.3613;0.162;0.543").
		 */
		Point3D(const std::string& s);
		Point3D(double x, double y, double z) { this->x = x; this->y = y; this->z = z; }
		virtual ~Point3D() {}

		std::string toString() { return std::to_string(x) + ';' + std::to_string(y) + ';' + std::to_string(z); }

		double x = 0;
		double y = 0;
		double z = 0;
	};

	typedef Point3D Vector3D;

	/**
	 * Class defining a 3x3 matrix.
	 */
	class Matrix3x3
	{
	public:
		double p00 = 0;
		double p10 = 0;
		double p20 = 0;
		double p01 = 0;
		double p11 = 0;
		double p21 = 0;
		double p02 = 0;
		double p12 = 0;
		double p22 = 0;

		Matrix3x3() {}

		virtual ~Matrix3x3() {}

		/**
		 * Inverses the matrix.
		 *
		 * @param[out] inversedMatrix The inversed matrix;
		 */
		void inverse(Matrix3x3& inversedMatrix);

		Math::Vector3D operator* (const Math::Vector3D& v) const;

		double determinant();

		std::string toString();
	};

	/**
	 * Class defining a line.
	 */
	class Line
	{
	public:
		Line() {}
		Line(const Point2D& a, const Point2D& b) { _a = a; _b = b; }
		virtual ~Line() {}

		Point2D getA() const { return _a; }
		void setA(Point2D value) { _a = value; }
		void setA(Point2D& value) { _a = value; }
		Point2D getB() const { return _b; }
		void setB(Point2D value) { _b = value; }
		void setB(Point2D& value) { _b = value; }

		/**
		 * Finds the closest point on the line to a point.
		 *
		 * @param[in] p The point to get the closest point on the line for.
		 * @param[out] r The point on the line which is closest to p.
		 */
		void closestPointToPoint(const Point2D& p, Point2D& r);
	protected:
		Point2D _a;
		Point2D _b;
	};

	/**
	 * Class defining a triangle.
	 */
	class Triangle
	{
	public:
		Triangle() {}
		Triangle(const Point2D& a, const Point2D& b, const Point2D& c) { _a = a; _b = b; _c = c; }
		virtual ~Triangle() {}

		Point2D getA() const { return _a; }
		void setA(Point2D value) { _a = value; }
		void setA(Point2D& value) { _a = value; }
		Point2D getB() const { return _b; }
		void setB(Point2D value) { _b = value; }
		void setB(Point2D& value) { _b = value; }
		Point2D getC() const { return _c; }
		void setC(Point2D value) { _c = value; }
		void setC(Point2D& value) { _c = value; }

		/**
		 * Calculates the distance of a point to the triangle.
		 *
		 * @see <a href="http://wonderfl.net/c/b27F">Flash example by mutantleg</a>
		 * @param[in] p The point to check.
		 * @param[out] (default nullptr) closestPoint When specified, the closest point in/on the triangle is stored in this variable.
		 * @return Returns the squared distance or 0 if the point is within the triangle.
		 */
		double distance(const Point2D& p, Point2D* closestPoint = nullptr);
	protected:
		Point2D _a;
		Point2D _b;
		Point2D _c;
	};

	Math();

	/**
	 * Destructor.
	 * Does nothing.
	 */
	virtual ~Math();

	/**
	 * Checks if a string is a number.
	 *
	 * @param s The string to check.
	 * @param hex (default false) Set to "true" if s contains a hexadecimal number.
	 * @return Returns true if the string is a decimal or hexadecimal number, otherwise false.
	 */
	static bool isNumber(const std::string& s, bool hex = false);

	/**
	 * Converts a string (decimal or hexadecimal) to an integer.
	 *
	 * @see getDouble()
	 * @see getUnsignedNumber()
	 * @param s The string to convert.
	 * @param isHex Set this parameter to "true", if the string is hexadecimal (default false). If the string is prefixed with "0x", it is automatically detected as hexadecimal.
	 * @return Returns the integer or "0" on error.
	 */
	static int32_t getNumber(std::string& s, bool isHex = false);

	/**
	 * Converts a hexadecimal character to an integer.
	 *
	 * @see getDouble()
	 * @see getUnsignedNumber()
	 * @param hexChar The hexadecimal character.
	 * @return Returns the integer or "0" on error.
	 */
	int32_t getNumber(char hexChar);

	/**
	 * Converts a string (decimal or hexadecimal) to an unsigned integer.
	 *
	 * @see getDouble()
	 * @see getNumber()
	 * @param s The string to convert.
	 * @param isHex Set this parameter to "true", if the string is hexadecimal (default false). If the string is prefixed with "0x", it is automatically detected as hexadecimal.
	 * @return Returns the unsigned integer or "0" on error.
	 */
	static uint32_t getUnsignedNumber(std::string &s, bool isHex = false);

	/**
	 * Converts a string to double.
	 *
	 * @see getNumber()
	 * @see getUnsignedNumber()
	 * @param s The string to convert to double.
	 * @return Returns the number or "0" if the conversion was not successful.
	 */
	static double getDouble(const std::string& s);
protected:
	/**
	 * Map to faster convert hexadecimal numbers.
	 */
	std::map<char, int32_t> _hexMap;
};

}

#endif
