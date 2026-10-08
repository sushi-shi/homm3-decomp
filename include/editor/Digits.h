// Digits.h - the number of decimal digits of a maximum value, at compile
// time: TDigits<9999>::getDigits() is 4. The image keeps one linkonce body
// per bound (HeroPropsGeneralPage.cpp owns TDigits<9> to <99999999>), each
// calling the next smaller bound; TDigits<9> returns 1. The editor sizes its
// numeric entries with it. The header name is not proven.
#ifndef HOMM3_EDITOR_DIGITS_H
#define HOMM3_EDITOR_DIGITS_H

template<unsigned int N>
class TDigits {
public:
    static unsigned int getDigits() { return TDigits<N / 10>::getDigits() + 1; }
};

template<>
class TDigits<9> {
public:
    static unsigned int getDigits() { return 1; }
};

#endif  /* HOMM3_EDITOR_DIGITS_H */
