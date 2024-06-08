import <print>;
import cc;
using namespace std;

int main()
{
	auto c = currency(100.0, currency_t::in);
	double d = c.get();
	println("{}", d);
}
