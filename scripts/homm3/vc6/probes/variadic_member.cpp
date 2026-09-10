// A stack-passed receiver does not distinguish a variadic member from a
// free formatter. See docs/vc6/variadic-members.md; compile with the game
// profile, including /Gr. No pragma or alternative declaration is needed.
#include <stdarg.h>

class VariadicChatProbe {
public:
    int m_state;
    int member(const char* format, ...);
};

int __cdecl freeChatProbe(VariadicChatProbe* manager, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    manager->m_state = va_arg(args, int);
    return manager->m_state + format[0];
}

int VariadicChatProbe::member(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    m_state = va_arg(args, int);
    return m_state + format[0];
}

extern VariadicChatProbe g_variadicChatProbe;

int callFreeChatProbe(const char* format, int number)
{
    return freeChatProbe(&g_variadicChatProbe, format, number);
}

int callMemberChatProbe(const char* format, int number)
{
    return g_variadicChatProbe.member(format, number);
}
