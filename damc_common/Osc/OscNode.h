#pragma once

#include <functional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

template<class T> struct osc_type_name {};

#define DEFINE_OSC_TYPE(type_) \
	template<> struct osc_type_name<type_> { \
		static constexpr const char* name = #type_; \
	}

DEFINE_OSC_TYPE(bool);
DEFINE_OSC_TYPE(int32_t);
DEFINE_OSC_TYPE(float);
DEFINE_OSC_TYPE(std::string);

#undef DEFINE_OSC_TYPE

#define EXPLICIT_INSTANCIATE_OSC_VARIABLE(prefix_, template_name_) \
	prefix_ class template_name_<bool>; \
	prefix_ class template_name_<int32_t>; \
	prefix_ class template_name_<float>; \
	prefix_ class template_name_<std::string>;

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
class OscRoot;

class OscNode {
public:
	OscNode(OscContainer* parent, std::string name) noexcept;
	OscNode(const OscNode&) = delete;
	OscNode& operator=(OscNode const&) = delete;
	virtual ~OscNode();

	template<typename T> bool getArgumentAs(const OscArgument& argument, T& v);

	void setOscParent(OscContainer* parent);
	const std::string& getFullAddress() const { return fullAddress; }
	const std::string& getName() const { return name; }
	virtual void dump() {}

	virtual void execute(std::string_view address, const std::vector<OscArgument>& arguments);

	virtual OscRoot* getRoot();

	virtual std::string getAsString() const = 0;

	// Called from derived types when their value is changed
	void sendMessage(const OscArgument* arguments, size_t number);

protected:
	friend class OscRoot;  // OscRoot calls execute on loadConfig
	// Called by the public execute to really execute the action on this node (rather than descending through the tree
	// of nodes)
	virtual void execute(const std::vector<OscArgument>&) {}

	static constexpr const char* KEYS_NODE = "keys";

private:
	std::string name;
	std::string fullAddress;
	OscContainer* parent;
};

extern template bool OscNode::getArgumentAs<bool>(const OscArgument& argument, bool& v);
extern template bool OscNode::getArgumentAs<int32_t>(const OscArgument& argument, int32_t& v);
extern template bool OscNode::getArgumentAs<float>(const OscArgument& argument, float& v);
extern template bool OscNode::getArgumentAs<std::string>(const OscArgument& argument, std::string& v);
