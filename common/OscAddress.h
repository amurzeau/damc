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

	template<typename T> static bool getArgumentAs(const OscArgument& argument, T& v) {
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

	virtual void execute(std::string_view address, const std::vector<OscArgument>& arguments);

	virtual std::string getAsString() = 0;

	// Called from derived types when their value is changed
	void sendMessage(const OscArgument* arguments, size_t number);

protected:
	// Called by the sendMessage above and bubble the call to OscRoot instance
	virtual void sendMessage(const std::string& address, const OscArgument* arguments, size_t number);

	// Called by the public execute to really execute the action on this node (rather than descending through the tree
	// of nodes)
	virtual void execute(const std::vector<OscArgument>&) {}

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

template<typename T> class OscReadOnlyVariable : protected OscContainer {
public:
	using underlying_type = T;

	OscReadOnlyVariable(OscContainer* parent, std::string name, T initialValue = {}) noexcept
	    : OscContainer(parent, name), value(initialValue) {
		notifyOsc();
	}
	OscReadOnlyVariable(const OscReadOnlyVariable&) = delete;

	void set(T v) {
		if(value != v) {
			value = v;
			notifyOsc();
			if(onChange) {
				onChange(this->get());
			}
		}
	}

	T& get() { return value; }
	const T& get() const { return value; }

	template<typename U = T> std::enable_if_t<std::is_same_v<U, std::string>, const char*> c_str() {
		return value.c_str();
	}

	operator T() const { return value; }

	using OscContainer::operator=;
	OscReadOnlyVariable& operator=(const T& v) {
		set(v);
		return *this;
	}
	OscReadOnlyVariable& operator=(const OscReadOnlyVariable<T>& v) {
		set(v.value);
		return *this;
	}
	template<typename U = T>
	std::enable_if_t<std::is_same_v<U, std::string>, OscReadOnlyVariable&> operator=(const char* v) {
		set(v);
		return *this;
	}
	bool operator==(const OscReadOnlyVariable<T>& other) { return value == other.value; }
	bool operator!=(const OscReadOnlyVariable<T>& other) { return !(*this == other); }

	std::string getAsString() override {
		if constexpr(std::is_same_v<T, std::string>) {
			return "\"" + getToOsc() + "\"";
		} else {
			return std::to_string(getToOsc());
		}
	}

	void setOscConverters(std::function<T(T)> convertToOsc, std::function<T(T)> convertFromOsc) {
		this->convertToOsc = convertToOsc;
		this->convertFromOsc = convertFromOsc;
	}

	void setChangeCallback(std::function<void(T)> onChange) {
		this->onChange = onChange;
		onChange(this->get());
	}

protected:
	void notifyOsc() {
		OscArgument valueToSend = getToOsc();
		sendMessage(&valueToSend, 1);
	}

	T getToOsc() {
		if(!convertToOsc)
			return get();
		else
			return convertToOsc(get());
	}
	void setFromOsc(T value) {
		if(!convertFromOsc)
			set(value);
		else
			set(convertFromOsc(value));
	}

private:
	T value{};

	std::function<T(T)> convertToOsc;
	std::function<T(T)> convertFromOsc;
	std::function<void(T)> onChange;
};

template<typename T> class OscVariable : public OscReadOnlyVariable<T> {
public:
	OscVariable(OscContainer* parent, std::string name, T initialValue = {}) noexcept
	    : OscReadOnlyVariable<T>(parent, name, initialValue) {
		if constexpr(std::is_same_v<T, bool>) {
			subEndpoint.emplace_back(new OscEndpoint(this, "toggle"))->setCallback([this](auto) {
				this->setFromOsc(!this->getToOsc());
			});
		} else if constexpr(std::is_same_v<T, std::string>) {
			// No toggle/increment/decrement
		} else {
			incrementAmount = (T) 1;
			subEndpoint.emplace_back(new OscEndpoint(this, "increment"))->setCallback([this](auto) {
				this->setFromOsc(this->getToOsc() + incrementAmount);
			});
			subEndpoint.emplace_back(new OscEndpoint(this, "decrement"))->setCallback([this](auto) {
				this->setFromOsc(this->getToOsc() - incrementAmount);
			});
		}
	}

	using OscReadOnlyVariable<T>::operator=;
	OscVariable& operator=(const OscVariable<T>& v) {
		this->set(v.get());
		return *this;
	}

	void execute(const std::vector<OscArgument>& arguments) override {
		if(!arguments.empty()) {
			T v;
			if(this->template getArgumentAs<T>(arguments[0], v)) {
				this->setFromOsc(std::move(v));
			} else {
				printf("Bad argument 0 type: %d\n", arguments[0].index());
			}
		}
	}

	void setIncrementAmount(T amount) { incrementAmount = amount; }

private:
	T incrementAmount;
	std::vector<std::unique_ptr<OscEndpoint>> subEndpoint;
};

template<typename T> class OscGenericArray : protected OscContainer {
public:
	OscGenericArray(OscContainer* parent, std::string name) noexcept
	    : OscContainer(parent, name),
	      oscAddEndpoint(this, "add"),
	      oscRemoveEndpoint(this, "remove"),
	      oscSize(this, "size") {}

	T& operator[](size_t index) { return *value[index]; }
	const T& operator[](size_t index) const { return *value[index]; }

	template<typename... Args> void push_back(Args... args) {
		oscAddEndpoint.sendMessage(nullptr, 0);

		T* newValue = new T(this, std::to_string(value.size()), args...);

		initializeItem(newValue);
		value.emplace_back(newValue);

		oscSize.set(value.size());
	}

	void pop_back() {
		oscRemoveEndpoint.sendMessage(nullptr, 0);

		value.pop_back();

		oscSize.set(value.size());
	}

	template<typename... Args> void resize(size_t newSize, Args... args) {
		if(newSize == value.size())
			return;

		if(newSize < value.size()) {
			while(value.size() > newSize) {
				pop_back();
			}
		} else {
			while(value.size() < newSize) {
				push_back(args...);
			}
		}
	}

	auto size() const { return value.size(); }
	auto begin() const { return value.begin(); }
	auto begin() { return value.begin(); }
	auto end() const { return value.end(); }
	auto end() { return value.end(); }
	const auto& back() const { return *value.back(); }
	auto& back() { return *value.back(); }

	std::string getAsString() override {
		std::string result = "[";

		for(const auto& item : value) {
			result += " " + item->getAsString() + ",";
		}

		if(result.back() == ',')
			result.pop_back();

		return result + " ]";
	}

protected:
	virtual void initializeItem(T*) {}

	OscEndpoint oscAddEndpoint;
	OscEndpoint oscRemoveEndpoint;

private:
	OscReadOnlyVariable<int32_t> oscSize;
	std::vector<std::unique_ptr<T>> value;
};

template<typename T> class OscArray : public OscGenericArray<OscVariable<T>> {
public:
	OscArray(OscContainer* parent, std::string name) : OscGenericArray<OscVariable<T>>(parent, name) {
		this->oscAddEndpoint.setCallback([this](auto&&) {
			printf("Adding item to %s\n", this->getFullAddress().c_str());
			T v{};
			this->push_back(v);
			this->onAdd(v);
		});
		this->oscRemoveEndpoint.setCallback([this](auto&&) {
			printf("Removing last item from %s\n", this->getFullAddress().c_str());
			this->onRemove();
			this->pop_back();
		});
	}

	void setOscConverters(std::function<T(T)> convertToOsc, std::function<T(T)> convertFromOsc) {
		this->convertToOsc = convertToOsc;
		this->convertFromOsc = convertFromOsc;
	}
	void setChangeCallback(std::function<void(T)> onChange) { this->onChange = onChange; }

	void setAddCallback(std::function<void(T)> onAdd) { this->onAdd = onAdd; }
	void setRemoveCallback(std::function<void()> onRemove) { this->onRemove = onRemove; }

protected:
	void initializeItem(OscVariable<T>* item) override {
		if(convertToOsc || convertFromOsc) {
			item->setOscConverters(convertToOsc, convertFromOsc);
		}
		if(onChange) {
			item->setChangeCallback(onChange);
		}
	}

private:
	std::function<T(T)> convertToOsc;
	std::function<T(T)> convertFromOsc;
	std::function<void(T)> onChange;

	std::function<void(T)> onAdd;
	std::function<void()> onRemove;
};

template<typename T> class OscContainerArray : public OscGenericArray<T> {
public:
	OscContainerArray(OscContainer* parent, std::string name) : OscGenericArray<T>(parent, name) {
		this->oscAddEndpoint.setCallback([this](auto&&) {
			printf("Adding item to %s\n", this->getFullAddress().c_str());
			this->push_back();
			this->onAdd();
		});
		this->oscRemoveEndpoint.setCallback([this](auto&&) {
			printf("Removing last item from %s\n", this->getFullAddress().c_str());
			this->onRemove();
			this->pop_back();
		});
	}

	void setAddCallback(std::function<void()> onAdd) { this->onAdd = onAdd; }
	void setRemoveCallback(std::function<void()> onRemove) { this->onRemove = onRemove; }

private:
	std::function<void()> onAdd;
	std::function<void()> onRemove;
};

class ConverterLogScale {
public:
	static float fromOsc(float value);
	static float toOsc(float value);
};
