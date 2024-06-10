import <print>;
import <format>;
import <exception>;
import <cstring>;
import <iostream>;

import cc;
import http;
import tls;
import hash;
import fused;

#define WHAT 3

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
		std::print("Hash of what? ");
		std::string input;
		std::getline(std::cin, input);
		hash compressed(input);
		std::println("'{}' = {}", input, compressed.str());
	}
#elif WHAT == 3
	try
	{
		double input;
		std::print("Double please: ");
		std::cin >> input;
		auto f = fused::from(input);
		auto d = f.ito();
		std::println("What you probably entered: {}", d);
	}
#endif
	catch (std::exception& e)
	{
		std::println(stderr, "Exception caught: {}", e.what());
	}
}
