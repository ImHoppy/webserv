#include "Utils.hpp"
#include <stack>

char	generateChar(void)
{
	const std::string alpha("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
	return alpha[std::rand() % alpha.size()];
}

std::string	generateFileName(int seed)
{
	std::srand(seed);
	std::string fileName = "upload_";
	fileName.reserve(25);
	for (int i = 8; i < 15; i++)
		fileName += generateChar();
	return fileName;
}

std::string		getFileExtension(const std::string & filename)
{
	std::string		extension;
	size_t	posDot = filename.find_last_of('.', filename.find_last_of('/'));
	if (posDot == std::string::npos)
	{
		extension = "plain";
		return extension;
	}
	extension = filename.substr(posDot + 1);
	return extension;
}

bool	ends_with(const std::string& str, const char c)
{
	if (str.empty())
		return false;
	if (*(str.end() - 1) != c)
		return false;
	return true;
}

bool	ends_with(const std::string& str, const std::string& extension)
{
	std::string::size_type	pos;
	pos = str.rfind(extension);
	if (pos != std::string::npos && pos == str.size() - extension.size())
		return true;
	return false;
}

bool	startsWith(std::string const & str, std::string const & start)
{
	if (str.length() < start.length())
		return false;
	return (str.compare(0, start.length(), start) == 0);
}

std::string	IntToStr(int nb)
{
	std::stringstream	ss;
	ss << nb;
	return ss.str();
}

int32_t		StrToInt(std::string const & str)
{
	std::istringstream s(str);
	int32_t i;
	s >> i;
	return i;
}

int32_t		StrToIp(std::string const & ip)
{
	std::stringstream s(ip);
	int a,b,c,d;
	char ch;
	s >> a >> ch >> b >> ch >> c >> ch >> d;
	return (a << 24) + (b << 16) + (c << 8) + d;
}
std::string	IpToStr(int32_t ip)
{
	std::stringstream s;
	s << ((ip) & 0xFF) << "." << ((ip >> 8) & 0xFF) << "." << ((ip >> 16) & 0xFF) << "." << (ip >> 24 & 0xFF);
	return s.str();
}

bool	isIpv4(std::string const & s)
{
	int		count = 0;
	int		i = 0;
	int		nb = 0;
	std::string::const_iterator it;

	for (it = s.begin(); it < s.end(); it++)
	{
		if (*it == '.')
		{
			if (nb > 255)
				return false;
			nb = 0;
			count++;
			i = 0;
		}
		else if (*it >= '0' && *it <= '9')
		{
			nb = nb * 10 + (*it - '0');
			i++;
		}
		else
			return false;
	}
	if (count != 3)
		return false;
	return true;
}

bool	isDigit(std::string const & s)
{
	std::string::const_iterator it;

	for (it = s.begin(); it < s.end(); it++)
	{
		if (*it < '0' || *it > '9')
			return false;
	}
	return true;
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::string pathExpand(std::string const & path)
{
	std::stack<std::string>	pathStack;

	std::vector<std::string> paths;
	split(path, '/', paths);

	for (std::vector<std::string>::iterator it = paths.begin(); it != paths.end(); ++it)
	{
		if (*it == "..")
		{
			if (not pathStack.empty())
				pathStack.pop();
		}
		else if (*it != ".")
			pathStack.push(*it);
	}
	
	std::string	expandedPath;
	while (pathStack.empty() == false)
	{
		if (not pathStack.top().empty())
			expandedPath = "/" + pathStack.top() + expandedPath;
		pathStack.pop();
	}
	expandedPath += (ends_with(path, "/") ? "/" : "");
	return expandedPath;
}

std::string getExtension(std::string const & filename)
{
	std::string::size_type pos = filename.find_last_of('.');
	if (pos == std::string::npos)
		return "";
	return filename.substr(pos + 1);
}

/*
Find in the range [first, last) the first occurence of CRLF ("\r\n"). Return an iterator
to '\r' if found, end otherwise.
*/
std::string::iterator	findCRLF(std::string::iterator start, std::string::iterator end)
{
	while (start != end)
	{
		if (*start == '\r' && (start + 1) != end && *(start + 1) == '\n')
			return start;
		++start;
	}
	return end;
}

std::string		UpperKey(std::string::iterator start, std::string::iterator end)
{
	std::string str(start, end);
	*(str.begin()) = std::toupper(*(str.begin()));
	std::string::iterator prevMinus = str.begin() + str.find('-');
	*prevMinus = std::toupper(*prevMinus);
	return str;
}
