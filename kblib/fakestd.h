#ifndef KBLIB_FAKESTD_H
#define KBLIB_FAKESTD_H

#include "tdecl.h"

#include <array>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#if __has_include("gsl/pointers")
#include "gsl/pointers"
#endif

#ifndef KBLIB_FAKESTD
#define KBLIB_FAKESTD (__cplusplus < 201703L)
#endif

/**
 * @file fakestd.h
 * @brief This header provides some features of C++17 <type_traits> and other
 * headers for C++14, as well as some other traits.
 */

namespace kblib {

template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template <typename T>
using decay_t = typename std::decay<T>::type;

template <bool v>
using bool_constant = std::integral_constant<bool, v>;

namespace detail {

	template <
	    typename F, typename... Args,
	    enable_if_t<not std::is_member_pointer<decay_t<F>>::value, int> = 0>
	constexpr decltype(auto) do_invoke(F&& f, Args&&... args) noexcept(
	    noexcept(std::forward<F>(f)(std::forward<Args>(args)...))) {
		return std::forward<F>(f)(std::forward<Args>(args)...);
	}

	template <typename F, typename Object, typename... Args,
	          enable_if_t<not std::is_pointer<decay_t<Object>>::value and
	                          std::is_member_function_pointer<F>::value,
	                      int> = 0>
	constexpr decltype(auto)
	do_invoke(F f, Object&& obj, Args&&... args) noexcept(
	    noexcept((obj.*f)(std::forward<Args>(args)...))) {
		return (obj.*f)(std::forward<Args>(args)...);
	}

	template <typename F, typename Pointer, typename... Args,
	          enable_if_t<std::is_pointer<Pointer>::value and
	                          std::is_member_function_pointer<F>::value,
	                      int> = 0>
	constexpr decltype(auto)
	do_invoke(F f, Pointer ptr, Args&&... args) noexcept(
	    noexcept((ptr->*f)(std::forward<Args>(args)...))) {
		return (ptr->*f)(std::forward<Args>(args)...);
	}

	template <typename Member, typename Object,
	          enable_if_t<not std::is_pointer<decay_t<Object>>::value and
	                          std::is_member_object_pointer<Member>::value,
	                      int> = 0>
	constexpr decltype(auto) do_invoke(Member mem, Object&& obj) noexcept {
		return std::forward<Object>(obj).*mem;
	}

	template <typename Member, typename Pointer,
	          enable_if_t<std::is_pointer<Pointer>::value and
	                          std::is_member_object_pointer<Member>::value,
	                      int> = 0>
	constexpr decltype(auto) do_invoke(Member mem, Pointer ptr) noexcept {
		return ptr.*mem;
	}
} // namespace detail

template <typename F, typename... Args>
constexpr decltype(auto) invoke(F&& f, Args&&... args) noexcept(noexcept(
    detail::do_invoke(std::forward<F>(f), std::forward<Args>(args)...))) {
#if KBLIB_USE_CXX17
	return std::apply(std::forward<F>(f),
	                  std::forward_as_tuple(std::forward<Args>(args)...));
#else
	return detail::do_invoke(std::forward<F>(f), std::forward<Args>(args)...);
#endif
}

/**
 * @namespace kblib::fakestd
 * @brief A namespace which holds all the C++14 implementations of C++17
 * standard library features. In C++17, it is simply defined as an alias to std.
 */
#if KBLIB_FAKESTD
namespace fakestd { // C++14 implementation of C++17 void_t, invoke_result,
	                 // (partially) is_invocable, and is_nothrow_swappable
	using std::swap;

	/**
	 * @namespace kblib::fakestd::detail
	 * @brief Implementation details for kblib::fakestd features
	 *
	 * @internal
	 */
	namespace detail {

		template <typename AlwaysVoid, typename, typename...>
		struct invoke_result {};
		template <typename F, typename... Args>
		struct invoke_result<decltype(void(invoke(std::declval<F>(),
		                                          std::declval<Args>()...))),
		                     F, Args...> {
			using type =
			    decltype(invoke(std::declval<F>(), std::declval<Args>()...));
		};
	} // namespace detail
	template <class F, class... ArgTypes>
	struct invoke_result : detail::invoke_result<void, F, ArgTypes...> {};

	template <typename F, typename... ArgTypes>
	using invoke_result_t = typename invoke_result<F, ArgTypes...>::type;

	template <typename... Ts>
	struct make_void {
		typedef void type;
	};
	template <typename... Ts>
	using void_t = typename make_void<Ts...>::type;

	namespace detail {
		// ALL generic swap overloads MUST already have a declaration available at
		// this point.

		struct nat {
			nat() = delete;
			nat(const nat&) = delete;
			nat& operator=(const nat&) = delete;
			~nat() = delete;
		};

		struct two {
			char lx[2];
		};

		struct is_referenceable_impl {
			template <class Tp>
			static Tp& test(int);
			template <class Tp>
			static two test(...);
		};

		template <class Tp>
		struct is_referenceable
		    : std::integral_constant<
		          bool,
		          not std::is_same<decltype(is_referenceable_impl::test<Tp>(0)),
		                           two>::value> {};

		template <class Tp, class Up = Tp,
		          bool NotVoid =
		              not std::is_void<Tp>::value and not std::is_void<Up>::value>
		struct swappable_with {
			template <class LHS, class RHS>
			static decltype(swap(std::declval<LHS>(), std::declval<RHS>()))
			test_swap(int);
			template <class, class>
			static nat test_swap(long);

			// Extra parens are needed for the C++03 definition of decltype.
			typedef decltype((test_swap<Tp, Up>(0))) swap1;
			typedef decltype((test_swap<Up, Tp>(0))) swap2;

			static const bool value = not std::is_same<swap1, nat>::value and
			                          not std::is_same<swap2, nat>::value;
		};

		template <class Tp, class Up>
		struct swappable_with<Tp, Up, false> : std::false_type {};

		template <class Tp, class Up = Tp,
		          bool Swappable = swappable_with<Tp, Up>::value>
		struct nothrow_swappable_with {
			static const bool value =
			    noexcept(swap(std::declval<Tp>(), std::declval<Up>()))and noexcept(
			        swap(std::declval<Up>(), std::declval<Tp>()));
		};

		template <class Tp, class Up>
		struct nothrow_swappable_with<Tp, Up, false> : std::false_type {};

	} // namespace detail

	template <class Tp>
	struct is_swappable
	    : public std::integral_constant<bool,
	                                    detail::swappable_with<Tp&>::value> {};

	template <class Tp>
	struct is_nothrow_swappable
	    : public std::integral_constant<
	          bool, detail::nothrow_swappable_with<Tp&>::value> {};

#if KBLIB_USE_CXX17

	template <class Tp, class Up>
	struct is_swappable_with
	    : public std::integral_constant<bool,
	                                    detail::swappable_with<Tp, Up>::value> {
	};

	template <class Tp>
	struct is_swappable
	    : public std::conditional<
	          detail::is_referenceable<Tp>::value,
	          is_swappable_with<typename std::add_lvalue_reference<Tp>::type,
	                            typename std::add_lvalue_reference<Tp>::type>,
	          std::false_type>::type {};

	template <class Tp, class Up>
	struct is_nothrow_swappable_with
	    : public integral_constant<
	          bool, detail::nothrow_swappable_with<Tp, Up>::value> {};

	template <class Tp>
	struct is_nothrow_swappable
	    : public conditional<
	          detail::is_referenceable<Tp>::value,
	          is_nothrow_swappable_with<typename add_lvalue_reference<Tp>::type,
	                                    typename add_lvalue_reference<Tp>::type>,
	          false_type>::type {};

	template <class Tp, class Up>
	constexpr bool is_swappable_with_v = is_swappable_with<Tp, Up>::value;

	template <class Tp>
	constexpr bool is_swappable_v = is_swappable<Tp>::value;

	template <class Tp, class Up>
	constexpr bool is_nothrow_swappable_with_v =
	    is_nothrow_swappable_with<Tp, Up>::value;

	template <class Tp>
	constexpr bool is_nothrow_swappable_v = is_nothrow_swappable<Tp>::value;

#endif

	namespace detail {
		template <typename F>
		struct not_fn_t {
			constexpr explicit not_fn_t(F&& f) : fd(std::forward<F>(f)) {}
			constexpr not_fn_t(const not_fn_t&) = default;
			constexpr not_fn_t(not_fn_t&&) = default;

			template <class... Args>
			constexpr auto operator()(Args&&... args) & -> decltype(
			    not std::declval<invoke_result_t<std::decay_t<F>&, Args...>>()) {
				return not invoke(fd, std::forward<Args>(args)...);
			}

			template <class... Args>
			constexpr auto operator()(Args&&... args) const& -> decltype(
			    not std::declval<
			        invoke_result_t<std::decay_t<F> const&, Args...>>()) {
				return not invoke(std::move(fd), std::forward<Args>(args)...);
			}

			std::decay_t<F> fd;
		};
	} // namespace detail

	template <typename F>
	detail::not_fn_t<F> not_fn(F&& f) {
		return detail::not_fn_t<F>(std::forward<F>(f));
	}

	struct in_place_t {
		explicit in_place_t() = default;
	};
	static constexpr in_place_t in_place{};

	template <class ForwardIt>
	constexpr ForwardIt max_element(ForwardIt first, ForwardIt last) {
		if (first == last)
			return last;

		ForwardIt largest = first;
		++first;
		for (; first != last; ++first) {
			if (*largest < *first) {
				largest = first;
			}
		}
		return largest;
	}

	template <class ForwardIt, class Compare>
	constexpr ForwardIt max_element(ForwardIt first, ForwardIt last,
	                                Compare comp) {
		if (first == last)
			return last;

		ForwardIt largest = first;
		++first;
		for (; first != last; ++first) {
			if (comp(*largest, *first)) {
				largest = first;
			}
		}
		return largest;
	}

	template <class C>
	constexpr auto size(const C& c) -> decltype(c.size()) {
		return c.size();
	}

	template <class T, std::size_t N>
	constexpr std::size_t size(const T (&)[N]) noexcept {
		return N;
	}

	// Adapted from libstdc++ code, licensed under GPL

	namespace detail {
		// invokable
		template <class Ret, class Fp, class... Args>
		struct invokable_r {
			template <class XFp, class... XArgs>
			static auto try_call(int)
			    -> decltype(kblib::invoke(std::declval<XFp>(),
			                              std::declval<XArgs>()...));
			template <class XFp, class... XArgs>
			static detail::nat try_call(...);

			using Result = decltype(try_call<Fp, Args...>(0));

			using type = typename std::conditional<
			    not std::is_same<Result, detail::nat>::value,
			    typename std::conditional<std::is_void<Ret>::value, std::true_type,
			                              std::is_convertible<Result, Ret>>::type,
			    std::false_type>::type;
			static const bool value = type::value;
		};
		template <class Fp, class... Args>
		using invokable = invokable_r<void, Fp, Args...>;

		template <bool IsInvokable, bool IsCVVoid, class Ret, class Fp,
		          class... Args>
		struct nothrow_invokable_r_imp {
			static const bool value = false;
		};

		template <class Ret, class Fp, class... Args>
		struct nothrow_invokable_r_imp<true, false, Ret, Fp, Args...> {
			typedef nothrow_invokable_r_imp ThisT;

			template <class Tp>
			static void test_noexcept(Tp) noexcept;

			static const bool value = noexcept(ThisT::test_noexcept<Ret>(
			    kblib::invoke(std::declval<Fp>(), std::declval<Args>()...)));
		};

		template <class Ret, class Fp, class... Args>
		struct nothrow_invokable_r_imp<true, true, Ret, Fp, Args...> {
			static const bool value = noexcept(
			    kblib::invoke(std::declval<Fp>(), std::declval<Args>()...));
		};

		template <class Ret, class Fp, class... Args>
		using nothrow_invokable_r =
		    nothrow_invokable_r_imp<invokable_r<Ret, Fp, Args...>::value,
		                            std::is_void<Ret>::value, Ret, Fp, Args...>;

		template <class Fp, class... Args>
		using nothrow_invokable =
		    nothrow_invokable_r_imp<invokable<Fp, Args...>::value, true, void, Fp,
		                            Args...>;

		template <class Fp, class... Args>
		struct invoke_of : public std::enable_if<
		                       invokable<Fp, Args...>::value,
		                       typename invokable_r<void, Fp, Args...>::Result> {
		};
	} // namespace detail

	// is_invocable

	template <class Fn, class... Args>
	struct is_invocable
	    : std::integral_constant<bool, detail::invokable<Fn, Args...>::value> {};

	template <class Ret, class Fn, class... Args>
	struct is_invocable_r
	    : std::integral_constant<bool,
	                             detail::invokable_r<Ret, Fn, Args...>::value> {
	};

	template <class Fn, class... Args>
	constexpr bool is_invocable_v = is_invocable<Fn, Args...>::value;

	template <class Ret, class Fn, class... Args>
	constexpr bool is_invocable_r_v = is_invocable_r<Ret, Fn, Args...>::value;

	// is_nothrow_invocable

	template <class Fn, class... Args>
	struct is_nothrow_invocable
	    : std::integral_constant<bool,
	                             detail::nothrow_invokable<Fn, Args...>::value> {
	};

	template <class Ret, class Fn, class... Args>
	struct is_nothrow_invocable_r
	    : std::integral_constant<
	          bool, detail::nothrow_invokable_r<Ret, Fn, Args...>::value> {};

	template <class Fn, class... Args>
	constexpr bool is_nothrow_invocable_v =
	    is_nothrow_invocable<Fn, Args...>::value;

	template <class Ret, class Fn, class... Args>
	constexpr bool is_nothrow_invocable_r_v =
	    is_nothrow_invocable_r<Ret, Fn, Args...>::value;

} // namespace fakestd
#else
namespace fakestd = std;
#endif

using fakestd::is_invocable;
using fakestd::is_invocable_v;

using fakestd::is_invocable_r;
using fakestd::is_invocable_r_v;

using fakestd::is_nothrow_invocable;
using fakestd::is_nothrow_invocable_v;

using fakestd::is_nothrow_invocable_r;
using fakestd::is_nothrow_invocable_r_v;

template <typename... Ts>
struct meta_type {};

template <typename T>
struct meta_type<T> {
	using type = T;
};

template <typename... Ts>
using meta_type_t = typename meta_type<Ts...>::type;

template <bool>
struct void_if {};

template <>
struct void_if<true> : meta_type<void> {};
template <bool b>
using void_if_t = typename void_if<b>::type;

using fakestd::void_t;

// metafunction_success:
// SFINAE detector for a ::type member type
template <typename T, typename = void>
struct metafunction_success : std::false_type {};

template <typename T>
struct metafunction_success<T, void_t<typename T::type>> : std::true_type {};

template <typename... T>
struct is_callable : metafunction_success<fakestd::invoke_result<T...>> {};

template <typename T>
using metafunction_value_t =
    std::integral_constant<decltype(T::value), T::value>;

/**
 * @brief Essentially just like std::enable_if, but with a different name that
 * makes it clearer what it does in the context of return type SFINAE.
 */
template <bool V, typename T>
struct return_assert {};

template <typename T>
struct return_assert<true, T> : meta_type<T> {};

template <bool V, typename T>
using return_assert_t = typename return_assert<V, T>::type;

namespace detail {

	template <typename F, typename Arg, typename = void>
	struct apply_impl {
		template <std::size_t... Is>
		constexpr static auto do_apply(F&& f, Arg&& arg) noexcept(
		    noexcept(kblib::invoke(std::forward<F>(f),
		                           std::get<Is>(std::forward<Arg>(arg))...)))
		    -> decltype(auto) {
			return kblib::invoke(std::forward<F>(f),
			                     std::get<Is>(std::forward<Arg>(arg))...);
		}
	};

} // namespace detail

template <typename F, typename Arg>
constexpr auto
apply(F&& f, Arg&& arg) noexcept(noexcept(detail::apply_impl<F, Arg>::do_apply(
    std::forward<F>(f), std::forward<Arg>(arg),
    std::index_sequence<std::tuple_size<Arg>::value>{}))) -> decltype(auto) {
	return detail::apply_impl<F, Arg>::do_apply(
	    std::forward<F>(f), std::forward<Arg>(arg),
	    std::index_sequence<std::tuple_size<Arg>::value>{});
}

template <typename T>
std::unique_ptr<T> to_unique(gsl::owner<T*> p) {
	return std::unique_ptr<T>(p);
}
template <typename T, typename D>
std::unique_ptr<T, D> to_unique(gsl::owner<T*> p, D&& d) {
	return std::unique_ptr<T, D>(p, d);
}

/**
 * @brief Cast integral argument to corresponding unsigned type
 */
template <typename I>
constexpr std::make_unsigned_t<I> to_unsigned(I x) {
	return static_cast<std::make_unsigned_t<I>>(x);
}
/**
 * @brief Cast integral argument to corresponding signed type
 */
template <typename I>
constexpr std::make_signed_t<I> to_signed(I x) {
	return static_cast<std::make_signed_t<I>>(x);
}

/**
 * @brief Cast argument to equivalently-sized type with the same signednessas
 * the template parameter
 */
template <typename A, typename F>
KBLIB_NODISCARD constexpr enable_if_t<std::is_integral<A>::value and
                                          std::is_integral<F>::value and
                                          std::is_signed<A>::value,
                                      std::make_signed_t<F>>
signed_cast(F x) {
	return to_signed(x);
}

/**
 * @brief Cast argument to equivalently-sized type with the same signednessas
 * the template parameter
 */
template <typename A, typename F>
KBLIB_NODISCARD constexpr enable_if_t<std::is_integral<A>::value and
                                          std::is_integral<F>::value and
                                          std::is_unsigned<A>::value,
                                      std::make_unsigned_t<F>>
signed_cast(F x) {
	return to_unsigned(x);
}

template <typename T>
struct has_member_swap {
	using yes = char (&)[1];
	using no = char (&)[2];

	template <typename C>
	static yes check(decltype(&C::swap));
	template <typename>
	static no check(...);

	constexpr static bool value = sizeof(check<T>(0)) == sizeof(yes);
};

template <typename T, typename = void>
struct is_tuple_like : std::false_type {};

template <typename T>
struct is_tuple_like<T, void_t<typename std::tuple_element<0, T>::type>>
    : std::true_type {};

namespace detail {

	template <typename... Ts>
	constexpr void ignore(Ts&&... /*unused*/) noexcept {}

	template <typename T, std::size_t... Is>
	constexpr void
	swap_tuple_impl(T& a, T& b, std::index_sequence<Is...> /*unused*/) noexcept(
	    noexcept(ignore(((void)swap(std::get<Is>(a), std::get<Is>(b)), 0)...))) {
		ignore(((void)swap(std::get<Is>(a), std::get<Is>(b)), 0)...);
	}

} // namespace detail

[[maybe_unused]] constexpr inline struct {
	/**
	 * @brief Swaps two objects, using move operations.
	 *
	 * @param a,b The objects that will be swapped.
	 */
	template <typename T, enable_if_t<not has_member_swap<T>::value and
	                                      not is_tuple_like<T>::value,
	                                  int> = 0>
	constexpr void operator()(T& a, T& b) const
	    noexcept(std::is_nothrow_move_constructible<T>::value and
	                 std::is_nothrow_move_assignable<T>::value) {
		auto tmp = std::move(a);
		a = std::move(b);
		b = std::move(tmp);
		return;
	}

	/**
	 * @brief Swaps two objects, using a member swap function, if detected.
	 *
	 * @param a,b The objects that will be swapped.
	 */
	template <typename T, enable_if_t<has_member_swap<T>::value, int> = 0>
	constexpr void operator()(T& a, T& b) const noexcept(noexcept(a.swap(b))) {
		a.swap(b);
		return;
	}

	/**
	 * @brief Swaps two arrays elementwise.
	 *
	 * @param a,b The arrays that will be swapped.
	 */
	template <typename T, std::size_t N>
	constexpr void operator()(T (&a)[N], T (&b)[N]) const
	    noexcept(std::is_nothrow_move_constructible<T>::value and
	                 std::is_nothrow_move_assignable<T>::value) {
		for (std::size_t i = 0; i < N; ++i) {
			swap(a[i], b[i]);
		}
	}

	/**
	 * @brief Swaps two tuples elementwise.
	 *
	 * @param a,b The tuples that will be swapped.
	 */
	template <typename T, enable_if_t<is_tuple_like<T>::value and
	                                      not has_member_swap<T>::value,
	                                  std::size_t>
	                          N = std::tuple_size<T>::value>
	constexpr void operator()(T& a, T& b) const noexcept(
	    noexcept(detail::swap_tuple_impl(a, b, std::make_index_sequence<N>{}))) {
		detail::swap_tuple_impl(a, b, std::make_index_sequence<N>{});
	}
} swap;

#if KBLIB_USE_CXX17

namespace detail {

	template <typename T>
	constexpr std::intmax_t max_val = std::numeric_limits<T>::max();

	constexpr std::uintmax_t msb(std::uintmax_t x) {
		x |= (x >> 1u);
		x |= (x >> 2u);
		x |= (x >> 4u);
		x |= (x >> 8u);
		x |= (x >> 16u);
		x |= (x >> 32u);
		return (x & ~(x >> 1u));
	}

	template <typename Num>
	constexpr Num msb_possible() {
		return static_cast<Num>(typename std::make_unsigned<Num>::type{1}
		                        << (std::numeric_limits<Num>::digits - 1u));
	}

	template <class... Args>
	struct type_list {
		template <std::size_t N>
		using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
	};

	template <auto K, typename V>
	struct type_map_el {
		constexpr static auto key = K;
		using value = V;
	};

	template <typename Key, typename Comp, typename... Vals>
	struct type_map {
		using types = type_list<Vals...>;
		template <std::size_t I>
		using element = typename types::template type<I>;

		template <Key key, std::size_t I = 0>
		constexpr static auto get() {
			static_assert(I < sizeof...(Vals), "key not found");
			if constexpr (Comp{}(key, element<I>::key)) {
				return tag<typename element<I>::value>{};
			} else {
				return get<key, I + 1>();
			}
		}

		template <Key key, typename Default = void, std::size_t I = 0>
		constexpr static auto get_default() {
			if constexpr (I == sizeof...(Vals)) {
				return Default();
			} else if constexpr (Comp{}(key, element<I>::key)) {
				return tag<typename element<I>::value>{};
			} else {
				return get<key, I + 1>();
			}
		}
	};

	template <typename N>
	using make_smap_el =
	    type_map_el<static_cast<std::intmax_t>(msb_possible<N>()), N>;

	template <typename T>
	struct next_larger_signed {
		static_assert(max_val<T> < max_val<std::intmax_t>,
		              "Cannot safely promote intmax_t.");
		struct false_compare {
			template <typename U>
			constexpr bool operator()(U, U) {
				return true;
			}
		};

		using ints_map = type_map<
		    std::intmax_t, std::less<>, make_smap_el<std::int_least8_t>,
		    make_smap_el<std::int_least16_t>, make_smap_el<std::int_least32_t>,
		    make_smap_el<std::int_least64_t>, make_smap_el<std::intmax_t>>;

		using type = typename decltype(
		    ints_map::template get_default<max_val<T> + 1>())::type;
	};

	template <typename N, bool = std::is_signed<N>::value>
	struct filter_signed;

	template <typename N>
	struct filter_signed<N, true> {
		using type = N;
	};

	template <typename N>
	using filter_signed_t = typename filter_signed<N>::type;

	template <typename N, bool = std::is_unsigned<N>::value>
	struct filter_unsigned;

	template <typename N>
	struct filter_unsigned<N, true> {
		using type = N;
	};

	template <typename N>
	using filter_unsigned_t = typename filter_unsigned<N>::type;

} // namespace detail

template <typename N, typename = void>
struct safe_signed;

template <typename N>
struct safe_signed<N, std::enable_if_t<std::is_integral<N>::value, void>> {
	using type = std::conditional_t<
	    std::is_signed<N>::value, N,
	    typename detail::next_larger_signed<std::make_signed_t<N>>::type>;
};

template <typename N>
using safe_signed_t = typename safe_signed<N>::type;

template <typename N>
KBLIB_NODISCARD constexpr safe_signed_t<N> signed_promote(N x) noexcept {
	return static_cast<safe_signed_t<N>>(x);
}

#endif

template <typename C, typename T,
          bool = std::is_const<typename std::remove_reference<C>::type>::value>
struct copy_const : meta_type<T> {};

template <typename C, typename T>
struct copy_const<C, T, true> : meta_type<const T> {};

template <typename C, typename T>
struct copy_const<C, T&, true> : meta_type<const T&> {};

template <typename C, typename T>
struct copy_const<C, T&&, true> : meta_type<const T&&> {};

template <typename C, typename V>
using copy_const_t = typename copy_const<C, V>::type;

template <typename T, typename = void>
struct value_detected : std::false_type {};

template <typename T>
struct value_detected<T, void_t<typename T::value_type>> : std::true_type {
	using type = typename T::value_type;
};

template <typename T>
constexpr bool value_detected_v = value_detected<T>::value;
template <typename T>
using value_detected_t = typename value_detected<T>::type;

template <typename T, typename = void>
struct key_detected : std::false_type {};

template <typename T>
struct key_detected<T, void_t<typename T::key_type>> : std::true_type {
	using type = typename T::key_type;
};

template <typename T>
constexpr bool key_detected_v = key_detected<T>::value;
template <typename T>
using key_detected_t = typename key_detected<T>::type;

template <typename T, typename = void>
struct mapped_detected : std::false_type {};

template <typename T>
struct mapped_detected<T, void_t<typename T::mapped_type>> : std::true_type {
	using type = typename T::mapped_type;
};

template <typename T>
constexpr bool mapped_detected_v = mapped_detected<T>::value;
template <typename T>
using mapped_detected_t = typename mapped_detected<T>::type;

template <typename T, typename = void>
struct hash_detected : std::false_type {};

template <typename T>
struct hash_detected<T, void_t<typename T::hasher>> : std::true_type {
	using type = typename T::hasher;
};

template <typename T>
constexpr bool hash_detected_v = hash_detected<T>::value;
template <typename T>
using hash_detected_t = typename hash_detected<T>::type;

template <typename Container, bool = key_detected_v<Container>,
          typename T = typename Container::value_type>
struct value_type_linear {};

template <typename Container>
struct value_type_linear<Container, false, typename Container::value_type>
    : meta_type<typename Container::value_type> {};

template <typename Container>
using value_type_linear_t = typename value_type_linear<Container>::type;

template <typename Container>
constexpr static bool is_linear_container_v =
    value_detected_v<Container> and not key_detected_v<Container>;

template <typename Container>
struct is_linear_container : bool_constant<is_linear_container_v<Container>> {};

template <typename Container, bool = key_detected_v<Container>,
          bool = mapped_detected_v<Container>>
struct key_type_setlike {};

template <typename Container>
struct key_type_setlike<Container, true, false>
    : meta_type<typename Container::key_type> {};

template <typename Container>
using key_type_setlike_t = typename key_type_setlike<Container>::type;

template <typename Container>
constexpr static bool
    is_setlike_v = (key_detected_v<Container> and
                    value_detected_v<Container> and
                    not mapped_detected_v<Container> and
                    std::is_same<key_detected_t<Container>,
                                 value_detected_t<Container>>::value);

template <class InputIt1, class InputIt2>
constexpr bool equal(InputIt1 first1, InputIt1 last1, InputIt2 first2) {
	for (; first1 != last1; ++first1, ++first2) {
		if (not(*first1 == *first2)) {
			return false;
		}
	}
	return true;
}

template <typename C>
constexpr auto size(const C& c) -> decltype(c.size()) {
	return c.size();
}

template <typename T, std::size_t N>
constexpr std::size_t size(const T (&)[N]) noexcept {
	return N;
}

template <class InputIt1, class InputIt2>
KBLIB_NODISCARD constexpr bool
lexicographical_compare(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                        InputIt2 last2) {
	for (; (first1 != last1) and (first2 != last2); ++first1, (void)++first2) {
		if (*first1 < *first2)
			return true;
		if (*first2 < *first1)
			return false;
	}
	return (first1 == last1) and (first2 != last2);
}

namespace detail {
	template <typename D, typename T, typename = void>
	struct pointer {
		using type = T*;
	};

	template <typename D, typename T>
	struct pointer<D, T, void_t<typename D::pointer>> {
		using type = typename D::pointer;
	};

} // namespace detail

constexpr struct in_place_agg_t {
} in_place_agg;

template <typename T>
class heap_value {
 public:
	using element_type = T;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;

	constexpr heap_value() noexcept : p{nullptr} {}
	constexpr heap_value(std::nullptr_t) noexcept : p{nullptr} {}

	template <typename... Args,
	          enable_if_t<std::is_constructible<T, Args...>::value> = 0>
	heap_value(fakestd::in_place_t, Args&&... args) : p{new T(args...)} {}
	template <typename... Args,
	          enable_if_t<std::is_constructible<T, Args...>::value> = 0>
	heap_value(in_place_agg_t, Args&&... args) : p{new T{args...}} {}

	heap_value(const heap_value& u) : p{(u.p ? (new T(*u.p)) : nullptr)} {}
	heap_value(heap_value&& u) noexcept : p{std::exchange(u.p, nullptr)} {}

	heap_value& operator=(const heap_value& u) & {
		if (this == &u) {
			return *this;
		} else if (not u) {
			reset();
		} else if (p) {
			*p = *u;
		} else {
			p = new T(*u.p);
		}
		return *this;
	}

	heap_value& operator=(heap_value&& u) & noexcept {
		if (this == &u) {
			return *this;
		}
		reset();
		p = std::exchange(u.p, nullptr);
		return *this;
	}

	heap_value& operator=(const T& val) & {
		if (this == &val) {
			return *this;
		}
		reset();
		p = new T(val);
	}
	heap_value& operator=(T&& val) & {
		if (this == &val) {
			return *this;
		}
		reset();
		p = new T(std::move(val));
	}

	void assign() & {
		reset();
		p = new T();
	}
	void assign(const T& val) & {
		reset();
		p = new T(val);
	}
	void assign(T&& val) & {
		reset();
		p = new T(std::move(val));
	}
	template <typename... Args>
	void assign(fakestd::in_place_t, Args&&... args) {
		reset();
		p = new T(std::forward<Args>(args)...);
	}
	template <typename... Args>
	void assign(in_place_agg_t, Args&&... args) {
		reset();
		p = new T{std::forward<Args>(args)...};
	}

	void reset() & {
		delete p;
		p = nullptr;
		return;
	}

	KBLIB_NODISCARD explicit operator bool() const& noexcept {
		return p != nullptr;
	}

	constexpr void swap(heap_value& other) noexcept { kblib::swap(p, other.p); }

	KBLIB_NODISCARD pointer get() & noexcept { return p; }
	KBLIB_NODISCARD const_pointer get() const& noexcept { return p; }

	KBLIB_NODISCARD reference value() & noexcept { return *p; }
	KBLIB_NODISCARD const_reference value() const& noexcept { return *p; }
	KBLIB_NODISCARD T&& value() && noexcept { return *p; }
	KBLIB_NODISCARD const T&& value() const&& noexcept { return *p; }

	KBLIB_NODISCARD reference operator*() & noexcept { return *p; }
	KBLIB_NODISCARD const_reference operator*() const& noexcept { return *p; }
	KBLIB_NODISCARD T&& operator*() && noexcept { return *p; }
	KBLIB_NODISCARD const T&& operator*() const&& noexcept { return *p; }

	KBLIB_NODISCARD pointer operator->() & noexcept { return p; }
	KBLIB_NODISCARD const_pointer operator->() const& noexcept { return p; }

	~heap_value() { delete p; }

 private:
	pointer p;
};

} // namespace kblib

#endif // KBLIB_FAKESTD_H
