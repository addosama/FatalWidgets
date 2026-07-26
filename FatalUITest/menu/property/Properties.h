#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace Properties
{
	class AbstractProperty
	{
	public:
		virtual ~AbstractProperty() = default;
	};

	template <typename T>
	class PropBase : public AbstractProperty
	{
		T value;
	protected:
		virtual T processValue(T valIn)
		{
			return valIn;
		}
		explicit PropBase(T defaultValue)
			: AbstractProperty(), value(defaultValue)
		{}
	public:
		void set(T newVal)
		{
			value = processValue(newVal);
		}
		[[nodiscard]]
		T get() const
		{
			return value;
		}
	};
	class NumberProp : public PropBase<double>
	{
		double min;
		double max;
		double step;
	protected:
		double processValue(double valIn) override
		{
			return processEx(valIn, min, max, step);
		}
	public:
		static double processEx(double in, double mn, double mx, double step)
		{
			return std::clamp(std::round(in / step) * step, mn, mx);
		}

		double getMin() const
		{
			return min;
		}
		double getMax() const
		{
			return max;
		}
		double getStep() const
		{
			return step;
		}

		NumberProp(double minIn, double maxIn, double stepIn, double defaultValue)
			: PropBase(processEx(defaultValue, minIn, maxIn, stepIn)), min(minIn), max(maxIn), step(stepIn)
		{}
	};

	class BoolProperty : public PropBase<bool>
	{
	public:
		BoolProperty(bool defaultValue)
			: PropBase(defaultValue)
		{}
	};

	class PropDesc
	{
		AbstractProperty* prop;
		std::vector<PropDesc> subProps;
	public:
		[[nodiscard]]
		bool hasProp() const
		{
			return prop;
		}
		[[nodiscard]]
		bool hasSubProp() const
		{
			return !subProps.empty();
		}

		AbstractProperty* getProp() const
		{
			return prop;
		}
		const std::vector<PropDesc>& getSubProps() const
		{
			return subProps;
		}

		PropDesc(AbstractProperty* propIn, std::vector<PropDesc>&& subPropsIn = {})
			: prop(propIn), subProps(std::move(subPropsIn))
		{}
	};
}
