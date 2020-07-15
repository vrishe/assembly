#pragma once

#ifndef UTILITY_HPP_
#define UTILITY_HPP_

#include <istream>
#include <type_traits>
#include <nmmintrin.h>


template<class CharT, class Traits>
static std::basic_istream<CharT, Traits>& ignore_all(
    std::basic_istream<CharT, Traits>& src,
    std::streamsize count = 1,
    typename Traits::int_type delim = Traits::to_int_type('\0')) {

  if (count > 0 && delim != Traits::eof()) {
    typename std::basic_istream<CharT, Traits>::sentry ok(src, true);
    if (ok) {
      std::ios_base::iostate state = std::ios_base::goodbit;

      try {
        typename Traits::int_type m = src.rdbuf()->sgetc();
        if (count == std::numeric_limits<std::streamsize>::max()) {
          for(;;) {
            if (Traits::eq_int_type(m, Traits::eof())) {
              state |= std::ios_base::eofbit;
              break;
            }

            if (!Traits::eq_int_type(m, delim)) {
              break;
            }

            m = src.rdbuf()->snextc();
          }
        }
        else do {
          if (Traits::eq_int_type(m, Traits::eof())) {
            state |= std::ios_base::eofbit;
            break;
          }

          if (!Traits::eq_int_type(m, delim)) {
            break;
          }

          m = src.rdbuf()->snextc();
        }
        while (--count);   
      }
      catch (...) {
        state |= std::ios_base::badbit;
        if (src.exceptions() & std::ios_base::badbit)
          throw;
      }

      src.setstate(state);
    }
  }

  return src;
}


template<class T, size_t N>
char (&countof_helper_(const T(&)[N]))[N];
#define countof_(obj) sizeof(countof_helper_(obj))


template<class T,
  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0
>
constexpr size_t ones(T val) {
  size_t result = 0;
  for (;val; val >>= 1) {
    result += (val & T( 1));
  }

  return result;
}

template<>
size_t ones(unsigned int val) {
  return _mm_popcnt_u32(val);
}

template<>
size_t ones(unsigned long long val) {
  return _mm_popcnt_u64(val);
}

template<class T,
  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0
>
constexpr size_t bitcount(T val) {
  size_t result = 1;
  while (val >>= 1) {
    ++result;
  }

  return result;
}

template<class T,
  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0
>
constexpr bool is_bit_set(T val, unsigned short bit) {
  return (val >> bit) & T(1);
}

static constexpr int BITS_PER_BYTE = bitcount((unsigned char)(-1));
#define bitsizeof_(expr) (sizeof(expr) * BITS_PER_BYTE)


template<class TEnum,
  typename std::enable_if<std::is_enum<TEnum>::value, int>::type = 0
>
constexpr auto as_integral(TEnum val) -> typename std::underlying_type<TEnum>::type {
  return static_cast<typename std::underlying_type<TEnum>::type>(val);
}

template<class TEnum,
  typename std::enable_if<std::is_enum<TEnum>::value, int>::type = 0
>
constexpr bool has_flag(typename std::underlying_type<TEnum>::type val, TEnum flag) {
  return (val & static_cast<typename std::underlying_type<TEnum>::type>(flag))
    == static_cast<typename std::underlying_type<TEnum>::type>(flag);
}

template<class TEnum,
  typename std::enable_if<std::is_enum<TEnum>::value, int>::type = 0
>
constexpr TEnum operator | (TEnum a, TEnum b) {
  using T = typename std::underlying_type<TEnum>::type;
  static_assert(std::is_unsigned<T>::value);

  return static_cast<TEnum>(static_cast<T>(a) | static_cast<T>(b));
}

template<class TEnum,
  typename std::enable_if<std::is_enum<TEnum>::value, int>::type = 0
>
constexpr TEnum operator |=(TEnum& a, TEnum b) {
  return a = a | b;
}

template<class N>
constexpr N round_up(size_t boundary, N value) {
  boundary -= 1;
  return (value + boundary) & ~boundary;
}

#endif // UTILITY_HPP_
