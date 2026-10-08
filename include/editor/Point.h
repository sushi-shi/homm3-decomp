// Point.h - two-dimensional points and extents (Loki h3maped).
// Templates only; GameMap.cpp owns the TPoint<unsigned int>/<int> and
// TExtent<unsigned int, unsigned int> instantiations. A TExtent is its
// top-left point and its size: right() is left plus width, bottom() top
// plus height, bottomRight() the sum of the two points. TTilePoint and
// TTileExtent are the map's names for the unsigned forms (__PRETTY_FUNCTION__
// spells them "class TTilePoint"/"class TTileExtent"). The header's file
// name and the members' names are not recorded.
#ifndef HOMM3_EDITOR_POINT_H
#define HOMM3_EDITOR_POINT_H

template<class T>
class TPoint {
public:
    TPoint() {}
    TPoint(const T& x, const T& y) : _m_x(x), _m_y(y) {}
    template<class U>
    TPoint(const TPoint<U>& other) : _m_x(other.x()), _m_y(other.y()) {}

    T x() const { return _m_x; }
    void x(const T& newX) { _m_x = newX; }
    T y() const { return _m_y; }
    void y(const T& newY) { _m_y = newY; }

    TPoint& operator+=(const TPoint& other)
    {
        _m_x += other._m_x;
        _m_y += other._m_y;
        return *this;
    }

    TPoint& operator-=(const TPoint& other)
    {
        _m_x -= other._m_x;
        _m_y -= other._m_y;
        return *this;
    }

private:
    T _m_x;
    T _m_y;
};

template<class T1, class T2>
inline const TPoint<T1> operator+(const TPoint<T1>& lhs, const TPoint<T2>& rhs)
{
    return TPoint<T1>(lhs) += rhs;
}

template<class T1, class T2>
inline const TPoint<T1> operator-(const TPoint<T1>& lhs, const TPoint<T2>& rhs)
{
    return TPoint<T1>(lhs) -= rhs;
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

template<class TCoord1, class TDim1, class TCoord2, class TDim2>
inline bool intersect(const TExtent<TCoord1, TDim1>& lhs, const TExtent<TCoord2, TDim2>& rhs)
{
    return lhs.left() < rhs.right() && lhs.top() < rhs.bottom()
        && lhs.right() > rhs.left() && lhs.bottom() > rhs.top();
}

typedef TPoint<unsigned int> TTilePoint;
typedef TExtent<unsigned int, unsigned int> TTileExtent;

#endif  /* HOMM3_EDITOR_POINT_H */
