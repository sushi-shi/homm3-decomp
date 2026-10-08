// Digits.h - the number of decimal digits of a maximum value, at compile
// time: TDigits<9999>::getDigits() is 4. The image keeps one linkonce body
// per bound (HeroPropsGeneralPage.cpp owns TDigits<9> to <99999999>), each
// calling the next smaller bound; TDigits<9> returns 1. TDigits<9> is an
// instance of the same template, not a specialization: ArmyDlg.o queues
// its body with the other bounds at the end of the file. The editor sizes
// its numeric entries with it. In h3maped /OPT:ICF folds the one-digit
// bounds onto one body (0x403178). The header name is not proven.
#ifndef HOMM3_EDITOR_DIGITS_H
#define HOMM3_EDITOR_DIGITS_H

template<unsigned int N>
class TDigits {
public:
    static unsigned int getDigits() { return N < 10 ? 1 : TDigits<N / 10>::getDigits() + 1; }
};

#endif  /* HOMM3_EDITOR_DIGITS_H */
