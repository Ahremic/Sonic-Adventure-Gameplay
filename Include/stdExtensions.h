#pragma once


namespace std
{
	float inline WrapFloat(const float number, const float bounds)
	{
		float result = number;

		if (number > bounds)
		{
			result = number - bounds;
		}

		if (number < 0)
		{
			result = number + bounds;
		}

		return result;
	}
	float inline fsign(const float num)
	{
		return num > 0 ? 1.0f : -1.0f;
	}
	bool inline approximately(const float lhs, const float rhs)
	{
		const float diff = lhs - rhs;
		const bool result = std::abs(diff) < std::numeric_limits<float>::epsilon();
		return result;
	}
	bool inline approximatelyOrGreater(const float lhs, const float rhs)
	{
		return approximately(lhs, rhs) || lhs > rhs;
	}
	bool inline approximatelyOrLess(const float lhs, const float rhs)
	{
		return approximately(lhs, rhs) || lhs < rhs;
	}

	float inline inverseLerpUnclamped(const float low, const float high, const float value)
	{
		const float div = high - low;
		if (std::abs(div) < std::numeric_limits<float>::epsilon()) return 0;
		return (value - low) / div;
	}

	float inline inverseLerp(const float low, const float high, const float value)
	{
		float result = inverseLerpUnclamped(low, high, value);
		if (result > 1.0f) result = 1.0f;
		if (result < 0.0f) result = 0.0f;
		return result;
	}

	float inline QuickPower(const float num, const int exp)
	{
		float result = 1.0f;
		float thisNum = num;
		int thisExp = exp;
		while (thisExp > 0)
		{
			if (thisExp % 2 == 1) result *= thisNum;
			thisExp >>= 1;
			thisNum *= thisNum;
		}
		return result;
	}

	float inline reversePower(const float value, const float exp)
	{
		const float result = 1 - std::pow(1 - value, exp);
		return result;
	}

	float inline reversePower(const float value, const int exp)
	{
		const bool isNegative = value < 0;
		const float _value = isNegative ? -value : value;

		const float result = 1 - QuickPower(1 - _value, exp);
		return isNegative ? -result : result;
	}
}
