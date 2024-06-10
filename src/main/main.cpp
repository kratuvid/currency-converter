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

#define WHAT 4

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
		double i1, i2;
		std::print("Two double please: ");
		std::cin >> i1 >> i2;
		auto f1 = fused::from(i1), f2 = fused::from(i2);
		auto fp = f2 * f2;
		auto d1 = f1.ito(), d2 = f2.ito();
		auto dp = fp.ito();
		std::println("{} * {} = {} (actuual {})", d1, d2, dp, i1 * i2);
	}
#elif WHAT == 4
	try
	{
		using fused_local = fused<10>;

		double i1, i2;
		std::print("Two double please: ");
		std::cin >> i1 >> i2;
		auto f1 = fused_local::from(i1), f2 = fused_local::from(i2);
		auto d1 = f1.ito(), d2 = f2.ito();
		std::println("{}, {}", d1, d2);
	}
#endif
	catch (std::exception& e)
	{
		std::println(stderr, "Exception caught: {}", e.what());
	}
}
