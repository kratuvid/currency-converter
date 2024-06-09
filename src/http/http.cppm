export module http;

import <print>;
import <format>;
import <exception>;
import <cstdint>;
import <cstring>;
import <string_view>;

import <sys/socket.h>;
import <netinet/in.h>;
import <unistd.h>;
import <netdb.h>;

export class http
{
private:
	int sock = -1;

	uint16_t port = 0;
	uint32_t address = 0;

public:
	http()
	{
	}

	http(std::string_view site, uint16_t port)
	{
		reset(site, port);
	}

	void reset(std::string_view site, uint16_t port)
	{
		this->port = port;

		auto addrinfo = gethostbyname(site.data());
		if (addrinfo)
		{
			if (addrinfo->h_addrtype != AF_INET)
				exception::enact("Unexpected address type: {}", addrinfo->h_addrtype);
			auto al = addrinfo->h_addr_list[0];
			this->address = htonl(*reinterpret_cast<uint32_t*>(al));
		} else exception::enact("Cannot resolve '{}'", site);

		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == -1)
			exception::enact("Failed to create socket in the process of connecting to '{}'", site);

		sockaddr_in sa;
		sa.sin_family = AF_INET;
		sa.sin_port = htons(this->port);
		sa.sin_addr.s_addr = htonl(this->address);
		if (connect(sock, reinterpret_cast<const sockaddr*>(&sa), sizeof(sa)) == -1)
			exception::enact("Unable to connect to '{}' aka {}.{}.{}.{}: {}", site, address >> 24, (address & 0xff0000) >> 16, (address & 0xff00) >> 8, address & 0xff, strerror(errno));
	}

	~http()
	{
		if (sock != -1)
			close(sock);
	}

	std::string get(std::string_view location)
	{
		return "";
	}

public:
	class exception : public std::runtime_error
	{
	public:
		exception(std::string_view msg) :std::runtime_error(msg.data()) {}

		template<class... Args>
		static void enact(std::string_view format, Args&&... args)
		{
			std::string format_real("http: ");
			format_real += format;
			throw exception(std::vformat(format_real, std::make_format_args(args...)));
		}
	};
};
