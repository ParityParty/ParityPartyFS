
template <typename T> constexpr T divCeil(T numerator, T denominator)
{
    return (numerator + denominator - 1) / denominator;
}

template <typename T> constexpr T binLog(T value)
{
    T log = 0;
    while (value >>= 1) {
        ++log;
    }
    return log;
}