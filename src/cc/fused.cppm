export module cc:fused;

import <cstdint>;

export class fused_t
{
private:
	// integer occupies the first half and fractions rest
	struct __attribute__ ((__packed__)) {
		uint64_t a, b, c, d;
	} store;

public:
	fused_t()
		:store {0, 0, 0, 0}
	{
	}

	fused_t(uint64_t a, uint64_t b, uint64_t c, uint64_t d)
		:store {a, b, c, d}
	{}

	fused_t(double source)
	{
		this->store = from(source).store;
	}
	
	fused_t& operator+=(const fused_t& rhs)
	{
		return *this;
	}

	fused_t& operator>>=(unsigned times)
	{
		auto ptr = reinterpret_cast<uint64_t*>(&store);
		auto real_times = times > 256 ? 256 : times;
		for (int i=0; i < real_times; i++)
		{
			uint64_t saved_bit = 0;
			for (int j=0; j < 4; j++)
			{
				const uint64_t next_saved_bit = static_cast<bool>(ptr[j] & 0x1);
				ptr[j] >>= 1;
				ptr[j] |= saved_bit << 63;
				saved_bit = next_saved_bit;
			}
		}
		return *this;
	}

	fused_t& operator<<=(unsigned times)
	{
		auto ptr = reinterpret_cast<uint64_t*>(&store);
		auto real_times = times > 256 ? 256 : times;
		for (int i=0; i < real_times; i++)
		{
			uint64_t saved_bit = 0;
			for (int j=3; j >= 0; j--)
			{
				const uint64_t next_saved_bit = static_cast<bool>(ptr[j] & (uint64_t(1) << 63));
				ptr[j] <<= 1;
				ptr[j] |= saved_bit;
				saved_bit = next_saved_bit;
			}
		}
		return *this;
	}

	uint64_t bit(unsigned i) const
	{
		const auto unit = i / 64, at = i % 64, at_real = 63 - at;
		auto ptr = reinterpret_cast<const uint64_t*>(&store);
		return (ptr[unit] >> at_real) & 0x1;
	}

	void bit_set(unsigned i)
	{
		const auto unit = i / 64, at = i % 64, at_real = 63 - at;
		auto ptr = reinterpret_cast<uint64_t*>(&store);
		ptr[unit] |= 1 << at_real;
	}
	
	void bit_clear(unsigned i)
	{
		const auto unit = i / 64, at = i % 64, at_real = 63 - at;
		auto ptr = reinterpret_cast<uint64_t*>(&store);
		ptr[unit] &= ~(1 << at_real);
	}

public:
	static fused_t from(double source)
	{
		const auto sz_exponent = 11, sz_fraction = 52;

		auto ptr = reinterpret_cast<const uint64_t*>(&source);
		const uint64_t sign = *ptr >> 63; // discarded
		const uint64_t exponent = (*ptr << 1) >> (sz_fraction + 1);
		const uint64_t fraction = (*ptr << (sz_exponent + 1)) >> (sz_exponent + 1);
		const int64_t normalised_exponent = exponent - 1023;

		fused_t converted; // initialized to zero by the default constructor

		if (*ptr == 0) // zero
		{
			// already zero
		}
		else if (exponent == 0x7ff) // infinity and nan
		{
			// do nothing
		}
		else if (exponent == 0 && fraction != 0) // subnormal
		{
			converted.store.c = fraction << (sz_exponent + 1);
		}
		else // normal
		{
			converted.store.b = 1;
			converted.store.c = fraction << (sz_exponent + 1);
			if (normalised_exponent >= 0)
				converted <<= normalised_exponent;
			else
				converted >>= -normalised_exponent;
		}

		return converted;
	}

	static double to(fused_t source)
	{
		const auto sz_exponent = 11, sz_fraction = 52;

		int first_one = -1;
		for (unsigned i=0; i < 256; i++)
		{
			if (source.bit(i))
			{
				first_one = i;
				break;
			}
		}

		double converted = 0;
		auto ptr = reinterpret_cast<uint64_t*>(&converted);

		if (first_one == -1) // zero
		{
			// already zero
		}
		else
		{
			const int64_t normalised_exponent = 127 - first_one;
			const uint64_t exponent = normalised_exponent + 1023;
			uint64_t fraction = 0;

			int i = first_one + 1;
			for (int j = 51; j >= 0 && i < 256; j--, i++)
			{
				const uint64_t bit = static_cast<bool>(source.bit(i));
				fraction |= bit << j;
			}

			*ptr = (exponent << sz_fraction) | fraction;
		}

		return converted;
	}
};
