export module fused;

import <print>;
import <cstring>;
import <cstddef>;
import <cstdint>;
import <exception>;
import <format>;
import <limits>;
import <typeinfo>;

export template<size_t LEN_base>
class fused
{
private:
	static constexpr size_t LEN = LEN_base * 2, SZ = LEN * sizeof(uint64_t), SZ_bits = SZ * 8;

	struct __attribute__ ((__packed__)) {
		uint64_t integer[LEN_base], fraction[LEN_base];
	} store;
	uint64_t* const ptr = store.integer;
	const uint64_t* const ptr_const = store.integer;

public:
	fused()
	{
		static_assert(LEN_base >= 1);
		zeroes();
	}

	fused(const fused& other)
	{
		memcpy(ptr, other.ptr, SZ);
	}

	template<size_t LEN_base_other>
	fused(const fused<LEN_base_other>& other)
	{
		memset(store.integer, other.is_negative() ? 0xff : 0, LEN_base * sizeof(uint64_t));
		memset(store.fraction, 0, LEN_base * sizeof(uint64_t));

		const auto LEN_base_min = std::min(LEN_base, LEN_base_other);
		const auto SZ_base_min = LEN_base_min * sizeof(uint64_t);
		memcpy(ptr + (LEN_base - LEN_base_min), other.get_store() + (LEN_base_other - LEN_base_min), SZ_base_min);
		memcpy(store.fraction, other.get_store() + LEN_base_other, SZ_base_min);
	}

	template<class... Args>
	fused(Args... args)
	{
		load(args...);
	}

	template<class T>
	fused(T source)
	{
		ifrom(source);
	}

	const uint64_t* get_store() const
	{
		return store.integer;
	}

	template<class... Args>
	void load(Args... args)
	{
		static_assert(sizeof...(args) == LEN);
		load_recursive(args...);
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

	void constant(uint64_t value)
	{
		for (size_t i = 0; i < LEN; i++)
			*(ptr + i) = value;
	}

	void zeroes()
	{
		constant(0);
	}

	void ones()
	{
		constant(~uint64_t(0));
	}

	fused<LEN_base * 2> operator*(const fused& rhs)
	{
		fused<LEN_base * 2> product;
		fused<LEN_base * 2> copy(*this), copy_rhs(rhs);

		const bool neg_rhs = rhs.is_negative();

		for (ssize_t i = (SZ_bits * 2) - 1; i >= 0; i--)
		{
			const bool bit = copy_rhs.bit(i);
			if (bit)
				product += copy;
			copy <<= 1;
		}
		
		return product;
	}

	fused& operator+=(const fused& rhs)
	{
		auto ptr_rhs = rhs.ptr_const;

		uint64_t carry = 0;
		for (ssize_t i = LEN-1; i >= 0; i--)
		{
			const bool neg = ptr[i] >> 63, neg_rhs = ptr_rhs[i] >> 63;
			ptr[i] += ptr_rhs[i] + carry;
			const bool neg_result = ptr[i] >> 63;

			if (neg && neg_rhs && !neg_result)
			{
				// ptr[i] |= uint64_t(1) << 63;
				carry = 1;
			}
			else if (!neg && !neg_rhs && neg_result)
			{
				// ptr[i] &= ~(uint64_t(1) << 63);
				carry = 0;
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

	fused& operator>>=(size_t times)
	{
		const size_t real_times = times > SZ_bits ? SZ_bits : times;

		for (size_t i=0; i < real_times; i++)
		{
			uint64_t saved_bit = 0;
			for (size_t j=0; j < LEN; j++)
			{
				const uint64_t next_saved_bit = static_cast<bool>(ptr[j] & 0x1);
				ptr[j] >>= 1;
				ptr[j] |= saved_bit << 63;
				saved_bit = next_saved_bit;
			}
		}

		return *this;
	}

	fused& operator<<=(size_t times)
	{
		const size_t real_times = times > SZ_bits ? SZ_bits : times;

		for (size_t i=0; i < real_times; i++)
		{
			uint64_t saved_bit = 0;
			for (ssize_t j = LEN-1; j >= 0; j--)
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
		for (size_t i=0; i < LEN; i++)
			ptr[i] = ~ptr[i];

		fused<LEN_base> last_one;
		*(last_one.ptr + (LEN-1)) = 1;

		return *this += last_one;
	}

	bool is_negative() const
	{
		return store.integer[0] >> 63;
	}

	bool bit(size_t i) const
	{
		const auto unit = i / 64, at = i % 64, at_real = 63 - at;
		return (ptr[unit] >> at_real) & 0x1;
	}

	void bit_set(size_t i)
	{
		const auto unit = i / 64, at = i % 64, at_real = 63 - at;
		ptr[unit] |= 1 << at_real;
	}
	
	void bit_clear(size_t i)
	{
		const auto unit = i / 64, at = i % 64, at_real = 63 - at;
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
	template<class Head, class... Rest>
	void load_recursive(Head head, Rest... rest)
	{
		static_assert(typeid(Head) == typeid(uint64_t));

		const size_t index = (LEN_base * 2) - (1 + sizeof...(rest));
		*(ptr + index) = head;

		if constexpr (sizeof...(rest) > 0)
			load_recursive(rest...);
	}
	
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

		const size_t \
			sz_exponent = typeid(T) == typeid(float) ? 8 : 11,
			sz_fraction = typeid(T) == typeid(float) ? 23 : 52,
			sz_all = sizeof(T) * 8, sz_left = 64 - sz_all;

		const Tsi exp_bias = typeid(T) == typeid(float) ? 127 : 1023;
		const Ti exp_max = typeid(T) == typeid(float) ? 0xff : 0x7ff;

		auto ptr_source = reinterpret_cast<const Ti*>(&source);
		const Ti sign = *ptr_source >> (sz_exponent + sz_fraction);
		const Ti exponent = (*ptr_source << 1) >> (sz_fraction + 1);
		const Ti fraction = (*ptr_source << (sz_exponent + 1)) >> (sz_exponent + 1);
		const Tsi normalised_exponent = exponent - exp_bias;

		fused converted; // initialized to zero by the default constructor

		bool did_work = false;
		if (*ptr_source == 0 || *ptr_source == Ti(1) << (sz_all-1)) // zero
		{
			// already zero
		}
		else if (exponent == exp_max) // infinity and nan
		{
			// do nothing
		}
		else if (exponent == 0 && fraction != 0) // subnormal
		{
			converted.store.fraction[0] = uint64_t(fraction) << (sz_exponent + sz_left + 1);
			did_work = true;
		}
		else // normal
		{
			converted.store.integer[LEN_base-1] = 1;
			converted.store.fraction[0] = uint64_t(fraction) << (sz_exponent + sz_left + 1);
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

		const size_t \
			sz_exponent = typeid(T) == typeid(float) ? 8 : 11,
			sz_fraction = typeid(T) == typeid(float) ? 23 : 52,
			sz_all = sizeof(T) * 8;

		const Tsi exp_bias = typeid(T) == typeid(float) ? 127 : 1023;

		const bool neg = source.is_negative();
		if (neg)
			source.negate();

		ssize_t first_one = -1;
		for (size_t i=0; i < SZ_bits; i++)
		{
			if (source.bit(i))
			{
				first_one = i;
				break;
			}
		}

		T converted = 0;
		auto ptr_converted = reinterpret_cast<Ti*>(&converted);

		if (first_one == -1) // zero
		{
			// already zero
		}
		else
		{
			const Ti sign = neg;
			const Tsi normalised_exponent = ((SZ_bits / 2) - 1) - first_one;
			const Ti exponent = normalised_exponent + exp_bias;
			Ti fraction = 0;

			ssize_t i = first_one + 1;
			for (ssize_t j = sz_fraction-1; j >= 0 && i < SZ_bits; j--, i++)
			{
				const Ti bit = static_cast<bool>(source.bit(i));
				fraction |= bit << j;
			}

			*ptr_converted = (sign << (sz_exponent + sz_fraction)) | (exponent << sz_fraction) | fraction;
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
