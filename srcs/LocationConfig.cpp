#include "LocationConfig.hpp"

LocationConfig::LocationConfig() :
_dirList(false),
_path("/"),
_root(),
_indexFile("index.html"),
_redirUrl(),
_CGICmd(),
_methods() {}
LocationConfig::LocationConfig(const LocationConfig &other)
{
	*this = other;
}
LocationConfig &LocationConfig::operator=(const LocationConfig &other)
{
	if (this != &other)
	{
		_dirList = other._dirList;
		_path = other._path;
		_CGICmd = other._CGICmd;
		// _CGI = other._CGI;
		_redirUrl = other._redirUrl;
		_indexFile = other._indexFile;
		_root = other._root;
		_methods = other._methods;
	}
	return *this;
}
LocationConfig::~LocationConfig() {}

LocationConfig::LocationConfig(std::string path) :
_dirList(false),
_path(path),
_root(),
_indexFile(),
_redirUrl(),
_CGICmd(),
_methods() {}

void	LocationConfig::setPath(const std::string & path)
{
	_path = path;
}
void	LocationConfig::setDirList(bool dirList)
{
	_dirList = dirList;
}
void	LocationConfig::setRootPath(const std::string &path)
{
	_root = path;
}
void	LocationConfig::setIndexFile(const std::string &indexFile)
{
	_indexFile = indexFile;
}
void	LocationConfig::setRedirUrl(const std::string &url)
{
	_redirUrl = url;
}
void	LocationConfig::setCGICmd(const std::string &CGICmd)
{
	_CGICmd = CGICmd;
}
void	LocationConfig::addMethod(http_methods method)
{
	_methods.set(method, true);
}


std::string 	LocationConfig::getIndexFile() const
{
	return _indexFile;
}
std::string		LocationConfig::getRootPath() const
{
	return _root;
}
bool			LocationConfig::isDirList() const
{
	return _dirList;
}
bool			LocationConfig::isRedirection() const
{
	return (not _redirUrl.empty());
}
bool			LocationConfig::isCGIActive() const
{
	return (not _CGICmd.empty());
}
std::string		LocationConfig::getPath() const
{
	return _path;
}
std::string		LocationConfig::getCGICmd() const
{
	return _CGICmd;
}
std::string		LocationConfig::getRedirUrl() const
{
	return _redirUrl;
}
std::bitset<3>	LocationConfig::getMethods() const
{
	return _methods;
}
bool			LocationConfig::methodIsAllowed(http_methods method) const
{
	return _methods.test(method);
}
bool	LocationConfig::isEmpty() const
{
	return (_dirList == false && isCGIActive() == false && _CGICmd.empty() && _indexFile.empty() && _root.empty() && _methods.none());
}

bool	LocationConfig::methodIsAllowed(std::string method) const
{
	if (method == "GET" && _methods.test(GET))
		return true;
	if (method == "POST" && _methods.test(POST))
		return true;
	if (method == "DELETE" && _methods.test(DELETE))
		return true;
	return false;
}

std::ostream&	operator<<(std::ostream& o, const LocationConfig& me)
{
	o << std::boolalpha << "Directory list active = " << me.isDirList() << std::endl;
	o << std::boolalpha << "CGI active = " << me.isCGIActive() << std::endl;
	o << "Path = " << me.getPath() << std::endl;
	o << "Root = " << me.getRootPath() << std::endl;
	o << "Index file = " << me.getIndexFile() << std::endl;
	o << "Redir URL = " << me.getRedirUrl() << std::endl;
	o << "CGI path = " << me.getCGICmd() << std::endl;
	o << "Methods = ";
	if (me.methodIsAllowed(LocationConfig::GET) == true)
		o << "GET ";
	if (me.methodIsAllowed(LocationConfig::POST) == true)
		o << "POST ";
	if (me.methodIsAllowed(LocationConfig::DELETE) == true)
		o << "DELETE ";
	o << std::endl;
	return o;
}
