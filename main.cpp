#include <iostream>
#include <map>
#include "Path.hpp"

#define hw "hi"

class A {
	public:
	A(){std::cout << "A\n";};
	~A(){std::cout << "~A\n";};
};

class B : public std::string {
	public:
	using std::string::basic_string;
	void test() {
		*this = ("new");
	};
};

int main(int ac, char **av)
{
	(void)ac;
	(void)av;
	int i = 1;
	B b("h");
	b += '3';
	std::cout << b << std::endl;
	b.test();
	std::cout << b << std::endl;
	std::cout << hw << i << "\n";

	std::cout << std::endl;

	std::string path(".");
	std::vector<std::string> listFiles = Path::listFiles(path);
	std::cout << "listFiles: " << std::endl;
	for (std::vector<std::string>::iterator it = listFiles.begin(); it != listFiles.end(); it++)
		std::cout << *it << " ";
	std::cout << std::endl << listFiles.size() << std::endl;
	
}

