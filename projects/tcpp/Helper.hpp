#pragma once
#include <cassert>
#include <utility>
#include <iostream>
#include <format>
#include <cstdint>
#include <arpa/inet.h> // Pour htonl et ntohl
#include <boost/circular_buffer.hpp>
#include <format>

#define TODO_RET(ret) template <typename... Args> \
ret

#define TODO_ARGS Args &&...args

#define FOCUS() _Pragma("message \"TODO! FOCUS HERE!\"")

#define unimplemented()                \
    do                                 \
    {                                  \
        assert(0 && "Unimplemented!"); \
    } while (0)

#ifdef DEBUG_BAR
#define PRINT_VAR(var)                                                  \
    do                                                                  \
    {                                                                   \
        std::cout << std::format("[" #var "] -> {}", var) << std::endl; \
    } while (0)
#else

#define PRINT_VAR(var)

#endif

template <typename T>
void print_buffer(const std::vector<T> &v, int n = 0)
{
    if (n == 0)
        n = v.size();

    for (int i = 0; i < n && i < v.size(); ++i)
    {
        std::cout << std::format("{:#x} ", v[i]);
    }
    std::cout << std::endl;
}

// htons()

// host to network short

// htonl()

// host to network long

// ntohs()

// network to host short

// ntohl()

// network to host long

// 32 bits
auto add(__be32 a, uint32_t b)
{
    uint32_t ha = ntohl(a);
    ha += b;
    return htonl(ha);
}

auto sadd(__be32 a, __be32 b)
{
    uint32_t ha = ntohl(a);
    uint32_t hb = ntohl(b);
    ha += hb;
    return htonl(ha);
}

auto add_tohs(__be32 a, uint32_t b)
{
    uint32_t ha = ntohl(a);
    ha += b;
    return ha;
}

auto sub(__be32 a, uint32_t b)
{
    uint32_t ha = ntohl(a);
    ha -= b;
    return htonl(ha);
}

auto ssub(__be32 a, __be32 b)
{
    uint32_t ha = ntohl(a);
    uint32_t hb = ntohl(b);
    ha -= hb;
    return htonl(ha);
}

auto sub_tohs(__be32 a, uint32_t b)
{
    uint32_t ha = ntohl(a);
    return ha - b;
}

auto ssub_tohs(__be32 a, __be32 b)
{
    uint32_t ha = ntohl(a);
    uint32_t hb = ntohl(b);
    return ha - hb;
}

// 16 bits

auto add_16(__be16 a, uint16_t b)
{
    uint16_t ha = ntohs(a);
    return htons(ha) + b;
}

auto sadd_16(__be16 a, __be16 b)
{
    uint16_t ha = ntohs(a);
    uint16_t hb = ntohs(b);
    ha += hb;
    return htonl(ha);
}

auto add_16_tohs(__be16 a, uint16_t b)
{
    uint16_t ha = ntohs(a);
    return ha + b;
}

auto sub_16(__be16 a, uint16_t b)
{
    uint16_t ha = ntohs(a);
    ha -= b;
    return htons(ha);
}

auto ssub_16(__be16 a, __be16 b)
{
    uint16_t ha = ntohs(a);
    uint16_t hb = ntohs(b);
    ha -= hb;
    return htons(ha);
}

auto sub_16_tohs(__be16 a, uint16_t b)
{
    uint16_t ha = ntohs(a);
    ha -= b;
    return ha;
}

auto ssub_16_tohs(__be16 a, __be16 b)
{
    uint16_t ha = ntohs(a);
    uint16_t hb = ntohs(b);
    ha -= b;
    return hb;
}

template <class EnumType, class... Ts>
constexpr bool match(EnumType e, Ts... inputs)
{
    static_assert(std::is_enum_v<EnumType>, "Should be an enum!");
    return ((e == inputs) || ...);
}

template <typename T>
T replace(T &target, T &&new_value)
{
    T old_value = std::move(target);
    target = std::move(new_value);
    return old_value;
}

template <typename T>
T unwrap_or(const std::optional<T> &opt, const T &v)
{
    if (opt.has_value())
    {
        return *opt;
    }
    else
    {
        return v;
    }
}

// Todo: Return a slice instead of a vector
template <typename T>
std::pair<std::vector<T>, std::vector<T>> as_slices(const boost::circular_buffer<T> &buffer)
{

    const T *data = buffer.array_one().first;    // Premier segment contigu
    size_t size_one = buffer.array_one().second; // Taille du premier segment

    const T *data2 = buffer.array_two().first;   // Deuxième segment contigu (si nécessaire)
    size_t size_two = buffer.array_two().second; // Taille du deuxième segment

    std::vector<T> slice1(data, data + size_one);
    std::vector<T> slice2(data2, data2 + size_two);

    return {slice1, slice2};
}