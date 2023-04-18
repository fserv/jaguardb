#ifndef _jag_defer_h_
#define _jag_defer_h_

template <typename Func>
struct privDefer {
    Func f;
    privDefer(Func f) : f(f) {}
    ~privDefer() { f(); }
};

template <typename Func>
privDefer<Func> defer_func(Func f) {
    return privDefer<Func>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})

#endif
