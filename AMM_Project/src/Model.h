#pragma once

#include <vector>
#include <string>
#include <cmath>

typedef struct vec2
{
	float x, y;

	float sqDist(const vec2& o) const {
		return (x - o.x)*(x - o.x) + (y - o.y)*(y - o.y);
	}

	float dist(const vec2& o) const {
		return std::sqrt(sqDist(o));
	}
} vec2, vec;

typedef struct City
{
	float population;
	vec cityPos;
} City;

typedef struct CenterType
{
	float serveDist;
	float maxPop;
	float cost;

} CenterType, Type;

class Model
{
public:
	Model() = default;

	bool readFromFile(const std::string& fileName);

	const std::vector<City>& getCities() const { return cities; }

	const std::vector<CenterType>& getCenterTypes() const { return centerTypes; }

	const std::vector<vec>& getLocations() const { return centerPos; }

	const float& getMinDistanceBetweenCenters() const { return minDistBetweenCenters; }

private:
	std::vector<City> cities;
	
	std::vector<vec> centerPos;

	std::vector<CenterType> centerTypes;

	float minDistBetweenCenters = 0.0f;

};