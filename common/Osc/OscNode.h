#pragma once

#include <functional>
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

	template<typename T> static bool getArgumentAs(const OscArgument& argument, T& v);

	void setOscParent(OscContainer* parent);
	const std::string& getFullAddress() const { return fullAddress; }
	const std::string& getName() const { return name; }
	virtual void dump() {}

	virtual bool visit(const std::function<bool(OscNode*)>* nodeVisitorFunction);
	virtual void execute(std::string_view address, const std::vector<OscArgument>& arguments);
	virtual bool notifyOscAtInit();
	virtual void notifyValueChanged();

	virtual std::string getAsString() const = 0;

	// Called from derived types when their value is changed
	void sendMessage(const OscArgument* arguments, size_t number);

protected:
	// Called by the sendMessage above and bubble the call to OscRoot instance
	virtual void sendMessage(const std::string& address, const OscArgument* arguments, size_t number);

	// Called by the public execute to really execute the action on this node (rather than descending through the tree
	// of nodes)
	virtual void execute(const std::vector<OscArgument>&) {}

	static constexpr const char* KEYS_NODE = "keys";

private:
	std::string name;
	std::string fullAddress;
	OscContainer* parent;
};

template<typename T> bool OscNode::getArgumentAs(const OscArgument& argument, T& v) {
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
