// Uncopyable.h - a base that forbids copying (Loki h3maped). Its inline
// constructor and RTTI node are linkonce bodies; TTerrainPlacementOp
// derives from it. The header name is not proven.
#ifndef HOMM3_EDITOR_UNCOPYABLE_H
#define HOMM3_EDITOR_UNCOPYABLE_H

class TUncopyable {
protected:
    TUncopyable() {}

private:
    TUncopyable(const TUncopyable& other);
    TUncopyable& operator=(const TUncopyable& other);
};

#endif  /* HOMM3_EDITOR_UNCOPYABLE_H */
