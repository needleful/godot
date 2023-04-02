
#ifndef BITFIELD_ENUM_H
#define BITFIELD_ENUM_H

#ifdef __GNUC__
// Stop warning me about the bitfield being too small!
#define BITFIELD_ENUM
#else
// N.B. Any enum stored as a bitfield should
// be specified as UNSIGNED to work around
// some compilers trying to store it as signed,
// and requiring 1 more bit than necessary.
#define BITFIELD_ENUM : unsigned int
#endif // __GNUC__

#endif //BITFIELD_ENUM_H