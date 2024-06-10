import <print>;
import <format>;
import <exception>;
import <cstring>;

import cc;
import http;
import tls;
import hash;

#define WHAT 2

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
#elif WHAT == 2
	try
	{
		std::string_view input = "subject";
		hash compressed(input);
		std::println("'{}' = {}", input, compressed.str());
	}
#endif
	catch (std::exception& e)
	{
		std::println(stderr, "Exception caught: {}", e.what());
	}
}
