
template <typename T> constexpr T div_ceil(T numerator, T denominator)
{
    return (numerator + denominator - 1) / denominator;
}