#include "Response.hpp"

/* Default constructor */
Response::Response(void) :
	_rqst(),
	_cgi(),
	_code(std::make_pair(501, "Not Implemented")),
	_headers(),
	_body(),
	_response(),
	_readData()
{}

/* Destructor */
Response::~Response(void) {
	if (_readData.buffer != NULL)
		delete[] _readData.buffer;
	if (_readData.file.is_open())
		_readData.file.close();
}

/* Copy constructor */
Response::Response(const Response& src) {this->operator=(src);}

/* Assignement operator */
Response &	Response::operator=(const Response& src)
{
	if (this != &src)
	{
		_rqst = src._rqst;
		_cgi = src._cgi;
		_code = src._code;
		_headers = src._headers;
		_body = src._body;
		_response = src._response;
		cgiStatus = src.cgiStatus;
	}
	return *this;
}

/* Parametric constructor */
Response::Response(Request* rqst, Client *client) :
	cgiStatus(CGI::NOT_CGI),
	_rqst(rqst),
	_client(client),
	_cgi(),
	_code(std::make_pair(501, "Not Implemented")),
	_headers(),
	_body(),
	_response(),
	_readData()
{
	_headers["Connection"] = "keep-alive";
	_headers["Server"] = "WebServ/1.0";
	if (_rqst->getHeaders().find("Cookie") != _rqst->getHeaders().end())
		_headers["Set-Cookie"] = _rqst->getHeaders().find("Cookie")->second;

	if (_rqst == NULL || _rqst->getValForHdr("Host").empty())
	{
		_code = std::make_pair(400, "Bad Request");
	}
	else if (_rqst->getUri().path.size() + _rqst->getUri().query.size() > 1024)
	{
		_code = std::make_pair(414, "Request-URI Too Long");
	}
	else if (not _rqst->getLocation()->getRedirUrl().empty())
	{
		_code = std::make_pair(302, "Found");
		std::string		redir = _rqst->getLocation()->getRedirUrl();
		if (not startsWith(redir, "http"))
			redir.insert(0, _rqst->getLocation()->getRedirUrl()[0] == '/' ? "" : "/");
		Logger::Info("Redirection to " + redir);
		_headers["Location"] = redir;
	}
	else if (_client->hasTimeout())
		_code = std::make_pair(408, "Request Timeout");
	else if (checkMethod() == false)
	{
		setAllowHeader();
	}
	else
		doMethod();
}

bool	Response::checkBodySize(void)
{
	return (StrToInt(_rqst->getContentLength()) > _rqst->getConfig()->getMaxBodySize());
}

/* "Launch" le process de la response. */
void	Response::doMethod(void)
{
	if (_rqst->getMethod() == "GET")
	{
		doGET();
	}
	else if (_rqst->getMethod() == "DELETE")
	{
		doDELETE();
	}
	else if (_rqst->getMethod() == "POST")
	{
		int32_t		length = StrToInt(_rqst->getContentLength());

		if (length <= 0)
			_code = std::make_pair(411, "Length Required");
		else if (length > _rqst->getConfig()->getMaxBodySize())
			_code = std::make_pair(413, "Request Entity Too Large");
		else
			doPOST();
	}
}

void	Response::setAllowHeader(void)
{
	LocationConfig*	loc = _rqst->getLocation();
	std::string		allowed;
	if (loc->methodIsAllowed(LocationConfig::GET))
		allowed += "GET, ";
	if (loc->methodIsAllowed(LocationConfig::POST))
		allowed += "POST, ";
	if (loc->methodIsAllowed(LocationConfig::DELETE))
		allowed += "DELETE";
	if (*(allowed.end() - 1) == ' ')
		allowed.erase(allowed.end() - 2, allowed.end());
	_headers["Allow"] = allowed;
	
	_code = std::make_pair(405,  "Method Not Allowed");
}

bool	Response::openPageError(std::string path)
{

	std::ifstream	errFile(path.c_str());
	if (!errFile.is_open())
		return false;
	std::string		buff;
	while (std::getline(errFile, buff))
		_body += buff + "\n";
	return true;
}

void	Response::generateBodyError()
{
	if (_code.first >= 400)
	{
		ServerConfig::errors_t::const_iterator	it = _rqst->getConfig()->getErrorPaths().find(_code.first);

		if (it == _rqst->getConfig()->getErrorPaths().end() || not openPageError(it->second))
		{
			_body = generateErrorBody(_code);
			_headers["Connection"] = "close";
		}
	}
}

void	Response::generateResponse(void)
{
	if (_client->hasTimeout())
	{
		if (_rqst->getHeaders().find("User-Agent") != _rqst->getHeaders().end() && _rqst->getHeaders().find("User-Agent")->second.find("Firefox") != std::string::npos)
			_code = std::make_pair(503, "Service Unavailable");
		else
			_code = std::make_pair(408, "Request Timeout");
	}

	generateBodyError();
	if (_headers.find("Content-Length") == _headers.end())
		_headers["Content-Length"] = IntToStr(_body.size());
	// if (_code.first == 200 && _body.empty() && _headers["Content-Length"][0] == '0' )
		// _code = std::make_pair(204, "No Content");
	_response = "HTTP/1.1 " + IntToStr(_code.first) + " " + _code.second + CLRF;

	headers_t::const_iterator	it;
	for (it = _headers.begin(); it != _headers.end(); it++)
	{
		_response += it->first + ": " + it->second + CLRF;
	}

	// if (_body.size() > 0)
		// _response += "Content-Length: " + IntToStr(_body.size()) + CLRF;
	
	_response += CLRF;

	_response += _body;
}

static int	deleteDir(const std::string &path)
{
	typedef std::vector<std::string>	strVec;
	strVec		files = listFiles(path, false);
	strVec::const_iterator	it;
	for (it = files.begin(); it != files.end(); it++)
	{
		std::string target = path + "/" + *it;
		if (ends_with(*it, '/'))
			deleteDir(target);
		else
			std::remove(target.c_str());
		
	}
	return (std::remove(path.c_str()));
}

/* Delete a file, or if its a directy: delete all elements in it before rmdir. 
Returns a status code wether access one file failed (breaks the whole operation, 
404) or entire operation succeeded (204)*/
void	Response::doDELETE()
{
	if (_rqst->getCGILocation())
		doCGI();
	else
	{
		if (deleteDir(_rqst->getTargetPath()) == -1)
			_code = std::make_pair(404, "Not Found");
		else
		{
			_code = std::make_pair(200, "OK");
			_body = generateErrorBody(_code);
		}
	}
}

void	Response::waitCGI()
{
	int		statusPid;

	statusPid = _cgi.waitCGI();
	if (statusPid == 404)
		_code = std::make_pair(404, "Not Found");
	else if (statusPid == -1)
		_code = std::make_pair(500, "Internal Server Error");
	else if (statusPid == 0)
	{
		cgiStatus = CGI::IN_PROGRESS;
		return;
	}
	else
	{
		if (readFromCgi() == -1)
			_code = std::make_pair(500, "Internal Server Error");
		else if (_headers.find("Status") != _headers.end())
		{
			std::string::size_type pos = _headers["Status"].find(' ');
			if (pos != std::string::npos)
				_code = std::make_pair(StrToInt(_headers["Status"].substr(0, pos)), _headers["Status"].substr(pos + 1));
		}
		else
			_code = std::make_pair(200, "OK");
	}
	cgiStatus = CGI::FINISH;
}

void	Response::doCGI(void)
{
	cgiStatus = CGI::IN_PROGRESS;
	setCgiEnv();
	if (_rqst->getMethod() == "POST")
		_cgi.initFileIn(_rqst->getUploadFile());
	_cgi.initFileOut();
	int statusLaunch = _cgi.launch(_rqst->getCGILocation()->getCGICmd(), _rqst->getTargetPath());
	if (statusLaunch == -1)
	{
		_code = std::make_pair(500, "Internal Server Error");
		cgiStatus = CGI::FINISH;
	}
}

void Response::UploadMultipart(void)
{
	Request::headers_t::const_iterator it = _rqst->getHeaders().find("Content-Type");
	if (it == _rqst->getHeaders().end() || it->second.find("boundary=") == std::string::npos)
	{
		_code = std::make_pair(400, "Bad Request");
		return ;
	}
	std::string boundary = it->second;
	boundary.erase(0, boundary.find("boundary=") + 9);
	boundary.insert(0, "--");

	std::ifstream file(_rqst->getUploadFile().c_str());
	if (not file.is_open())
	{
		_code = std::make_pair(500, "Internal Server Error");
		return ;
	}
	std::string		line;
	std::string		filename;
	std::string		content;
	std::bitset<2>	headers;

	while (std::getline(file, line))
	{
		if (not headers.test(1) && startsWith(line, "Content-Disposition"))
		{
			if (line.find("filename") != std::string::npos)
			{
				filename = line.substr(line.find("filename") + 10);
				filename = filename.substr(0, filename.find("\""));
			}
			headers.set(1);
		}
		else if (not headers.test(0) && startsWith(line, "Content-Type"))
		{
			headers.set(0);
			continue;
		}
		else if (line == boundary + "\r" || line == boundary + "--\r")
		{
			if (!content.empty())
			{
				if (!filename.empty())
				{
					if (not ends_with(_rqst->getLocation()->getRootPath(), '/'))
						filename.insert(0, "/");
					filename.insert(0, _rqst->getLocation()->getRootPath());

					std::ofstream ofs(filename.c_str());
					if (not ofs.is_open())
					{
						Logger::Error("Response::UploadMultipart(): failed to open file '%s'", filename.c_str());
						_code = std::make_pair(500, "Internal Server Error");
						return ;
					}
					else
						Logger::Info("Response::UploadMultipart(): file '%s' created", filename.c_str());
					content.erase(0, 2);
					content.erase(content.size() - 2, 2);
					ofs << content;
					ofs.close();
				}
				filename.clear();
				content.clear();
				headers.reset();
			}
		}
		else
			content += line + '\n';
	}
	_code = std::make_pair(201, "OK");
	_body = generateErrorBody(_code);
}

void	Response::doPOST(void)
{
	Logger::Info("doPOST() entered");
	Request::headers_t	headers = _rqst->getHeaders();
	Request::headers_t::const_iterator	type = headers.find("Content-Type");
	
	if (type == headers.end())
	{
		_code = std::make_pair(501, "Not Implemented Content-Type");
	}
	else if (startsWith(type->second, "multipart/form-data"))
	{
		UploadMultipart();
	}
	else if (_rqst->getCGILocation() && type->second == "application/x-www-form-urlencoded")
	{
		cgiStatus = CGI::READY_TO_LAUNCH;
	}
	else
		_code = std::make_pair(400, "Bad Request");
}

/*
	1) Essai d'ouvrir le fichier : return false si fail (file not found)
	2) Remplit le _response body avec le contenu du fichier, et set le bon Content-Lenght
*/
bool	Response::tryFile(void)
{
	if (_readData.buffer == NULL)
	{
		Logger::Info("GET - Opening %s", _rqst->getTargetPath().c_str());
		_readData.file.open(_rqst->getTargetPath().c_str());
		if (_readData.file.is_open() == false)
			return false;

		// get length of _readData.file:
		_readData.file.seekg(0, _readData.file.end);
		// Print bitset of _readData.file.rdstate()
		int length = _readData.file.tellg();
		if (not _readData.file.good())
		{
			if (ends_with(_rqst->getTargetPath(), "/")) return false;
			_code = std::make_pair(301, "Moved Permanently");
			_headers["Location"] = _rqst->getUri().path + "/";
			_headers["Location"] += (_rqst->getUri().query.empty() ? "" : "?") + _rqst->getUri().query;
			return false;
		}
		_headers["Content-Length"] = IntToStr(length);
		_readData.file.seekg(0, _readData.file.beg);

		_readData.buffer = new char [BUFFSIZE_RES];
		std::memset(_readData.buffer, 0, BUFFSIZE_RES);

		_readData.file.read(_readData.buffer, BUFFSIZE_RES);
		if (not _readData.file.eof() && _readData.file.fail())
			throw std::runtime_error("read file failed");
		_readData.read_bytes = _readData.file.gcount();

		_body.assign(_readData.buffer, _readData.read_bytes);
	}
	else
	{
		if (not _readData.file.is_open()) return false;
		std::memset(_readData.buffer, 0, BUFFSIZE_RES);
		_readData.file.read(_readData.buffer, BUFFSIZE_RES);
		_readData.read_bytes = _readData.file.gcount();
	}
	if (_readData.file.eof())
	{
		_readData.file.close();
		_readData.status = NONE;
	}
	else
	{
		if (_readData.file.fail())
			throw std::runtime_error("read file chunk failed");
		_readData.status = IMCOMPLETE_READ;
	}
	return true;
}

static int transformChar(int c)
{
	if (c == '-')
		return '_';
	return std::toupper(c);
}

/* Set le vector d'environnement variables (RFC 3875) */
void		Response::setCgiEnv(void)
{
	_cgi.addVarToEnv("REDIRECT_STATUS=true");
	_cgi.addVarToEnv("GATEWAY_INTERFACE=CGI/1.1");
	_cgi.addVarToEnv("PATH_INFO=" + _rqst->getUri().path);
	_cgi.addVarToEnv("QUERY_STRING=" + _rqst->getUri().query);
	_cgi.addVarToEnv("CONTENT_LENGTH=" + _rqst->getContentLength());
	_cgi.addVarToEnv("CONTENT_TYPE=" + _rqst->getValForHdr("Content-Type"));
	_cgi.addVarToEnv("REQUEST_METHOD=" + _rqst->getMethod());
	_cgi.addVarToEnv("SCRIPT_FILENAME=" + _rqst->getTargetPath());
	_cgi.addVarToEnv("SCRIPT_NAME=" + _rqst->getUri().path);
	_cgi.addVarToEnv("SERVER_NAME=" + _rqst->getConfig()->getServerNames()[0]);
	_cgi.addVarToEnv("SERVER_PORT=" + IntToStr(_rqst->getConfig()->getPort()));
	_cgi.addVarToEnv("SERVER_PROTOCOL=HTTP/1.1");
	_cgi.addVarToEnv("SERVER_SOFTWARE=WebServ");
	for (Request::headers_t::const_iterator cit = _rqst->getHeaders().begin(); cit != _rqst->getHeaders().end(); ++cit)
	{
		if (cit->first == "Content-Type" || cit->first == "Content-Length" || cit->first == "Authorization")
			continue;
		std::string key = cit->first;
		std::transform(key.begin(), key.end(), key.begin(), transformChar);
		_cgi.addVarToEnv("HTTP_" + key + "=" + cit->second);
	}
}

/* Remplit le _body depuis les datas read from le pipe du CGI. */
int		Response::readFromCgi(void)
{
	int	fd = _cgi.getOutputFile();
	if (lseek(fd, 0, SEEK_SET) == -1)
		Logger::Error("Response::readFromCgi() lseek() failed");
	char buf[BUFFSIZE] = {};
	ssize_t	nbread = 0;
	nbread = read(fd, buf, BUFFSIZE - 1);
	if (nbread == -1)
	{
		Logger::Error("Response::Cgi() read() failed");
		return -1;
	}
	if (nbread == 0)
		return 0;
	buf[nbread] = 0;
	std::string		raw(buf, nbread);

	std::string::size_type pos = raw.find("\r\n\r\n");
	if (pos != std::string::npos) // thee is \r\n\r\n so headers
	{
		std::string::iterator	hdrs_end = raw.begin() + pos;
		std::string::iterator	new_hdr = raw.begin();
		std::string::iterator	end_new_hdr;
		while (new_hdr < hdrs_end)
		{
			end_new_hdr = findCRLF(new_hdr, hdrs_end);
			std::string::iterator		name_end = find(new_hdr, end_new_hdr, ':');
			std::string::iterator		value_start = name_end + 1;
			while (value_start != end_new_hdr && std::isspace(*value_start))
				++value_start;
			_headers[UpperKey(new_hdr, name_end)] = std::string(value_start, end_new_hdr);
			new_hdr = end_new_hdr + 2;
		}
		_body.assign(hdrs_end + 4, raw.end());
	}
	else
		_body = raw;
	if (_headers.find("Content-Type") == _headers.end())
		_headers["Content-Type"] = "text/html; charset=UTF-8";
	while (nbread == BUFFSIZE - 1)
	{
		nbread = read(fd, buf, BUFFSIZE - 1);
		if (nbread == -1)
		{
			Logger::Error("Response::Cgi() read() failed");
			return -1;
		}
		buf[nbread] = 0;
		_body += buf;
	}
	return (nbread);
}

void		Response::doDirectoryListening(void)
{
	_body = GenerateHtmlDirectory(_rqst->getTargetPath());
	_code = std::make_pair(200, "OK");
	if (_body.empty())
		_code = std::make_pair(404, "Not Found");
	else
		_headers["Content-Length"] = IntToStr(_body.size());
}

void		Response::doGET(void)
{
	std::string const & uriPath = _rqst->getUri().path;
	std::string locationPath = _rqst->getLocation()->getPath();
	if (locationPath[locationPath.size() - 1] == '/')
		locationPath = locationPath.substr(0, locationPath.size() - 1);
	if (not ends_with(uriPath, "/") && uriPath == locationPath)
	{
		_code = std::make_pair(301, "Moved Permanently");
		_headers["Location"] = _rqst->getUri().path + "/";
		_headers["Location"] += (_rqst->getUri().query.empty() ? "" : "?") + _rqst->getUri().query;
	}
	else if (_rqst->targetIsDir() && _rqst->getLocation()->isDirList() == true)
	{
		doDirectoryListening();
		return ;
	}
	else if (_rqst->targetIsFile() && tryFile())
	{
		_code = std::make_pair(200, "OK");
	}
	else if (_rqst->getCGILocation())
	{
		cgiStatus = CGI::READY_TO_LAUNCH;
	}
	else
	{
		if (_code.first == 501)
			_code = std::make_pair(404, "Not Found");
	}
}

bool	Response::checkMethod(void) const
{
	return _rqst->getLocation()->methodIsAllowed(_rqst->getMethod());
}

const Response::headers_t &	Response::getHeaders(void) const
{
	return _headers;
}

std::string	Response::getResponse(void) const
{
	return _response;
}

Response::ReadData const & Response::getReadData(void) const
{
	return _readData;
}

std::ostream&	operator<<(std::ostream& o, const Response& me)
{
	o << me.getResponse();
	return o;
}

const std::string &		Response::getBody(void) const
{
	return _body;
}
const std::pair<int, std::string> &	Response::getCode(void) const
{
	return _code;
}
