#include "reader.h"
#include <fstream>

int main()
{
	std::ifstream fin("test.yaml");
	YAML::Reader reader(fin);
	YAML::Document doc;
	reader.GetNextDocument(doc);

	return 0;
}