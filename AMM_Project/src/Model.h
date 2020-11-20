#pragma once

#include <vector>
#include <string>

class Model
{

};

typedef struct vec2
{
	float x, y;
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

class ModelData
{
public:
	ModelData() = default;

	bool readFromFile(const std::string& fileName);

private:
	std::vector<City> cities;
	
	std::vector<vec> centerPos;

	std::vector<CenterType> centerTypes;

	float minDistBetweenCenters;

};