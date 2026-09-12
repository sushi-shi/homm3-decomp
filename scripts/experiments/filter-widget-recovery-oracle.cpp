// Native behavior fixture, not a retail layout model.
struct font { enum { PRIMARY = 3, CENTER_JUSTIFIED = 1, VERT_CENTER_JUSTIFIED = 2, LEFT_JUSTIFIED = 4 }; };
enum { MAP_DIMENSION_MEDIUM = 72, SCENARIO_FILTER_CATEGORY_ANY = -7 };
struct Texts {
    const char* getText(int index) {
        static const char* names[] = {"753", "754", "755", "756", "757", "758", "759"};
        return names[index - 753];
    }
} g_texts;
Texts* g_generalText = &g_texts;
struct widget {
    int x, y, w, h, id, kind;
    std::string resource;
    widget(int a, int b, int c, int d, int e, const char* name, int k)
        : x(a), y(b), w(c), h(d), id(e), kind(k), resource(name) {}
    virtual ~widget() {}
};
struct button : widget {
    long m_highlightedFrame, m_disabledFrame;
    int values[5];
    button(int x, int y, int w, int h, int id, const char* name, int a, int b, int c, int d, int e)
        : widget(x,y,w,h,id,name,1), m_highlightedFrame(0), m_disabledFrame(0) {
        values[0]=a; values[1]=b; values[2]=c; values[3]=d; values[4]=e;
    }
    // @DISABLED_SETTER@
};
struct textWidget : widget {
    std::string face;
    int color, alignment, style, flags;
    textWidget(int x,int y,int w,int h,const char* text,const char* font,int c,int id,int a,int s,int f)
        : widget(x,y,w,h,id,text,2), face(font), color(c), alignment(a), style(s), flags(f) {}
};
struct TSingleSelectionWindow {
    int m_randomMapOptions[8];
    std::vector<widget*> m_widgets;
    button* m_filterCountAButtons[9];
    button* m_filterCountBButtons[9];
    button* m_filterCountCButtons[9];
    button* m_filterCountDButtons[8];
    button* m_filterWaterButtons[4];
    button* m_filterStrengthButtons[4];
    void createFilterWidgets();
    ~TSingleSelectionWindow() { for (unsigned i=0;i<m_widgets.size();++i) delete m_widgets[i]; }
};
// @BODY@
bool check() {
    const int options[] = {72,2,-1,-1,0,-1,SCENARIO_FILTER_CATEGORY_ANY,-1};
    const int firstIds[] = {0x11f,0x129,0x133,0x13d,0x146,0x14b};
    const int counts[] = {9,9,9,8,4,4};
    const int rowY[] = {153,219,285,351,419,485};
    const int textIds[] = {0x118,0x11e,0x128,0x132,0x13c,0x145,0x14a};
    const int textY[] = {82,133,199,265,331,398,465};
    const char* numberNames[] = {"RanNum0.def","RanNum1.def","RanNum2.def","RanNum3.def","RanNum4.def","RanNum5.def","RanNum6.def","RanNum7.def","RanNum8.def"};
    const char* sizeNames[] = {"RanSizS.def","RanSizM.def","RanSizL.def","RanSizX.def","RanUndr.def"};
    const char* waterNames[] = {"RanNone.def","RanNorm.def","RanIsld.def"};
    const char* strengthNames[] = {"RanWeak.def","RanNorm.def","RanStrg.def"};
    for (int prefix=0;prefix!=3;++prefix)
    for (int reserve=0;reserve!=2;++reserve) {
        TSingleSelectionWindow window;
        if (reserve) window.m_widgets.reserve(100);
        for (int i=0;i<prefix;++i) window.m_widgets.push_back(new button(0,0,1,1,-i-1,"existing",0,1,0,0,2));
        window.createFilterWidgets();
        if (window.m_widgets.size()!=static_cast<unsigned>(prefix+56)) return false;
        for (int i=0;i<prefix;++i) if (window.m_widgets[i]->id!=-i-1) return false;
        for (int i=0;i!=8;++i) if (window.m_randomMapOptions[i]!=options[i]) return false;
        button** arrays[] = {window.m_filterCountAButtons,window.m_filterCountBButtons,window.m_filterCountCButtons,window.m_filterCountDButtons,window.m_filterWaterButtons,window.m_filterStrengthButtons};
        for (int i=0;i!=56;++i) {
            widget* w=window.m_widgets[prefix+i];
            if (w->id!=0x118+i) return false;
            if (w->kind==1) {
                button* b=static_cast<button*>(w);
                const int values[]={0,1,0,0,2};
                for (int j=0;j!=5;++j) if (b->values[j]!=values[j]) return false;
                bool grouped=false;
                for (int group=0;group!=6;++group) {
                    int index=w->id-firstIds[group];
                    if (index<0 || index>=counts[group]) continue;
                    grouped=true;
                    if (arrays[group][index]!=b || b->m_highlightedFrame!=2 || b->m_disabledFrame!=1) return false;
                    const bool random=index==counts[group]-1;
                    int x=random?326:group<4?70+32*index:70+85*index;
                    int width=random?55:group<4?30:83;
                    int height=group==1 && (index==1 || index==2)?24:32;
                    if (w->x!=x || w->y!=rowY[group] || w->w!=width || w->h!=height) return false;
                    const char* name=random?"RanRand.def":group<4?numberNames[index+(group==0)]:group==4?waterNames[index]:strengthNames[index];
                    if (w->resource!=name) return false;
                }
                if (!grouped) {
                    if (b->m_highlightedFrame || b->m_disabledFrame) return false;
                    if (w->id==0x14f) {
                        if (w->x!=57 || w->y!=535 || w->w!=337 || w->h!=40 || w->resource!="RanShow.def") return false;
                    } else {
                        int index=w->id-0x119;
                        if (index<0 || index>=5 || w->x!=161+47*index || w->y!=81 || w->w!=44 || w->h!=33 || w->resource!=sizeNames[index]) return false;
                    }
                }
            } else if (w->kind==2) {
                textWidget* text=static_cast<textWidget*>(w);
                int row=0; while (row!=7 && textIds[row]!=w->id) ++row;
                if (row==7 || w->x!=(row?71:58) || w->y!=textY[row] || w->w!=(row==0?99:row<5?250:105) || w->h!=(row?16:31)) return false;
                if (w->resource!=g_generalText->getText(753+row) || text->face!="smalfont.fnt" || text->color!=font::PRIMARY || text->style!=0 || text->flags!=8 || text->alignment!=(row?font::LEFT_JUSTIFIED:font::CENTER_JUSTIFIED|font::VERT_CENTER_JUSTIFIED)) return false;
            } else return false;
        }
    }
    return true;
}
