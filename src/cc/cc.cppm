export module cc;
export import :fused;

import <cstdint>;

export enum currency_t
{
	us, in, ru
};

export class currency
{
private:
	fused_t value;
	currency_t type;
	
public:
	currency()
		:type(currency_t::in), value(0)
	{}
	
	currency(fused_t value, currency_t type)
		:value(value), type(type)
	{}

	currency(double value, currency_t type)
		:type(type)
	{
		load(value);
	}

public:
	void load(double value)
	{
		this->value = fused_t::from(value);
	}
	void get(double& value)
	{
		value = fused_t::to(this->value);
	}
	double get()
	{
		return fused_t::to(this->value);
	}

	void load(fused_t value)
	{
		this->value = value;
	}
	void get(fused_t& value)
	{
		value = this->value;
	}
};
