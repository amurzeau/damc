#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

using OscArgument = std::variant<bool, int32_t, float, std::string>;

template<typename T> inline constexpr bool always_false_v = false;

template<typename> struct tag {};  // <== this one IS literal

template<typename T, typename V> struct get_index;

template<typename T, typename... Ts>
struct get_index<T, std::variant<Ts...>> : std::integral_constant<size_t, std::variant<tag<Ts>...>(tag<T>()).index()> {
};

template<typename... ExpectedTypes> bool check_osc_arguments(const std::vector<OscArgument>& args) {
	std::initializer_list<size_t> types_index = {get_index<ExpectedTypes, OscArgument>::value...};

	if(args.size() != types_index.size())
		return false;

	auto it = types_index.begin();

	for(const OscArgument& arg : args) {
		if(arg.index() != *it)
			return false;
		++it;
	}

	return true;
}

class OscContainer;

class OscNode {
public:
	OscNode(OscContainer* parent, std::string name) noexcept;
	OscNode(const OscNode&) = delete;
	OscNode& operator=(OscNode const&) = delete;
	virtual ~OscNode();

	template<typename T> bool getArgumentAs(const OscArgument& argument, T& v) {
		bool ret = false;
		std::visit(
		    [&v, &ret](auto&& arg) -> void {
			    using U = std::decay_t<decltype(arg)>;
			    if constexpr(std::is_same_v<U, T>) {
				    v = arg;
				    ret = true;
			    } else if constexpr(!std::is_same_v<U, std::string> && !std::is_same_v<T, std::string>) {
				    v = (T) arg;
				    ret = true;
			    }
		    },
		    argument);

		return ret;
	}

	const std::string& getFullAddress();

	void sendMessage(const OscArgument* arguments, size_t number);
	void sendMessage(const std::string& address, const OscArgument* arguments, size_t number);

	virtual void execute(std::string_view address, const std::vector<OscArgument>& arguments);
	virtual void execute(const std::vector<OscArgument>&) {}

	virtual std::string getAsString() = 0;

protected:
	template<class Converter, typename T> static T getFromOsc(T value) {
		if constexpr(std::is_void_v<Converter>) {
			return value;
		} else {
			return Converter::toOsc(value);
		}
	}
	template<class Converter, typename T> static T setFromOsc(T v) {
		if constexpr(std::is_void_v<Converter>) {
			return v;
		} else {
			return Converter::fromOsc(v);
		}
	}

private:
	std::string name;
	std::string fullAddress;
	OscContainer* parent;
};

class OscContainer : public OscNode {
public:
	using OscNode::OscNode;

	void addChild(std::string name, OscNode* child);
	void removeChild(std::string name);

	using OscNode::execute;
	void execute(std::string_view address, const std::vector<OscArgument>& arguments) override;

	std::string getAsString() override;

private:
	std::map<std::string, OscNode*> children;
};

class OscEndpoint : public OscNode {
public:
	using OscNode::OscNode;
	void setCallback(std::function<void(std::vector<OscArgument>)> onExecute);

	void execute(const std::vector<OscArgument>& arguments) override;

	std::string getAsString() override { return std::string{}; }

private:
	std::function<void(std::vector<OscArgument>)> onExecute;
};

template<typename T, class Converter = void> class OscReadOnlyVariable : protected OscContainer {
public:
	OscReadOnlyVariable(OscContainer* parent, std::string name, T initialValue) noexcept
	    : OscContainer(parent, name), value(initialValue) {}

	void set(T v) {
		value = v;
		notifyOsc();
	}

	T get() { return value; }

	std::string getAsString() override {
		if constexpr(std::is_same_v<T, std::string>) {
			return value;
		} else {
			return std::to_string(value);
		}
	}

protected:
	template<typename TArray, class ConverterArray> friend class OscArray;

	void notifyOsc() {
		OscArgument valueToSend = OscNode::getFromOsc<Converter>(value);
		sendMessage(&valueToSend, 1);
	}

private:
	T value{};
};

template<typename T, class Converter = void> class OscVariable : public OscReadOnlyVariable<T, Converter> {
public:
	OscVariable(OscContainer* parent, std::string name, T initialValue) noexcept
	    : OscReadOnlyVariable<T, Converter>(parent, name, initialValue) {
		if constexpr(std::is_same_v<T, bool>) {
			subEndpoint.emplace_back(new OscEndpoint(this, "toggle"))->setCallback([this](auto) {
				this->setFromOsc(!OscNode::getFromOsc<Converter>(this->get()));
				this->notifyOsc();
			});
		} else {
			subEndpoint.emplace_back(new OscEndpoint(this, "increment"))->setCallback([this](auto) {
				this->setFromOsc(OscNode::getFromOsc<Converter>(this->get()) + incrementAmount);
				this->notifyOsc();
			});
			subEndpoint.emplace_back(new OscEndpoint(this, "decrement"))->setCallback([this](auto) {
				this->setFromOsc(OscNode::getFromOsc<Converter>(this->get()) - incrementAmount);
				this->notifyOsc();
			});
		}
	}

	void execute(const std::vector<OscArgument>& arguments) override {
		if(!arguments.empty()) {
			T v;
			if(this->template getArgumentAs<T>(arguments[0], v)) {
				setFromOsc(std::move(v));
			} else {
				printf("Bad argument 0 type: %d\n", arguments[0].index());
			}
		}
	}

	void setIncrementAmount(T amount) { incrementAmount = amount; }

	void setChangeCallback(std::function<void(T)> onChange) { this->onChange = onChange; }

protected:
	void setFromOsc(T v) {
		this->set(OscNode::setFromOsc<Converter>(v));
		if(onChange) {
			onChange(this->get());
		}
	}

private:
	T incrementAmount = (T) 1;

	std::function<void(T)> onChange;
	std::vector<std::unique_ptr<OscEndpoint>> subEndpoint;
};

template<typename T, class Converter = void> class OscArray : protected OscContainer {
public:
	OscArray(OscContainer* parent, std::string name) noexcept
	    : OscContainer(parent, name),
	      size(this, "size"),
	      oscAddEndpoint(this, "add"),
	      oscRemoveEndpoint(this, "remove") {
		oscAddEndpoint.setCallback([this](auto&& arguments) {
			if(!arguments.empty()) {
				T argument;
				if(this->template getArgumentAs<T>(arguments[0], argument)) {
					printf("Adding item to %s\n", getFullAddress().c_str());
					T v = OscNode::setFromOsc<Converter>(argument);
					this->push_back(v);
					this->onAdd(v);
				} else {
					printf("Bad argument 0 type: %d\n", arguments[0].index());
				}
			}
		});
		oscRemoveEndpoint.setCallback([this](auto&& arguments) {
			if(!check_osc_arguments<int32_t>(arguments))
				return;

			int32_t index = std::get<int32_t>(arguments[0]);
			printf("Removing item at index %d from %s\n", index, getFullAddress().c_str());

			this->onRemove();
			this->erase(index);
		});
	}

	OscVariable<T>& operator[](size_t index) { return *value[index]; }
	const OscVariable<T>& operator[](size_t index) const { return *value[index]; }

	void push_back(T v) {
		std::array<OscArgument, 2> arguments;

		value.emplace_back(new OscVariable<T>(this, value.size() - 1, v));

		arguments[0] = int32_t(value.size() - 1);
		arguments[1] = OscNode::getFromOsc<Converter>(v);
		oscAddEndpoint.sendMessage(arguments.data(), arguments.size());

		size.set(value.size());
	}

	void pop_back() {
		std::array<OscArgument, 2> arguments;

		arguments[0] = int32_t(value.size() - 1);
		arguments[1] = OscNode::getFromOsc<Converter>(value.back()->get());
		oscRemoveEndpoint.sendMessage(arguments.data(), arguments.size());

		value.pop_back();

		size.set(value.size());
	}

	void resize(size_t newSize, T defaultValue = {}) {
		if(newSize == value.size())
			return;

		if(newSize < value.size()) {
			while(value.size() > newSize) {
				pop_back();
			}
		} else {
			while(value.size() < newSize) {
				push_back(defaultValue);
			}
		}
	}

	void setAddCallback(std::function<void(T)> onAdd) { this->onAdd = onAdd; }
	void setRemoveCallback(std::function<void()> onRemove) { this->onRemove = onRemove; }

	std::string getAsString() override {
		std::string result = "[";

		for(const auto& item : value) {
			result += " " + item->getAsString() + ",";
		}

		if(result.back() == ',')
			result.pop_back();

		return result + " ]";
	}

private:
	OscReadOnlyVariable<int32_t> size;
	std::vector<std::unique_ptr<OscVariable<T>>> value;

	std::function<void(T)> onAdd;
	std::function<void()> onRemove;
	OscEndpoint oscAddEndpoint;
	OscEndpoint oscRemoveEndpoint;
};

class ConverterLogScale {
public:
	static float fromOsc(float value);
	static float toOsc(float value);
};
