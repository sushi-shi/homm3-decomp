// rmg_voronoi.cpp - the random-map generator's Voronoi subdivision.
//
// Retail keeps the quad-edge subdivision and its point/vector arithmetic
// together at 0x5fceb0..0x5fdf45, after viewwrld's code: one object of its
// own in the original link order. The original file name is unknown.
#include "va.h"

#include <algorithm>
#include <math.h>

#include "exceptions.h"
#include "rmg.h"

VA(0x005fceb0, 0x39)
MAC_ADDRESS(0x25c018, 0x64)
s32 TRmgVector::length() const
{
    return static_cast<s32>(sqrt(static_cast<double>(m_x * m_x + m_y * m_y)));
}

// Both constructors start a half-edge as its own ring with no vertex. The
// ordinary helper is expanded in both; its /Ob2 cost of 60 is spent twice
// inside createEdge's paired-constructor expansion (126 + 60 + 81 + 60),
// which is what leaves the second insert's count-insert body a budget of
// 34 and keeps its first size() call as retail does. The same stores
// written inline in each constructor (costs 157 and 87..135) never spend
// enough: createEdge stayed at 90.96% through 14 constructor spellings.
MAC_ADDRESS(0x25c164, 0x20)
void TRmgHalfEdge::initialize()
{
    m_next = this;
    m_previous = this;
    m_vertexComputed = 0;
    m_vertex.m_x = -1;
    m_vertex.m_y = -1;
}

// The retained paired constructor expands this ordinary twin constructor
// into the successful allocation arm. The same site/zone fields feed the
// Voronoi vertex calculations. Body assignments (cost 81) keep createEdge
// exact; the initializer-list form costs 70 and loses it (90.96%).
TRmgHalfEdge::TRmgHalfEdge(
    TPoint sitePosition, TRmgZone* zone, TRmgHalfEdge* twin)
{
    m_sitePosition = sitePosition;
    m_zone = zone;
    m_twin = twin;
    initialize();
}

VA(0x005fcef0, 0x6c)
MAC_ADDRESS(0x25c07c, 0x98) // anchor-callee 0x5fd078; Complete-only, ret 0x18
TRmgHalfEdge::TRmgHalfEdge(
    TPoint sitePosition, TRmgZone* zone, TPoint twinSitePosition, TRmgZone* twinZone)
    : m_zone(zone)
{
    m_sitePosition = sitePosition;
    m_twin = new TRmgHalfEdge(twinSitePosition, twinZone, this);
    initialize();
}

// Replacing the second pointer exchange with std::swap leaves this body exact
// but lowers the Voronoi constructor from 100% to 83.35% through expansion.
VA(0x005fcf60, 0x31)
MAC_ADDRESS(0x25c184, 0x34) // anchor-callee 0x5fd308; thiscall, ret 4; Complete-only
void TRmgHalfEdge::splice(TRmgHalfEdge* other)
{
    std::swap(m_next->m_previous, other->m_next->m_previous);
    TRmgHalfEdge* next = m_next;
    m_next = other->m_next;
    other->m_next = next;
}

VA(0x005fcfa0, 0x61)
MAC_ADDRESS(0x25c1b8, 0x4c) // anchor-callee addSite 0x5fd790; thiscall, ret 0; Complete-only
void TRmgHalfEdge::detach()
{
    TRmgHalfEdge* previous = m_previous;
    TRmgHalfEdge* twinPrevious = m_twin->m_previous;
    splice(previous);
    m_twin->splice(twinPrevious);
}

// Starts as a large square split by a diagonal. North is up (y down); 1-4 are
// firstEdge..fourthEdge and 5 the connectEdges diagonal from first to third.
//   first --1--> second
//     ^  \         |
//     4    5       2
//     |      \     v
//   fourth <-3-- third
VA(0x005fd010, 0x316)
MAC_ADDRESS(0x25c4e4, 0x14c) // anchor-caller 0x53e050 and five createEdge expansions/calls
TRmgVoronoi::TRmgVoronoi()
{
    TPoint first(-200, -200);
    TPoint second(400, -200);
    TPoint third(400, 400);
    TPoint fourth(-200, 400);
    TRmgHalfEdge* firstEdge = createEdge(first, 0, second, 0);
    TRmgHalfEdge* secondEdge = createEdge(second, 0, third, 0);
    TRmgHalfEdge* thirdEdge = createEdge(third, 0, fourth, 0);
    TRmgHalfEdge* fourthEdge = createEdge(fourth, 0, first, 0);
    firstEdge->getTwin()->splice(secondEdge);
    secondEdge->getTwin()->splice(thirdEdge);
    thirdEdge->getTwin()->splice(fourthEdge);
    fourthEdge->getTwin()->splice(firstEdge);
    connectEdges(fourthEdge, thirdEdge);
    m_startingEdge = firstEdge;
}

// The subdivision owns every allocated half-edge and its pointer vector.
// Its retained destructor proves the +0x04 vector and trivial edge cleanup.
VA(0x005fd330, 0x58)
MAC_ADDRESS(0x25c6b4, 0xa4) // anchor-callee 0x53e685; thiscall, ret 0
TRmgVoronoi::~TRmgVoronoi()
{
    for (s32 edge = 0; edge < m_edges.size(); ++edge)
        delete m_edges[edge];
}

VA(0x005fd390, 0x21c)
MAC_ADDRESS(0x25c758, 0x170) // anchor-callers 0x5fd010/0x5fd790; Complete-only, ret 0x18
TRmgHalfEdge* TRmgVoronoi::createEdge(TPoint first, TRmgZone* firstZone,
    TPoint second, TRmgZone* secondZone)
{
    TRmgHalfEdge* edge = new TRmgHalfEdge(first, firstZone, second, secondZone);
    m_edges.push_back(edge);
    m_edges.push_back(edge->getTwin());
    return edge;
}

// Quad-edge Connect(a, b): a new edge from first's destination to second's
// origin, spliced into first's left face and second's origin ring. Ordinary
// and shared; retail expands it in the constructor (0x5fd2c3) and addSite
// (0x5fd72a) while calling createEdge and the fan splices inside it. Its
// /Ob2 cost of 98 sits inside the constructor's 63..145 bracket; field
// reads (93), a named twin (103), chained accessors (113) or endpoint
// locals (122) keep the same bytes here, and 137 or more loses the
// constructor's first diagonal splice.
MAC_ADDRESS(0x25caec, 0x94)
TRmgHalfEdge* TRmgVoronoi::connectEdges(TRmgHalfEdge* first,
    TRmgHalfEdge* second)
{
    TRmgHalfEdge* edge = createEdge(first->getOppositeSitePosition(),
        first->getOppositeZone(), second->getSitePosition(), second->getZone());
    edge->splice(first->getLeftNext());
    edge->getTwin()->splice(second);
    return edge;
}

VA(0x005fd5b0, 0xff)
MAC_ADDRESS(0x25c8c8, 0x100) // anchor-caller 0x5fd790; Complete-only, thiscall ret 4
void TRmgVoronoi::removeEdge(TRmgHalfEdge* edge)
{
    edge->detach();
    u32 index = 0;
    while (index < m_edges.size() && m_edges[index] != edge)
        ++index;
    m_edges.erase(m_edges.begin() + index);
    TRmgHalfEdge* twin = edge->getTwin();
    index = 0;
    while (index < m_edges.size() && m_edges[index] != twin)
        ++index;
    m_edges.erase(m_edges.begin() + index);
    delete edge;
    delete twin;
}

// Provisional shared edge-side predicate, used by locate and legalization.
// docs/reference/rmg-voronoi-provenance.md records the adaptation evidence and its
// limits; this resemblance does not establish an original name/declaration.
// Graphics Gems IV delaunay/quadedge.C's RightOf(x, e) is ccw(x, Dest, Org)
// over TriArea; Complete uses integer by-value TPoint and the canonical
// orientation below in the same cyclic order. The ccw layer is ordinary:
// addSite expands RightOf with a nested budget of 54, expands ccw (cost 31)
// and refuses the orientation call as retail does at 0x5fdfa7; locate's
// budgets expand all three. Retail locate's first expanded orientation has
// no spilled endpoint; the named twin restores all 215 bytes. Flattening
// the call boundary returns 91.0460%.
MAC_ADDRESS(0x25c304, 0x68)
static s32 isRmgCounterClockwise(TPoint first, TPoint second, TPoint third)
{
    return getRmgPointOrientation(first, second, third) > 0;
}

static s32 isRmgPointRightOfEdge(TPoint point, TRmgHalfEdge* edge)
{
    TRmgHalfEdge* twin = edge->m_twin;
    return isRmgCounterClockwise(edge->m_sitePosition, point, twin->m_sitePosition);
}

VA(0x005fd6b0, 0xd7)
MAC_ADDRESS(0x25c9c8, 0x124) // anchor-callers 0x53dad0/0x53e050/0x5fd790; ret 8
TRmgHalfEdge* TRmgVoronoi::locate(TPoint point)
{
    TRmgHalfEdge* edge = m_startingEdge;
    for (;;) {
        {
            TPoint origin = edge->m_sitePosition;
            if (point == origin)
                break;
        }
        {
            TPoint destination = edge->m_twin->m_sitePosition;
            if (point == destination) {
                edge = edge->m_twin;
                break;
            }
        }
        if (isRmgPointRightOfEdge(point, edge)) {
            edge = edge->m_twin;
        } else {
            TRmgHalfEdge* next = edge->m_next;
            if (!isRmgPointRightOfEdge(point, next)) {
                edge = next;
                continue;
            }
            TRmgHalfEdge* previous = edge->m_twin->m_previous->m_twin;
            if (isRmgPointRightOfEdge(point, previous))
                break;
            edge = previous;
        }
    }
    return edge;
}

// Provisional edge flip: retail saves both predecessors before detach,
// transfers their opposite sites/zones, and splices into the new rings.
// Using the opposite-site/zone and left-face accessors throughout this flip,
// detach and addSite's closing test lowers addSite from 100% to 93.74%; the
// other support functions remain exact. Preserve the matched helper boundaries.
MAC_ADDRESS(0x25c204, 0xb0)
static void flipRmgEdge(TRmgHalfEdge* edge)
{
    TRmgHalfEdge* previous = edge->m_previous;
    TRmgHalfEdge* twinPrevious = edge->m_twin->m_previous;
    edge->detach();
    edge->m_zone = previous->m_twin->m_zone;
    edge->m_sitePosition = previous->m_twin->m_sitePosition;
    edge->m_twin->m_zone = twinPrevious->m_twin->m_zone;
    edge->m_twin->m_sitePosition = twinPrevious->m_twin->m_sitePosition;
    edge->splice(previous->m_twin->m_previous);
    edge->m_twin->splice(twinPrevious->m_twin->m_previous);
}

// Provisional segment predicate, Graphics Gems IV's OnEdge: retail snapshots
// the opposite endpoint, calls the three squared distances, rejects a site
// beyond either endpoint, then evaluates the implicit line a*x + b*y + c
// (a = dy, b = -dx, c = -(a*org.x + b*org.y)) and materializes the zero
// test as a byte. Inside addSite this expansion gets a nested budget of 33
// (908 - 174 over 22 remaining sites), which refuses all three distance
// calls like retail; a TRmgLine constructor/evaluate pair is refused at
// that budget too (72.5%-77.6%), so the equation stays inline. An
// orientation call here scores 89.4762% against the accessor form's
// 91.1250%. The line origin is bound by reference to the site field, the
// way the flip helper reads it: retail loads org.x once into ecx and spills
// dx/dy to [ebp-8]/[ebp-0xc] for the four products, which only this binding
// reproduces; a by-value TPoint copy through the accessor re-reads the
// field and keeps dx in a register (addSite 90.3482% with the rest exact).
static b8 isRmgPointOnSegment(TPoint point, TRmgHalfEdge* edge)
{
    TPoint opposite = edge->getOppositeSitePosition();
    s32 firstDistance = getRmgSquaredDistance(point, edge->getSitePosition());
    s32 secondDistance = getRmgSquaredDistance(point, opposite);
    s32 edgeDistance = getRmgSquaredDistance(edge->getSitePosition(), opposite);
    if (firstDistance > edgeDistance || secondDistance > edgeDistance)
        return 0;
    const TPoint& origin = edge->m_sitePosition;
    s32 dx = opposite.m_x - origin.m_x;
    s32 dy = opposite.m_y - origin.m_y;
    s32 c = -(dy * origin.m_x - dx * origin.m_y);
    return dy * point.m_x - dx * point.m_y + c == 0;
}

// Provisional geometric predicate: retail snapshots three points before
// four orientation calls and a signed 64-bit circumcircle determinant.
// Sixty determinant-expression forms tested named versus embedded area
// calls, equivalent sum groupings, x/y square order, which product operand
// widens, and result lifetime. Twenty distinct objects span 33.2202% to the
// unchanged 45.4167% caller score; none recovers the missing orientation
// calls. Preserve the actual by-value point and signed-product boundaries.
MAC_ADDRESS(0x25cb80, 0x1c4)
static b8 isRmgPointInsideCircle(TPoint first, TPoint second,
    TPoint third, TPoint point)
{
    s32 firstArea = getRmgPointOrientation(second, third, point);
    s32 secondArea = getRmgPointOrientation(first, third, point);
    s32 thirdArea = getRmgPointOrientation(first, second, point);
    s32 pointArea = getRmgPointOrientation(first, second, third);
    s64 determinant = static_cast<s64>(third.m_x * third.m_x + third.m_y * third.m_y) * thirdArea
        - static_cast<s64>(second.m_x * second.m_x + second.m_y * second.m_y) * secondArea
        + static_cast<s64>(first.m_x * first.m_x + first.m_y * first.m_y) * firstArea
        - static_cast<s64>(point.m_x * point.m_x + point.m_y * point.m_y) * pointArea;
    return determinant > 0;
}

VA(0x005fd790, 0x348)
MAC_ADDRESS(0x25cd44, 0x218) // anchor-caller 0x53e050; Complete-only, thiscall ret 0xc
void TRmgVoronoi::addSite(TPoint point, TRmgZone* zone)
{
    TRmgHalfEdge* edge = locate(point);
    {
        TPoint origin = edge->getSitePosition();
        if (point == origin)
            return;
    }
    {
        TPoint destination = edge->getOppositeSitePosition();
        if (point == destination)
            return;
    }
    if (isRmgPointOnSegment(point, edge)) {
        edge = edge->getPrevious();
        removeEdge(edge->getNext());
    }
    TRmgHalfEdge* base = createEdge(edge->getSitePosition(), edge->getZone(), point, zone);
    base->splice(edge);
    m_startingEdge = base;
    do {
        base = connectEdges(edge, base->getTwin());
        edge = base->getPrevious();
    } while (edge->getTwin()->getPrevious() != m_startingEdge);

    for (;;) {
        TRmgHalfEdge* previous = edge->getPrevious();
        if (isRmgPointRightOfEdge(previous->getTwin()->getSitePosition(), edge)) {
            if (isRmgPointInsideCircle(edge->getSitePosition(),
                    previous->getTwin()->getSitePosition(), edge->getOppositeSitePosition(), point)) {
                flipRmgEdge(edge);
                edge = edge->getPrevious();
                continue;
            }
        }
        if (edge->getNext() == m_startingEdge)
            return;
        edge = edge->getNext()->getNext()->getTwin();
    }
}

VA(0x005fdae0, 0x2b)
MAC_ADDRESS(0x25c2b4, 0x50) // anchor-callee 0x5fd937/0x5fd97e; Complete-only
s32 getRmgPointOrientation(TPoint first, TPoint second, TPoint third)
{
    return (second.m_x - first.m_x) * (third.m_y - first.m_y)
        - (second.m_y - first.m_y) * (third.m_x - first.m_x);
}

// The subdivision constructor retains seven single-edge insertions at
// 0x5fd091/0x5fd0f6/0x5fd10e/0x5fd15a/0x5fd172/0x5fd1bb/0x5fd1d3.
// Four-byte elements, ret 8 and the owning m_edges vector identify this
// ordinary Dinkumware specialization independently of its ICF helper names.
VA_COMPGEN(0x005fdd60, 0x1b1, VECTOR_INSERT_SINGLE, TRmgHalfEdge)

// Retail's only callers of the four-byte fill at 0x5fdf20 are
// TRmgVoronoi::createEdge (0x5fd4b9, 0x5fd53d) and the boundary-vertex insert
// above (0x5fde8c): it is this unit's vector<TRmgHalfEdge*> _Ufill.
// Widget vectors reach the folded copy at 0x48d940 instead.
VA_COMPGEN(0x005fdf20, 0x26, VECTOR_UFILL, TRmgHalfEdge)

// Each uncomputed interior half-edge identifies an incident triangle. Retail
// computes its integer circumcenter through canonical point/vector operations,
// then shares it with the other two incident half-edges. These Complete-only
// helper names/interfaces remain provisional: no DC counterpart was found.
// The first subtraction expands inside an ordinary three-point helper; two
// later subtractions and five arithmetic operations remain calls. Flattening
// that boundary or exposing the edge-navigation body inside the helper does
// not reproduce the retained call sequence. No inline-depth pin is needed.
// The tiny by-value vector dot lives in the header, matching the analogous
// explicitly-inline Graphics Gems helper. Both calls expand; ordinary and
// in-class definitions produce the same buildVertices bytes, while the member
// ownership is what restores retail's arithmetic allocation. Preserve its
// y-before-x products and the left-operand receivers below.

// Circumcenter of the triangle with these three sites: the perpendicular
// bisector of the origin-to-second side, scaled by the projected sides.
// Materialize the opposite site at the caller. This by-value parameter order
// reproduces retail's 0x78 frame and its origin/third-site stack slots.
static TPoint computeRmgCircumcenter(TPoint third, TPoint origin, TPoint second)
{
    TRmgVector axis = second - origin;
    TRmgVector perpendicular(-axis.m_y, axis.m_x);
    TRmgVector secondSide = third - second;
    TRmgVector thirdSide = origin - third;
    return origin + (axis + perpendicular * secondSide.dot(thirdSide)
        / perpendicular.dot(thirdSide)) / 2;
}

// Residual 98.2254%: retail and candidate have the same 0x78 frame, eight CFG
// blocks, seven named calls/relocations, function length and return. Bytes are
// exact through +0x5a and from +0x6e through the complete three-edge setter
// tail. In the remaining twelve rows, candidate loads second.y into EDX before
// the third site; retail uses EDX for third.x/y first, reloads origin.x, then
// loads second.y. The by-value member dot plus the setter's named snapshot
// raised the prior 97.5070% peak and recovered the entire tail.
//
// New-parent controls exhaust caller site snapshots (8 states), canonical
// point-subtraction construction (6), axis binding (5), three-parameter value/
// reference ownership (8), setter ownership/construction (4), and ordinary
// versus in-class dot placement/ownership (4). All valid subtraction bodies
// retain the independently exact 0x20 operator at 0x5fdd40. None improves the
// short input-load schedule. Earlier free-dot families cover argument order,
// parent/result lifetimes and canonical arithmetic bodies. Native triangle/
// ring checks preserve integer division, all three position/flag writes,
// inactive edges and graph links.
// All six parameter orders under the current member-dot model also retain
// this peak; the helper's argument order does not explain the remaining loads.
VA(0x005FDB40, 0x16E)
MAC_ADDRESS(0x25d144, 0x12c) // anchor-caller 0x53e050; Complete-only, thiscall ret 0
void TRmgVoronoi::buildVertices()
{
    for (unsigned int index = 0; index < m_edges.size(); ++index) {
        TRmgHalfEdge* edge = m_edges[index];
        if (edge->getZone() && !edge->isPositionComputed()) {
            TPoint second = edge->getOppositeSitePosition();
            TPoint position = computeRmgCircumcenter(edge->getNext()->getOppositeSitePosition(),
                edge->getSitePosition(), second);
            edge->setPosition(position);
            edge = edge->getNext()->getTwin();
            edge->setPosition(position);
            edge = edge->getNext()->getTwin();
            edge->setPosition(position);
        }
    }
}
