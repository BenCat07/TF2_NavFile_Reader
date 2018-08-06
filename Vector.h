#pragma once

#define min(x,y) y < x ? y : x
#define max(x,y) y > x ? y : x

//Simple vector class
class Vector
{
public:
	float x, y, z;

	float& Vector::operator[](int i)
	{
		return ((float*)this)[i];
	}
};