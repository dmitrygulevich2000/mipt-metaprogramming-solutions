#ifndef MIPT_METAPROGRAMMING_SPY_HPP
#define MIPT_METAPROGRAMMING_SPY_HPP

#include <concepts>
#include <functional>
#include <utility>

struct LoggerInvokeTable {
    void (* log)(void*, unsigned int) = nullptr;
    void (* destroy)(void*) = nullptr;
    void* (* copy)(void*) = nullptr;
};

template <class T>
class Spy {
public:
    using LogFuncPtr = void(*)(void*, unsigned int);

    struct ExprScopeObserver {
        T* t;
        void* logger;
        LogFuncPtr log;
        unsigned int* n;

        ExprScopeObserver(T* t, void* logger, LogFuncPtr log, unsigned int* counter):
                t(t), logger(logger), log(log), n(counter) {}

        ~ExprScopeObserver() {
            if (*n > 0) {
                if (logger != nullptr) {
                    log(logger, *n);
                }
                *n = 0;
            }
        }

        T* operator->() {
            return t;
        }
    };


    explicit Spy(T&& t): t_(std::forward<T>(t)) {}

    T& operator *() { return t_; }
    const T& operator *() const { return t_; }

    ExprScopeObserver operator ->() {
        ++counter;
       return ExprScopeObserver{&t_, logger_, funcs_.log, &counter};
    }

    Spy() requires std::default_initializable<T> : t_(T{}) {}

    ~Spy() requires std::destructible<T> {
        if (funcs_.destroy != nullptr) {
            funcs_.destroy(logger_);
        }
    }

    Spy(const Spy& other) requires std::copyable<T>:
            t_(other.t_),
            funcs_(other.funcs_),
            counter(0)
    {
        if (other.logger_ != nullptr) {
            logger_ = other.funcs_.copy(other.logger_);
        }
    }
    Spy(Spy&& other) requires std::movable<T>:
            t_(std::move(other.t_)),
            counter(0)
    {
        std::swap(logger_, other.logger_);
        std::swap(funcs_, other.funcs_);
    }

    Spy& operator=(const Spy& other) requires std::copyable<T> {
        if (this == &other) {
            return *this;
        }
        if (funcs_.destroy != nullptr) {
            funcs_.destroy(logger_);
        }

        t_ = other.t_;
        if (other.logger_ != nullptr) {
            logger_ = other.funcs_.copy(other.logger_);
        } else {
            logger_ = nullptr;
        }
        funcs_ = other.funcs_;
        counter = 0;

        return *this;
    }
    Spy& operator=(Spy&& other) requires std::movable<T> {
        if (this == &other) {
            return *this;
        }

        t_ = std::move(other.t_);
        std::swap(logger_, other.logger_);
        std::swap(funcs_, other.funcs_);
        counter = 0;

        return *this;
    }

    bool operator==(const Spy& other) const requires std::equality_comparable<T> {
        return t_ == other.t_;
    }

    template <std::invocable<unsigned int> Logger>
    requires std::destructible<T> && std::movable<T> && (!std::copyable<T>) &&
        std::destructible<std::remove_cvref_t<Logger>> && std::move_constructible<std::remove_cvref_t<Logger>>
    void setLogger(Logger&& logger) {
        using ClearLogger = std::remove_cvref_t<Logger>;

        logger_ = new ClearLogger(std::forward<Logger>(logger));

        funcs_.log = +[](void* self, unsigned int n){
            std::invoke(*static_cast<ClearLogger*>(self), n);
        };

        funcs_.destroy = +[](void* self){
            delete static_cast<ClearLogger*>(self);
        };
    }

    template <std::invocable<unsigned int> Logger>
    requires std::destructible<T> && std::copyable<T>
        && std::destructible<std::remove_cvref_t<Logger>> && std::copyable<std::remove_cvref_t<Logger>>
    void setLogger(Logger&& logger) {
        using ClearLogger = std::remove_cvref_t<Logger>;

        logger_ = new ClearLogger(std::forward<Logger>(logger));

        funcs_.log = +[](void* self, unsigned int n){
            std::invoke(*static_cast<ClearLogger*>(self), n);
        };

        funcs_.destroy = +[](void* self){
            delete static_cast<ClearLogger*>(self);
        };

        funcs_.copy = +[](void * self)->void*{
            return new ClearLogger(*static_cast<ClearLogger*>(self));
        };
    }

private:
    T t_;

    void* logger_{nullptr};
    LoggerInvokeTable funcs_;

    unsigned int counter = 0;
};

#endif //MIPT_METAPROGRAMMING_SPY_HPP
