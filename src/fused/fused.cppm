export module fused;

import <cstdint>;

export class fused
{
private:
	// integer occupies the first half and fractions rest
	struct __attribute__ ((__packed__)) {
		uint64_t a, b, c, d;
	} store;

public:
	fused()
		:store {0, 0, 0, 0}
	{
	}

	fused(uint64_t a, uint64_t b, uint64_t c, uint64_t d)
		:store {a, b, c, d}
	{}

	fused(double source)
	{
		ifrom(source);
	}

	fused& ifrom(double source)
	{
		*this = from(source);
		return *this;
	}

	double ito() const
	{
		return to(*this);
	}
	
	fused& operator+=(const fused& rhs)
	{
		auto ptr = reinterpret_cast<uint64_t*>(&store);
		auto ptr_rhs = reinterpret_cast<const uint64_t*>(&rhs.store);

		uint64_t carry = 0;
		for (int i=3; i >= 0; i--)
		{
			const bool neg = ptr[i] >> 63, neg_rhs = ptr_rhs[i] >> 63;
			ptr[i] += ptr_rhs[i] + carry;
			const bool neg_result = ptr[i] >> 63;
			if (neg && neg_rhs && !neg_result)
			{
				ptr[i] |= uint64_t(1) << 63;
				carry = 1;
			}
			else if (!neg && !neg_rhs && neg_result)
			{
				ptr[i] &= ~(uint64_t(1) << 63);
				carry = 1;
			}
			else carry = 0;
		}
		return *this;
	}

	fused operator+(const fused& rhs) const
	{
		fused copy(*this);
		return copy += rhs;
	}

	fused operator-() const
	{
		fused copy(*this);
		return copy.negate();
	}

	fused& operator-=(const fused& rhs)
	{
		return *this += -rhs;
	}

	fused operator-(const fused& rhs) const
	{
		fused copy(*this);
		return copy -= rhs;
	}

	fused& operator>>=(unsigned times)
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

	fused& operator<<=(unsigned times)
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

	fused& negate()
	{
		auto ptr = reinterpret_cast<uint64_t*>(&store);
		for (int i=0; i < 4; i++)
			ptr[i] = ~ptr[i];
		return *this += fused(0, 0, 0, 1);
	}

	bool is_negative() const
	{
		return store.a >> 63;
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
	static fused from(double source)
	{
		const auto sz_exponent = 11, sz_fraction = 52;

		auto ptr = reinterpret_cast<const uint64_t*>(&source);
		const uint64_t sign = *ptr >> 63;
		const uint64_t exponent = (*ptr << 1) >> (sz_fraction + 1);
		const uint64_t fraction = (*ptr << (sz_exponent + 1)) >> (sz_exponent + 1);
		const int64_t normalised_exponent = exponent - 1023;

		fused converted; // initialized to zero by the default constructor

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

		if (sign)
			converted.negate();

		return converted;
	}

	static double to(fused source)
	{
		const auto sz_exponent = 11, sz_fraction = 52;

		const auto neg = source.is_negative();
		if (neg)
			source.negate();

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
			const uint64_t sign = neg;
			const int64_t normalised_exponent = 127 - first_one;
			const uint64_t exponent = normalised_exponent + 1023;
			uint64_t fraction = 0;

			int i = first_one + 1;
			for (int j = 51; j >= 0 && i < 256; j--, i++)
			{
				const uint64_t bit = static_cast<bool>(source.bit(i));
				fraction |= bit << j;
			}

			*ptr = (sign << (sz_exponent+sz_fraction)) | (exponent << sz_fraction) | fraction;
		}

		return converted;
	}
};
