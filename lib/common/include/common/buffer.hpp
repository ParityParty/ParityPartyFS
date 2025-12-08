template <typename T> struct buffer_base {
    virtual ~buffer_base() = default;
    virtual T& operator[](size_t idx) = 0;
    virtual const T& operator[](size_t idx) const = 0;
    virtual size_t size() const = 0;
    virtual size_t max_size() const = 0;
    virtual void push_back(const T& value) = 0;
};