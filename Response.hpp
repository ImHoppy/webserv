#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <iostream>
# include <string>
# include <map>
# include "ServerConfig.hpp"
# include "Request.hpp"
# include "Utils.hpp"

class Response
{
	public:
		typedef std::map<std::string, std::string>	headers_t;
	private:
		ServerConfig*						_config;
		LocationConfig*						_location;
		Request*							_rqst;
		std::string							_response;

	public:
		/* COplien */
		Response(void);
		~Response(void);
		Response(const Response& src);
		Response&	operator=(const Response& src);
		Response(ServerConfig* config, LocationConfig* loc, Request* req);
		/* Getteurs */
		std::string		getResponse(void) const;
		/* Public Member functions */
	
}; // end class Response

std::ostream&	operator<<(std::ostream& o, const Response& me);

#endif