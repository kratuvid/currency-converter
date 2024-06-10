export module cc;

import <cstdint>;

import fused;

export enum currency_t
{
	us, in, ru
};

export class currency
{
private:
	fused value;
	currency_t type;
	
public:
	currency()
		:type(currency_t::in), value(0.f)
	{}
	
	currency(fused value, currency_t type)
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
		this->value = fused::from(value);
	}
	void get(double& value)
	{
		value = fused::to(this->value);
	}
	double get()
	{
		return fused::to(this->value);
	}

	void load(fused value)
	{
		this->value = value;
	}
	void get(fused& value)
	{
		value = this->value;
	}
};
