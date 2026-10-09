// Point.h - two-dimensional points and extents, shared by the game's random
// map generator and the editor (Loki h3maped's TPoint<T> and TExtent<,>).
// Loki instantiates TPoint<int> (the neighbour offsets) and TPoint<unsigned
// int> (map tiles); the game's retained bodies are the same instances:
// TPoint<int>::operator+= at 0x4fa540, TPoint<unsigned int>'s converting
// constructor from TPoint<int> at 0x4fa520, its coordinate constructor at
// 0x5b76b0 and operator< <unsigned int, unsigned int> at 0x5b8ca0. A TExtent
// is its top-left point and its size: right() is left plus width, bottom()
// top plus height, bottomRight() the sum of the two points. TTilePoint and
// TTileExtent are the map's names for the unsigned forms (__PRETTY_FUNCTION__
// spells them "class TTilePoint"/"class TTileExtent"). The header's file
// name and the members' names are not recorded; the game reads the
// coordinates directly.
#ifndef HOMM3_POINT_H
#define HOMM3_POINT_H

#include "va.h"

template<class T>
class TPoint {
public:
    TPoint() {}
    // VA instance: TPoint<unsigned int>::TPoint(const unsigned int&, const unsigned int&)
    VA(0x005B76B0, 0x18) // the terrain painter's retained body; ret 8
    TPoint(const T& x, const T& y) : m_x(x), m_y(y) {}
    // The terrain painter expands this conversion; the line walker's object
    // keeps the retained instance.
    template<class U>
    // VA instance: TPoint<unsigned int>::TPoint(const TPoint<int>&)
    VA(0x004fa520, 0x16)
    MAC_ADDRESS(0x2228a4, 0x14) // anchor-callee 0x4f9f77; thiscall, ret 4; MAC_ABSTRACTION_FROM(tokens1:0e00ed15f0a6,100.0000): aa93e92de's retail object split makes this conversion a header inline body that the terrain painter expands; CodeWarrior emits no out-of-line copy for the retained call.
    TPoint(const TPoint<U>& other) : m_x(other.x()), m_y(other.y()) {}

    T x() const { return m_x; }
    void x(const T& newX) { m_x = newX; }
    T y() const { return m_y; }
    void y(const T& newY) { m_y = newY; }

    // VA instance: TPoint<int>::operator+=
    VA(0x004fa540, 0x21) // anchor-callers 0x4f9f00/0x4fa3c0; thiscall, ret 4
    TPoint& operator+=(const TPoint& other)
    {
        m_x += other.m_x;
        m_y += other.m_y;
        return *this;
    }
    TPoint& operator-=(const TPoint& other)
    {
        m_x -= other.m_x;
        m_y -= other.m_y;
        return *this;
    }

    T m_x;
    T m_y;
};

template<class T1, class T2>
inline const TPoint<T1> operator+(const TPoint<T1>& lhs, const TPoint<T2>& rhs)
{
    TPoint<T1> result = lhs;
    return result += rhs;
}

template<class T1, class T2>
inline const TPoint<T1> operator-(const TPoint<T1>& lhs, const TPoint<T2>& rhs)
{
    TPoint<T1> result = lhs;
    return result -= rhs;
}

// Row-major order (y, then x): the order of TerrainPlacement.cpp's tile sets.
template<class T1, class T2>
// VA instance: bool operator< <unsigned int, unsigned int>(const TPoint<unsigned int>&, const TPoint<unsigned int>&)
VA(0x005b8ca0, 0x20) // anchor-callee 0x5b4e96; fastcall, two point references
bool operator<(const TPoint<T1>& lhs, const TPoint<T2>& rhs)
{
    return lhs.y() < rhs.y() || (lhs.y() == rhs.y() && lhs.x() < rhs.x());
}

template<class TCoord, class TDim>
class TExtent {
public:
    TExtent() {}
    TExtent(const TPoint<TCoord>& topLeft, const TPoint<TDim>& size)
        : _m_topLeft(topLeft), _m_size(size) {}

    TCoord left() const { return _m_topLeft.x(); }
    void left(const TCoord& newLeft) { _m_topLeft.x(newLeft); }
    TCoord top() const { return _m_topLeft.y(); }
    void top(const TCoord& newTop) { _m_topLeft.y(newTop); }
    TCoord right() const { return _m_topLeft.x() + _m_size.x(); }
    TCoord bottom() const { return _m_topLeft.y() + _m_size.y(); }
    TDim width() const { return _m_size.x(); }
    TDim height() const { return _m_size.y(); }
    const TPoint<TCoord> bottomRight() const { return _m_topLeft + _m_size; }

    TExtent& operator&=(const TExtent& other);
    TExtent& operator|=(const TExtent& other);

private:
    TPoint<TCoord> _m_topLeft;
    TPoint<TDim> _m_size;
};

// Out of the class (Loki: not inline, so g++ keeps `other` in its stack slot).
template<class TCoord, class TDim>
TExtent<TCoord, TDim>& TExtent<TCoord, TDim>::operator&=(const TExtent& other)
{
    TPoint<TCoord> br = bottomRight();
    left(left() > other.left() ? left() : other.left());
    top(top() > other.top() ? top() : other.top());
    br.x(br.x() < other.right() ? br.x() : other.right());
    br.y(br.y() < other.bottom() ? br.y() : other.bottom());
    _m_size = br - _m_topLeft;
    return *this;
}

template<class TCoord, class TDim>
TExtent<TCoord, TDim>& TExtent<TCoord, TDim>::operator|=(const TExtent& other)
{
    TPoint<TCoord> br = bottomRight();
    left(left() < other.left() ? left() : other.left());
    top(top() < other.top() ? top() : other.top());
    br.x(br.x() > other.right() ? br.x() : other.right());
    br.y(br.y() > other.bottom() ? br.y() : other.bottom());
    _m_size = br - _m_topLeft;
    return *this;
}

template<class TCoord1, class TDim1, class TCoord2, class TDim2>
inline const TExtent<TCoord1, TDim1> operator&(const TExtent<TCoord1, TDim1>& lhs, const TExtent<TCoord2, TDim2>& rhs)
{
    TExtent<TCoord1, TDim1> result = lhs;
    return result &= rhs;
}

template<class TCoord1, class TDim1, class TCoord2, class TDim2>
inline const TExtent<TCoord1, TDim1> operator|(const TExtent<TCoord1, TDim1>& lhs, const TExtent<TCoord2, TDim2>& rhs)
{
    TExtent<TCoord1, TDim1> result = lhs;
    return result |= rhs;
}

template<class TCoord1, class TDim1, class TCoord2, class TDim2>
inline bool intersect(const TExtent<TCoord1, TDim1>& lhs, const TExtent<TCoord2, TDim2>& rhs)
{
    return lhs.left() < rhs.right() && lhs.top() < rhs.bottom()
        && lhs.right() > rhs.left() && lhs.bottom() > rhs.top();
}

typedef TPoint<unsigned int> TTilePoint;
typedef TExtent<unsigned int, unsigned int> TTileExtent;

#endif  /* HOMM3_POINT_H */
