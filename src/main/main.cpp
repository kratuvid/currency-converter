import <print>;
import <format>;
import <exception>;
import <cstring>;

import cc;
import http;
import tls;

#define WHAT 1

template<class... Args>
void enact_std_exception(std::string_view format, Args&&... args)
{
	throw std::runtime_error(std::vformat(format, std::make_format_args(args...)));
}

int main()
{
#if WHAT == 0
	http place;
	try
	{
		place.reset("fsf.org", 80);
		std::println("{}", place.get("/"));
	}
#elif WHAT == 1
	tls trial;
	try
	{
	}
#endif
	catch (std::exception& e)
	{
		std::println(stderr, "Exception caught: {}", e.what());
	}
}
