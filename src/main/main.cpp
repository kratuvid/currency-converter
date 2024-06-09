import <print>;
import <format>;
import <exception>;
import <cstring>;

import <sys/socket.h>;
import <netinet/in.h>;
import <unistd.h>;

import cc;

#define STD_EXCEPT(...) throw std::runtime_error(std::format(__VA_ARGS__));

int main()
{
	/*
	auto c = currency(100.0, currency_t::in);
	double d = c.get();
	println("{}", d);
	*/

	int retcode = 0;
	int soc = -1;

	try
	{
		int ret;

		soc = socket(PF_INET, SOCK_STREAM, 0);
		if (soc == -1)
			STD_EXCEPT("socket() errored: {}", strerror(errno));

		sockaddr_in sa;
	    sa.sin_family = AF_INET;
		sa.sin_port = htons(8080);
		sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		ret = connect(soc, reinterpret_cast<const sockaddr*>(&sa), sizeof(sa));
		if (ret == -1)
			STD_EXCEPT("connect() errored: {}", strerror(errno));
	}
	catch (std::exception& e)
	{
		std::println(stderr, "Exception caught: {}", e.what());
		retcode = 1;
	}

	if (soc != -1)
		close(soc);

	return retcode;
}
