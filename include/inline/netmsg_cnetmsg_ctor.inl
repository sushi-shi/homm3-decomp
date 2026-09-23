    VA(0x004f2930, 0x23)  // anchor-callee + exact body, retail-only slot
    CNetMsg(eRS_Messages subType, unsigned long size)
    {
        this->m_subType = subType;
        m_from = -1;
        this->m_size = size;
        m_dpidFrom = 0;
        m_uncompressedSize = 0;
    }
