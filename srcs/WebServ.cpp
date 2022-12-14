#include "WebServ.hpp"

void	WebServ::EndLoop(void)
{
	WebServ::_isRunning = false;
}

void	WebServ::InitEpoll(void)
{
	if (_servers.size() == 0)
		throw std::runtime_error("No servers to listen to");
	_epollInstance = epoll_create(_servers.size());
	if (_epollInstance < 0)
		throw std::runtime_error("epoll_create failed");
	struct epoll_event event = {};
	for (vec_servers::iterator it = _servers.begin(); it != _servers.end(); ++it)
	{
		Server*	server = *it;
		server->setEpollInstance(_epollInstance);
		event.data.ptr = server; 
		event.events = EPOLLIN;
		if (epoll_ctl(_epollInstance, EPOLL_CTL_ADD, server->getSocket(), &event) < 0) {
			   throw std::runtime_error("epoll_ctl add failed");
		}
	}
}

void	WebServ::StartLoop(void)
{
	if (_epollInstance < 0)
		InitEpoll();
	if (WebServ::_isRunning)
		return;
	WebServ::_isRunning = true;
	struct epoll_event events[MAX_EVENTS];
	int nfds = 0;
	std::signal(SIGINT, sig_handler);
	while (WebServ::_isRunning)
	{
		nfds = epoll_wait(_epollInstance, events, MAX_EVENTS, 5000);
		if (nfds < 0)
		{
			WebServ::_isRunning = false;
			continue ;
		}
		for (int i = 0; i < nfds; ++i)
		{
			SocketHandler *base = static_cast<SocketHandler*>(events[i].data.ptr);
			if (base == NULL)
			{
				Logger::Error("Base is NULL");
				continue ;
			}
			if (base->getType() == "Server")
			{
				Server*	server = dynamic_cast<Server*>(base);
				server->AcceptNewClient();
			}
			else if (base->getType() == "Client")
			{
				Client*	client = dynamic_cast<Client*>(base);
				if (events[i].events & EPOLLERR)
				{
					Logger::Error("ERROR on socket %d", client->getSocket());
					client->getServer()->removeClient(client);
				}
				else if (events[i].events & EPOLLIN)
				{
					int	readSize;
					try {
						readSize = client->recvRequest();
					}
					catch (std::exception& exc)
					{
						Logger::Error("%s", exc.what());
						readSize = 0;
					}
					if (readSize == 0)
						client->getServer()->removeClient(client);
				}
				else if (events[i].events & EPOLLOUT)
				{
					try
					{
						client->getServer()->respond(client);
						if (client->hasTimeout() || client->hasError())
						{
							client->getServer()->removeClient(client);
						}
					}
					catch(const std::exception& e)
					{
						Logger::Error("Problem client response: %s", e.what());
						client->getServer()->removeClient(client);
					}
				}
			} 
		}
		for (vec_servers::iterator it = _servers.begin(); it != _servers.end(); ++it)
		{
			Server*	server = *it;
			server->checkTimeout();
		}
	}
}

void	WebServ::addServer(Server *serv)
{
	_servers.push_back(serv);
}

/* Destructor */
WebServ::~WebServ(void)
{
	for (vec_servers::iterator it = _servers.begin(); it != _servers.end(); ++it)
	{
		const socket_t socket = (*it)->getSocket();

		if (socket >= 0)
			close(socket);
		delete *it;
	}
	if (_epollInstance > 0)
		close(_epollInstance);
}

/* Check if the 'toAdd' server has the same IP:PORT than an already registered
server. Return an iterator on the end if no _servers has the same pair, or 
an iterator on the Server* which match it, on to whom the toAdd Server is to 
be added. (Server::addconfig(Server)) */
std::vector<Server*>::iterator		WebServ::checkIpPort(const ServerConfig& toAdd)
{
	for (vec_servers::iterator serv = _servers.begin(); serv != _servers.end(); ++serv)
	{
		if ((*serv)->isSameHostPort(toAdd.getHost(), toAdd.getPort()) == true)
			return serv;
	}
	return _servers.end();
}

const std::vector<Server*> &		WebServ::getServers(void) const
{
	return _servers;
}

/* Default Constructor */
WebServ::WebServ(void) : _servers(), _epollInstance(-1) {}

/* Copy constructor */
WebServ::WebServ(WebServ const & other)
{
	*this = other;
}

WebServ &	WebServ::operator=(WebServ const & other)
{
	if (this != &other) {
		_servers = other._servers;
		_epollInstance = other._epollInstance;
	}
	return *this;
}

bool WebServ::_isRunning = false;

void	sig_handler(int sig)
{
	(void)sig;
	std::cout << "SIG HANDLER REACHED" << std::endl;
	WebServ::_isRunning = false;
}
