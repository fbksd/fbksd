/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef POINT_H
#define POINT_H

#include <cstdint>
#include <rpc/msgpack.hpp>


namespace fbksd
{

/**
 * @brief A simple 2d point.
 */
template<class T>
struct TPoint2
{
    TPoint2() = default;

    TPoint2(T x, T y):
        x(x), y(y)
    {}

    T& operator[](int i)
    { return (&x)[i]; }

    T operator[](int i) const
    { return (&x)[i]; }

    bool operator==(const TPoint2& p) const
    { return x == p.x && y == p.y; }

    bool operator!=(const TPoint2& p) const
    { return !(*this == p); }

    bool operator<(const TPoint2& p) const
    { return x < p.x && y < p.y; }

    T x = 0;
    T y = 0;

    MSGPACK_DEFINE_ARRAY(x, y)
};


/**
 * @brief A simple 3d point.
 */
template<class T>
struct TPoint3
{
    TPoint3() = default;

    TPoint3(T x, T y, T z):
        x(x), y(y), z(z)
    {}

    T& operator[](int i)
    { return (&x)[i]; }

    T operator[](int i) const
    { return (&x)[i]; }

    bool operator==(const TPoint3& p) const
    { return x == p.x && y == p.y && z == p.z; }

    bool operator!=(const TPoint3& p) const
    { return !(*this == p); }

    bool operator<(const TPoint3& p) const
    { return x < p.x && y < p.y && z < p.z; }

    T x = 0;
    T y = 0;
    T z = 0;

    MSGPACK_DEFINE_ARRAY(x, y, z)
};

using Point2l = TPoint2<int64_t>;

} // namespace fbksd

#endif // POINT_H
