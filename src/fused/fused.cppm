export module fused;

import <format>;
import <exception>;
import <limits>;
import <typeinfo>;
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

	template<class T>
	fused(T source)
	{
		ifrom(source);
	}

	template<class T>
	fused& ifrom(T source)
	{
		return *this = from(source);
	}

	float itof() const
	{
		return tof(*this);
	}

	double ito() const
	{
		return to(*this);
	}

	void zeroes()
	{
		auto ptr = reinterpret_cast<uint64_t*>(&store);
		for (unsigned i = 0; i < 4; i++, ptr++)
			*ptr = 0;
	}

	void ones()
	{
		auto ptr = reinterpret_cast<uint64_t*>(&store);
		for (unsigned i = 0; i < 4; i++, ptr++)
			*ptr = ~uint64_t(0);
	}

	fused operator*(const fused& rhs)
	{
		exception::enact("Multiplication is unimplemented");

		fused product;
		return product;
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
	static fused from(float source)
	{
		return fused::from_raw<float, uint32_t, int32_t>(source);
	}

	static fused from(double source)
	{
		return fused::from_raw<double, uint64_t, int64_t>(source);
	}

	static float tof(fused source)
	{
		return fused::to_raw<float, uint32_t, int32_t>(source);
	}

	static double to(fused source)
	{
		return fused::to_raw<double, uint64_t, int64_t>(source);
	}

private:
	template<class T, class Ti, class Tsi>
	static void are_valid_conversion_types()
	{
		static_assert(std::numeric_limits<T>::is_iec559);
		static_assert(sizeof(T) == sizeof(Ti) && !std::numeric_limits<Ti>::is_signed && !std::numeric_limits<Ti>::is_iec559);
		static_assert(sizeof(T) == sizeof(Tsi) && std::numeric_limits<Tsi>::is_signed && !std::numeric_limits<Tsi>::is_iec559);
		static_assert((typeid(T) == typeid(float) && sizeof(float) == 4) ||
					  (typeid(T) == typeid(double) && sizeof(double) == 8));
	}

	template<class T, class Ti, class Tsi>
	static fused from_raw(T source)
	{
		are_valid_conversion_types<T, Ti, Tsi>();

		const std::size_t sz_exponent = typeid(T) == typeid(float) ? 8 : 11,
			sz_fraction = typeid(T) == typeid(float) ? 23 : 52,
			sz_all = sizeof(T) * 8, sz_left = 64 - sz_all;
		const Tsi exp_bias = typeid(T) == typeid(float) ? 127 : 1023;
		const Ti exp_max = typeid(T) == typeid(float) ? 0xff : 0x7ff;

		auto ptr = reinterpret_cast<const Ti*>(&source);
		const Ti sign = *ptr >> (sz_exponent + sz_fraction);
		const Ti exponent = (*ptr << 1) >> (sz_fraction + 1);
		const Ti fraction = (*ptr << (sz_exponent + 1)) >> (sz_exponent + 1);
		const Tsi normalised_exponent = exponent - exp_bias;

		fused converted; // initialized to zero by the default constructor

		bool did_work = false;
		if (*ptr == 0 || *ptr == Ti(1) << (sz_all-1)) // zero
		{
			// already zero
		}
		else if (exponent == exp_max) // infinity and nan
		{
			// do nothing
		}
		else if (exponent == 0 && fraction != 0) // subnormal
		{
			converted.store.c = uint64_t(fraction) << (sz_exponent + sz_left + 1);
			did_work = true;
		}
		else // normal
		{
			converted.store.b = 1;
			converted.store.c = uint64_t(fraction) << (sz_exponent + sz_left + 1);
			if (normalised_exponent >= 0)
				converted <<= normalised_exponent;
			else
				converted >>= -normalised_exponent;
			did_work = true;
		}

		if (sign && did_work)
			converted.negate();

		return converted;
	}

	template<class T, class Ti, class Tsi>
	static T to_raw(fused source)
	{
		are_valid_conversion_types<T, Ti, Tsi>();

		const std::size_t sz_exponent = typeid(T) == typeid(float) ? 8 : 11,
			sz_fraction = typeid(T) == typeid(float) ? 23 : 52,
			sz_all = sizeof(T) * 8;
		const Tsi exp_bias = typeid(T) == typeid(float) ? 127 : 1023;

		const auto neg = source.is_negative();
		if (neg)
			source.negate();

		int first_one = -1;
		for (int i=0; i < 256; i++)
		{
			if (source.bit(i))
			{
				first_one = i;
				break;
			}
		}

		T converted = 0;
		auto ptr = reinterpret_cast<Ti*>(&converted);

		if (first_one == -1) // zero
		{
			// already zero
		}
		else
		{
			const Ti sign = neg;
			const Tsi normalised_exponent = 127 - first_one;
			const Ti exponent = normalised_exponent + exp_bias;
			Ti fraction = 0;

			int i = first_one + 1;
			for (int j = sz_fraction-1; j >= 0 && i < 256; j--, i++)
			{
				const Ti bit = static_cast<bool>(source.bit(i));
				fraction |= bit << j;
			}

			*ptr = (sign << (sz_exponent + sz_fraction)) | (exponent << sz_fraction) | fraction;
		}

		return converted;
	}

public:
	class exception : public std::runtime_error
	{
	public:
		exception(std::string_view msg) :std::runtime_error(msg.data()) {}

		template<class... Args>
		static void enact(std::string_view format, Args&&... args)
		{
			std::string format_real("fused: ");
			format_real += format;
			throw exception(std::vformat(format_real, std::make_format_args<Args...>(args...)));
		}
	};
};
