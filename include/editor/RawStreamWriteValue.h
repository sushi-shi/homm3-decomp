// Scalar output shared by the two editors. Include after the owning
// edition defines TRawOStream. Both editors copy values into short-lived
// storage before passing them to the stream's reference-taking operator.
#ifndef HOMM3_EDITOR_RAWSTREAM_WRITE_VALUE_H
#define HOMM3_EDITOR_RAWSTREAM_WRITE_VALUE_H

template <class T>
inline void writeValue(TRawOStream& stream, T value)
{
    stream << value;
}

#endif  /* HOMM3_EDITOR_RAWSTREAM_WRITE_VALUE_H */
