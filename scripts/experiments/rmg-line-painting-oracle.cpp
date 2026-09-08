// Host-only behavioral oracle. Never included in a matching VC6 TU.
struct Painter : TRmgLinePainterInterface {
    int kinds[64], blocked[64];
    std::vector<int> events;
    Painter(unsigned w, unsigned h) : TRmgLinePainterInterface(TRmgGridPoint(w, h)) {
        for (unsigned i = 0; i != 64; ++i) {
            kinds[i] = i % 4; blocked[i] = 0;
        }
    }
    int index(const TRmgGridPoint& p) { return p.m_y * 8 + p.m_x; }
    void record(int action, const TRmgGridPoint& p, int value) {
        events.push_back(action); events.push_back(p.m_x);
        events.push_back(p.m_y); events.push_back(value);
    }
    virtual TRmgLinePatternTable* getPattern(int) { return 0; }
    virtual void setTile(const TRmgGridPoint& p, const rmgTerrainTile& tile) {
        record(2, p, tile.m_terrain);
        events.push_back(tile.m_frame); events.push_back(tile.m_flipX); events.push_back(tile.m_flipY);
        kinds[index(p)] = tile.m_terrain;
    }
    virtual void setOverlay(const TRmgGridPoint& p, int value) {
        record(3, p, value); kinds[index(p)] = value;
    }
    virtual int canPaint(const TRmgGridPoint& p) {
        record(4, p, blocked[index(p)]); return blocked[index(p)];
    }
    virtual void getTile(const TRmgGridPoint&, rmgTerrainTile&) {}
    virtual int getLand(const TRmgGridPoint& p) {
        record(1, p, kinds[index(p)]); return kinds[index(p)];
    }
};

void refreshRmgLinePoint(TRmgLinePainterInterface* painter, const TRmgGridPoint& point) {
    static_cast<Painter*>(painter)->record(5, point, 0);
}

void visitBorder(Painter& painter, unsigned x, unsigned y) {
    TRmgGridPoint p(x, y);
    if (painter.getLand(p)) refreshRmgLinePoint(&painter, p);
}

void referenceClear(Painter& painter, unsigned x, unsigned y, unsigned w, unsigned h) {
    for (unsigned row = y; row < y + h; ++row)
        for (unsigned col = x; col < x + w; ++col) {
            TRmgGridPoint p(col, row);
            if (painter.getLand(p)) painter.setTile(p, rmgTerrainTile(0, 0));
        }
    unsigned firstY = y == 0 ? 0 : y - 1;
    if (x != 0) {
        unsigned lastY = y + h;
        if (lastY < painter.m_size.m_y) ++lastY;
        for (unsigned row = firstY; row < lastY; ++row) visitBorder(painter, x - 1, row);
    }
    if (x + w < painter.m_size.m_x) {
        unsigned lastY = y + h;
        // Retail deliberately differs from the left-border bound here.
        if (lastY < painter.m_size.m_y - 1) ++lastY;
        for (unsigned row = firstY; row < lastY; ++row) visitBorder(painter, x + w, row);
    }
    if (y != 0)
        for (unsigned col = x; col < x + w; ++col) visitBorder(painter, col, y - 1);
    if (y + h < painter.m_size.m_y)
        for (unsigned col = x; col < x + w; ++col) visitBorder(painter, col, y + h);
}

void referencePoint(Painter& painter, const TRmgGridPoint& point, int type) {
    int old = painter.getLand(point);
    if (old == type || static_cast<unsigned char>(painter.canPaint(point))) return;
    if (old) referenceClear(painter, point.m_x, point.m_y, 1, 1);
    painter.setOverlay(point, type);
    refreshRmgLinePoint(&painter, point);
    bool matches[8];
    for (unsigned d = 0; d != 8; ++d) {
        int x = point.m_x + g_tileDirections[d].m_x;
        int y = point.m_y + g_tileDirections[d].m_y;
        matches[d] = x >= 0 && y >= 0 && unsigned(x) < painter.m_size.m_x
            && unsigned(y) < painter.m_size.m_y
            && painter.getLand(TRmgGridPoint(x, y)) == type;
    }
    for (unsigned d = 0; d != 8; ++d)
        if (matches[d]) refreshRmgLinePoint(&painter, point + g_tileDirections[d]);
}

bool same(const Painter& a, const Painter& b) {
    if (a.events != b.events) return false;
    for (unsigned i = 0; i != 64; ++i) if (a.kinds[i] != b.kinds[i]) return false;
    return true;
}

int check() {
    for (unsigned width = 1; width <= 4; ++width)
    for (unsigned height = 1; height <= 4; ++height)
    for (unsigned x = 0; x <= width; ++x)
    for (unsigned y = 0; y <= height; ++y)
    for (unsigned w = 0; w <= width - x; ++w)
    for (unsigned h = 0; h <= height - y; ++h) {
        Painter a(width, height), b(width, height);
        TRmgGridRectangle rectangle(TRmgGridPoint(x, y), w, h);
        clearRmgLineRectangle(&a, rectangle);
        referenceClear(b, x, y, w, h);
        if (!same(a, b)) return 1;
        if (rectangle.m_origin.m_x != x || rectangle.m_origin.m_y != y
            || rectangle.m_size.m_x != w || rectangle.m_size.m_y != h) return 2;
    }
    // The retained neighbour-mask helper assumes dimensions > 1: its north/
    // south and west/east exclusions are else-if chains. Test painting in that
    // domain, while the rectangle checks above include one-cell dimensions.
    for (unsigned width = 2; width <= 5; ++width)
    for (unsigned height = 2; height <= 5; ++height)
    for (unsigned x = 0; x < width; ++x)
    for (unsigned y = 0; y < height; ++y)
    for (int old = 0; old <= 3; ++old)
    for (int blockCase = 0; blockCase != 4; ++blockCase) {
        Painter a(width, height), b(width, height);
        TRmgGridPoint point(x, y);
        int i = a.index(point), block = (blockCase / 2) * 256 + blockCase % 2;
        a.kinds[i] = b.kinds[i] = old; a.blocked[i] = b.blocked[i] = block;
        // The actual retained constructor invokes the actual paintPoint body.
        TRmgLineWalker walker(&a, 3, point);
        referencePoint(b, point, 3);
        if (!same(a, b)) return 3;
        if (point.m_x != x || point.m_y != y) return 4;
    }
    return 0;
}
