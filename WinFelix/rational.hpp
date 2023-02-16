#ifndef _RATIONAL_H
#define _RATIONAL_H

#include <cstddef>
#include <stdexcept>
#include <type_traits>

#ifndef RATIONAL_NCONSTEXPR
#define CONSTEXPR constexpr
#else
#define CONSTEXPR /* constexpr */
#endif

namespace rational {

    struct NoReduceTag {};

    struct DivideByZeroException : public std::runtime_error {
        DivideByZeroException() noexcept :
            runtime_error("Division by zero is undefined.")
        {  }
    };

    struct OverflowException : public std::runtime_error {
        OverflowException() noexcept :
            runtime_error("Arithmetic operation resulted in an overflow.")
        {  }
    };

    namespace detail {

        template<class T>
        inline CONSTEXPR T cpow(const T base, unsigned const exponent) noexcept
        {
            return (exponent == 0) ? 1 : (base * cpow(base, exponent-1));
        }

        template<class T>
        inline T gcd(T a, T b) noexcept {
            if(a < 0) a = -a;
            if(b < 0) b = -b;
            while(b != 0) {
                a %= b;
                if(a == 0) {
                    return b;
                }
                b %= a;
            }
            return a;
        }
        
    } // namespace detail

    template<class T>
    struct Ratio {
    public:

        static_assert(std::is_integral<T>::value, "Ratio<T>: T must meet the requirements of Integral");

        const T numer;
        const T denom;

        inline CONSTEXPR Ratio() noexcept :
            numer(0),
            denom(1)
        {  }
        
        inline CONSTEXPR Ratio(const T& numer) noexcept :
            numer(numer),
            denom(1)
        {
        }

        inline Ratio(const T& numer, const T& denom) :
            numer(numer),
            denom(denom)
        {
            if(denom == 0) {
                throw DivideByZeroException();
            }
            *this = reduce(*this);
        }

        inline CONSTEXPR Ratio(const T& numer, const T& denom, NoReduceTag) noexcept :
            numer(numer),
            denom(denom)
        {  }

        inline CONSTEXPR Ratio(const Ratio& r) noexcept :
            numer(r.numer),
            denom(r.denom)
        {  }

        inline Ratio& operator=(const Ratio& r) noexcept
        {
            new (this) Ratio<T>(r);
            return *this;
        }

        inline static CONSTEXPR T to_integer(const Ratio& r) noexcept
        {
            return T(r.numer / r.denom);
        }

        template<class Float = double>
        inline static CONSTEXPR Float to_float(const Ratio& r) noexcept
        {
            return Float(Float(r.numer) / Float(r.denom));
        }

        template<class Float = float>
        inline static Ratio from_float(const Float& u, const unsigned prec)
        {
            const Ratio r (T(u * detail::cpow(10, prec)), T(detail::cpow(10, prec)));
            if((u > 0 && u > r.numer)
                || (u < 0 && u < r.numer)) {
                throw OverflowException();
            }
            return r;
        }

        inline static CONSTEXPR Ratio zero() noexcept
        {
            return Ratio(0, 1, NoReduceTag());
        }

        inline static CONSTEXPR Ratio one() noexcept
        {
            return Ratio(1, 1, NoReduceTag());
        }

        inline static CONSTEXPR Ratio pi() noexcept
        {
            return Ratio(6283, 2000, NoReduceTag());
        }

        template<class U>
        inline explicit CONSTEXPR operator Ratio<U>() const noexcept
        {
            return Ratio<U>(U(this->numer), U(this->denom), NoReduceTag());
        }

        template<class Float>
        inline explicit CONSTEXPR operator Float() const noexcept
        {
            return to_float<Float>(*this);
        }

        inline friend Ratio<T> operator+(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return Ratio<T>((lhs.numer * rhs.denom) + (lhs.denom * rhs.numer), (lhs.denom * rhs.denom));
        }

        inline friend Ratio<T> operator-(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return Ratio<T>((lhs.numer * rhs.denom) - (lhs.denom * rhs.numer), (lhs.denom * rhs.denom));
        }

        inline friend Ratio<T> operator%(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return Ratio<T>((lhs.numer * rhs.denom) % (lhs.denom * rhs.numer), (lhs.denom * rhs.denom));
        }

        inline friend Ratio<T> operator*(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return Ratio<T>((lhs.numer * rhs.numer), (lhs.denom * rhs.denom));
        }

        inline friend Ratio<T> operator/(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return Ratio<T>((lhs.numer * rhs.denom), (lhs.denom * rhs.numer));
        }

        inline friend Ratio<T> operator+=(Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return lhs = lhs + rhs;
        }

        inline friend Ratio<T> operator-=(Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return lhs = lhs - rhs;
        }

        inline friend Ratio<T> operator*=(Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return lhs = lhs * rhs;
        }

        inline friend Ratio<T> operator/=(Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return lhs = lhs / rhs;
        }

        inline friend Ratio<T> operator%=(Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return lhs = lhs % rhs;
        }

        inline friend bool operator==(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return (lhs.numer * lhs.denom) == (rhs.numer * rhs.denom);
        }

        inline friend bool operator!=(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        inline friend bool operator>(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return (lhs.numer * lhs.denom) > (rhs.numer * rhs.denom);
        }

        inline friend bool operator<(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return (rhs > lhs);
        }

        inline friend bool operator<=(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return !(lhs > rhs);
        }

        inline friend bool operator>=(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
        {
            return !(lhs < rhs);
        }
            
    };

    template<class T>
    inline Ratio<T> operator+(const Ratio<T>& r) noexcept
    {
        return Ratio<T>(r);
    }

    template<class T>
    inline Ratio<T> operator-(const Ratio<T>& r) noexcept
    {
        return Ratio<T>(-r.numer, r.denom, NoReduceTag());
    }

    template<class T>
    inline Ratio<T>& operator++(Ratio<T>& r) noexcept
    {
        return r = Ratio<T>(r.numer + r.denom, r.denom);
    }

    template<class T>
    inline Ratio<T>& operator--(Ratio<T>& r) noexcept
    {
        return r = Ratio<T>(r.numer - r.denom, r.denom);
    }

    template<class T>
    inline Ratio<T> operator++(Ratio<T>& r, int) noexcept
    {
        const Ratio<T> cpy = r;
        ++r;
        return cpy;
    }

    template<class T>
    inline Ratio<T> operator--(Ratio<T>& r, int) noexcept
    {
        const Ratio<T> cpy = r;
        --r;
        return cpy;
    }

    template<class T>
    inline bool is_integer(const Ratio<T>& r) noexcept
    {
        return r.denom == T(1);
    }
    template<class T>
    inline bool is_zero(const Ratio<T>& r) noexcept
    {
        return (r.numer == 0);
    }

    template<class T>
    inline bool is_positive(const Ratio<T>& r) noexcept
    {
        return (r.numer > 0);
    }

    template<class T>
    inline bool is_negative(const Ratio<T>& r) noexcept
    {
        return (r.numer < 0);
    }

    template<class T>
    inline Ratio<T> reduce(const Ratio<T>& r) noexcept
    {
        T numer = r.numer;
        T denom = r.denom;
        
        const T g = detail::gcd(numer, denom);

        numer /= g;
        denom /= g;

        if(denom < T(0)) {
            return Ratio<T>(-numer, -denom, NoReduceTag());
        } else {
            return Ratio<T>(numer, denom, NoReduceTag());
        }
    }

    template<class T>
    inline Ratio<T> floor(const Ratio<T>& r) noexcept
    {
        const T numer = r.numer;
        const T denom = r.denom;

        if(is_negative(r)) {
            return Ratio<T>((numer - denom + T(1)) / denom);
        } else {
            return Ratio<T>(numer / denom);
        }
    }

    template<class T>
    inline Ratio<T> ceil(const Ratio<T>& r) noexcept
    {
        const T numer = r.numer;
        const T denom = r.denom;
        
        if(is_negative(r)) {
            return Ratio<T>(numer / denom);
        } else {
            return Ratio<T>((numer + denom - T(1)) / denom);
        }
    }

    template<class T>
    inline Ratio<T> round(const Ratio<T>& r) noexcept
    {
        const Ratio<T> zero = Ratio<T>::zero();
        const T one(1);
        const T two(one + one);

        Ratio<T> fractional = fract(r);
        if(fractional < zero) {
            fractional = zero - fractional;
        }

        const bool half_or_larger;
        if(fractional.denom % 2 == 0) {
            half_or_larger = fractional.numer >= fractional.denom / two;
        } else {
            half_or_larger = fractional.numer >= (fractional.denom / two) + one;
        }

        if(half_or_larger) {
            if(is_positive(r)) {
                return trunc(r) + Ratio<T>::one();
            } else {
                return trunc(r) - Ratio<T>::one();
            }
        } else {
            return trunc(r);
        }
    }

    template<class T>
    inline Ratio<T> trunc(const Ratio<T>& r) noexcept
    {
        return Ratio<T>(r.numer / r.denom);
    }

    template<class T>
    inline Ratio<T> fract(const Ratio<T>& r) noexcept
    {
        return Ratio<T>(r.numer % r.denom, r.denom, NoReduceTag());
    }

    template<class T>
    inline Ratio<T> recip(const Ratio<T>& r) noexcept
    {
        return Ratio<T>(r.denom, r.numer, NoReduceTag());
    }

    template<class T, class F>
    inline Ratio<T> pow(const Ratio<T>& r, int expon, F&& tpow) noexcept
    {
        if(expon == 0) {
            return Ratio<T>(1);
        } else if(expon < 0) {
            return pow(recip(r), -expon);
        } else {
            return Ratio<T>(tpow(r.numer, expon), tpow(r.numer, expon));
        }
    }

    template<class T>
    inline Ratio<T> abs(const Ratio<T>& r) noexcept
    {
        return (is_negative(r) ? -r : r);
    }
    
    template<class T>
    inline Ratio<T> abs_sub(const Ratio<T>& lhs, const Ratio<T>& rhs) noexcept
    {
        return abs(lhs - rhs);
    }

    template<class T>
    inline Ratio<T> signum(const Ratio<T>& r) noexcept
    {
        if(is_zero(r)) {
            return Ratio<T>(0);
        } else if(is_positive(r)) {
            return Ratio<T>(1);
        } else {
            return Ratio<T>(-1);
        }
    }

    template<class T, class F>
    inline Ratio<T> sqrt(const Ratio<T>& r, F&& sqrt) noexcept
    {
        return Ratio<T>(sqrt(r.numer), sqrt(r.denom));
    }

    typedef Ratio<int> Rational;
    typedef Ratio<std::int32_t> Rational32;
    typedef Ratio<std::int64_t> Rational64;

} // namespace rational

#endif