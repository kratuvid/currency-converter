export module cc;

import <cstdint>;

import http;
import fused;

export enum currency_t
{
	us, in, ru
};

export class currency
{
private:
	using fused_internal = fused<2>;

	fused_internal value;
	currency_t type;

	/*
public:
	currency()
		:type(currency_t::in), value(0.f)
	{}
	
	currency(fused_internal value, currency_t type)
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
		this->value = fused_internal::from(value);
	}
	void get(double& value)
	{
		value = fused_internal::to(this->value);
	}
	double get()
	{
		return fused_internal::to(this->value);
	}

	void load(fused_internal value)
	{
		this->value = value;
	}
	void get(fused_internal& value)
	{
		value = this->value;
	}
	*/
};
