#pragma once
#include <mutex>
#include <utility>

template <class T>
class Safe
{
public:
    template <typename... Args>
    Safe(Args &&...args) : _instance{std::forward<Args>(args)...} {}

    auto lock()
    {
        std::unique_lock<std::mutex> guard{_mu};
        return std::pair<T &, std::unique_lock<std::mutex>>{_instance, std::move(guard)};
    }

    auto lock() const
    {
        std::unique_lock<std::mutex> guard(_mu);
        return std::pair<const T &, std::unique_lock<std::mutex>>{_instance, std::move(guard)};
    }

private:
    mutable std::mutex _mu;
    T _instance;
};