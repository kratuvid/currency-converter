import <print>;
import <format>;
import <exception>;
import <cstring>;

import cc;
import http;

#define STD_EXCEPT(...) throw std::runtime_error(std::format(__VA_ARGS__));

int main()
{
	/*
	auto c = currency(100.0, currency_t::in);
	double d = c.get();
	println("{}", d);
	*/

	http place;
	try
	{
		place.reset("gnu.org", 80);
	}
	catch (std::exception& e)
	{
		std::println(stderr, "Exception caught: {}", e.what());
	}
}
