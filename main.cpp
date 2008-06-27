#include "document.h"
#include "regex.h"

int main()
{
	YAML::RegEx alpha = YAML::RegEx('a', 'z') || YAML::RegEx('A', 'Z');
	alpha.Matches("a");
	alpha.Matches("d");
	alpha.Matches("F");
	alpha.Matches("0");
	alpha.Matches("5");
	alpha.Matches(" ");

	YAML::RegEx blank = YAML::RegEx(' ') || YAML::RegEx('\t');
	YAML::RegEx docstart = YAML::RegEx("---") + (blank || YAML::RegEx(EOF) || YAML::RegEx());
	docstart.Matches("--- ");
	docstart.Matches("... ");
	docstart.Matches("----");
	docstart.Matches("---\t");
	docstart.Matches("---");

	YAML::Document doc("test.yaml");

	return 0;
}