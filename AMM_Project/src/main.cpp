

#include "Model.h"
#include <iostream>

int main(int argc, char* argv[]) {

	ModelData modelData;

	std::string fileName = "data/project.2.dat";
	bool read = modelData.readFromFile(fileName);
	if (!read)
	{
		std::cout << "Cannot read file " << fileName << std::endl;
	}

	return 0;
}