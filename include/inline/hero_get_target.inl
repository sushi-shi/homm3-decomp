    // E:\gamedcs\Hero.h:986, dc 0x1fd30. SetHeroContext preserves this
    // header-inline helper in source. Retail folds its packed-point
    // construction into SetHeroContext and move_hero, so the canonical
    // declaration belongs to hero rather than either TU's private view.
    type_point getTarget() const
    {
        return type_point(m_pathTargetX, m_pathTargetY, m_pathTargetZ);
    }
