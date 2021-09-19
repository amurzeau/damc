#pragma once

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
template<typename... Args> struct QNonConstOverload {
	template<typename R, typename T>
	Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...)) const noexcept -> decltype(ptr) {
		return ptr;
	}

	template<typename R, typename T> static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...)) noexcept -> decltype(ptr) {
		return ptr;
	}
};

template<typename... Args> struct QConstOverload {
	template<typename R, typename T>
	Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...) const) const noexcept -> decltype(ptr) {
		return ptr;
	}

	template<typename R, typename T>
	static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...) const) noexcept -> decltype(ptr) {
		return ptr;
	}
};

template<typename... Args> struct QOverload : QConstOverload<Args...>, QNonConstOverload<Args...> {
	using QConstOverload<Args...>::of;
	using QConstOverload<Args...>::operator();
	using QNonConstOverload<Args...>::of;
	using QNonConstOverload<Args...>::operator();

	template<typename R> Q_DECL_CONSTEXPR auto operator()(R (*ptr)(Args...)) const noexcept -> decltype(ptr) {
		return ptr;
	}

	template<typename R> static Q_DECL_CONSTEXPR auto of(R (*ptr)(Args...)) noexcept -> decltype(ptr) { return ptr; }
};

#if defined(__cpp_variable_templates) && __cpp_variable_templates >= 201304  // C++14
template<typename... Args> Q_CONSTEXPR Q_DECL_UNUSED QOverload<Args...> qOverload = {};
template<typename... Args> Q_CONSTEXPR Q_DECL_UNUSED QConstOverload<Args...> qConstOverload = {};
template<typename... Args> Q_CONSTEXPR Q_DECL_UNUSED QNonConstOverload<Args...> qNonConstOverload = {};
#endif
#endif