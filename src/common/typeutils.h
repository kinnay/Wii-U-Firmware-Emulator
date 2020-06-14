
#pragma once

#include <type_traits>

template <class... Dummy> struct ReturnType_;
template <class... Dummy> struct FirstArgument_;

template <class R, class C, class... Args>
struct ReturnType_<R (C::*)(Args...)> {
	typedef R type;
};

template <class R, class C, class A>
struct FirstArgument_<R (C::*)(A)> {
	typedef A type;
};

template <class T> using ReturnType = typename ReturnType_<T>::type;
template <class T> using FirstArgument = typename FirstArgument_<T>::type;
template <class T> using RemovePointer = typename std::remove_pointer<T>::type;

#define EnableImpl(...) typename std::enable_if_t<__VA_ARGS__> *
#define DisableImpl(...) typename std::enable_if_t<!(__VA_ARGS__)> *

#define EnableIf(...) EnableImpl(__VA_ARGS__) = nullptr
#define DisableIf(...) DisableImpl(__VA_ARGS__) = nullptr

#define IsEnum(T) std::is_enum<T>::value
#define Convertible(T1, T2) std::is_convertible<T1, T2>::value
