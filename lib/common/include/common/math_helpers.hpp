#include <type_traits>

template <typename T, typename U> constexpr auto divCeil(T numerator, U denominator)
{
    using CommonType = std::common_type_t<T, U>;
    return (static_cast<CommonType>(numerator) / static_cast<CommonType>(denominator))
        + ((static_cast<CommonType>(numerator) % static_cast<CommonType>(denominator)) != 0);
}

template <typename T> constexpr T binLog(T value)
{
    T log = 0;
    while (value >>= 1) {
        ++log;
    }
    return log;
}