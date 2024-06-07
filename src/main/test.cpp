import <print>;
import cc;

using namespace std;

int main()
{
	for (unsigned long i = ~0u; i < ~0ul; i++)
	{
		fused_t a {static_cast<double>(i)};
		println("{} - {}", i, fused_t::to(a));
	}
}
