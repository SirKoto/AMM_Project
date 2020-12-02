#include "Model.h"

#include <fstream>
#include <cassert>
#include <iostream>

bool Model::readFromFile(const std::string& fileName)
{

    std::ifstream stream(fileName, std::ifstream::in);

    if (!stream)
    {
        return false;
    }

    std::string op;
    std::string tmp;

    while (!stream.eof()) {
        stream >> op;
        stream >> tmp; // read the first equal....

        if (op == "nLocations") {
            int nLocations;
            stream >> nLocations;
            this->centerPos.resize(nLocations);
            std::getline(stream, tmp); // ignore line
        }
        else if (op == "nCities") {
            int nCities;
            stream >> nCities;
            this->cities.resize(nCities);
            std::getline(stream, tmp); // ignore line
        }
        else if (op == "nTypes") {
            int nTypes;
            stream >> nTypes;
            this->centerTypes.resize(nTypes);
            std::getline(stream, tmp); // ignore line
        }
        else if (op == "p") {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '['); // ignore until open bracket
            for (City& city : cities) {
                stream >> city.population;
            }
            std::getline(stream, tmp); // ignore line
        }
        else if (op == "posCities") {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '['); // ignore until open bracket
            for (City& city : cities) {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '['); // ignore until open bracket
                stream >> city.cityPos.x;
                stream >> city.cityPos.y;
            }
            std::getline(stream, tmp); // ignore line
        }
        else if (op == "posLocations") {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '['); // ignore until open bracket
            for (vec& pos : centerPos) {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '['); // ignore until open bracket
                stream >> pos.x;
                stream >> pos.y;
            }
            std::getline(stream, tmp); // ignore line
        }
        else if (op == "d_city") {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '['); // ignore until open bracket
            for (CenterType& type : centerTypes) {
                float aux;
                stream >> aux;
                type.serveDist=aux;
            }
            std::getline(stream, tmp); // ignore line
        }
        else if (op == "cap") {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '['); // ignore until open bracket
            for (CenterType& type : centerTypes) {
                stream >> type.maxPop;
            }
            std::getline(stream, tmp); // ignore line
        }
        else if (op == "cost") {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '['); // ignore until open bracket
            for (CenterType& type : centerTypes) {
                stream >> type.cost;
            }
            std::getline(stream, tmp); // ignore line
        }
        else if (op == "d_center") {
            float aux;
            stream >> aux;
            this->minDistBetweenCenters=aux;;
            std::getline(stream, tmp); // ignore line
        }
        else {
            //ignore line
            std::cout << "ignored: " << tmp;
            std::getline(stream, tmp); // ignore line
            std::cout << " " << tmp << std::endl;
        }
    }

    return true;
}
