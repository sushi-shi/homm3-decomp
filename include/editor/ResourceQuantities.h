// ResourceQuantities.h - an amount of each of the seven resources
// (Loki h3maped ResourceQuantities.cpp; 0x1c bytes inside TTimedEvent).
// Declared as far as its users need it; the class is matched with its own
// object.
#ifndef HOMM3_EDITOR_RESOURCEQUANTITIES_H
#define HOMM3_EDITOR_RESOURCEQUANTITIES_H

class TRawIStream;
class TRawOStream;

class TResourceQuantities {
public:
    TResourceQuantities();
    TResourceQuantities& operator=(const TResourceQuantities& other);

private:
    int _m_quantities[7];
};

bool operator==(const TResourceQuantities& lhs, const TResourceQuantities& rhs);
TRawIStream& operator>>(TRawIStream& stream, TResourceQuantities& quantities);
TRawOStream& operator<<(TRawOStream& stream, const TResourceQuantities& quantities);

#endif  /* HOMM3_EDITOR_RESOURCEQUANTITIES_H */
