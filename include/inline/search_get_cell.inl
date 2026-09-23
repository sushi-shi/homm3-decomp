    VA(0x0042ecc0, 0x62)  // hd-crossbuild + exact body/callers x2, dc 0x20064
    pathCell* getCell(type_point point, bool flying) const
    {
        if (!m_cellData)
            return m_cellData;
        return &m_cellData[((point.m_z * 2 + flying) * g_mapHeight + point.m_y)
                         * g_mapWidth + point.m_x];
    }
