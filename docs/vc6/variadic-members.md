# Variadic members retain a stack-passed receiver

VC6 SP3 compiles a non-static variadic member using caller stack cleanup,
with `this` at `[ebp+8]`, its first explicit argument at `[ebp+0xc]`, and
the first variadic argument at `[ebp+0x10]`. `/Gr` does not make it a
fastcall operation. A free `__cdecl` function with an explicit receiver
can have identical instructions; stack arguments do not prove free ownership.

The isolated [probe](../../scripts/homm3/vc6/probes/variadic_member.cpp)
checks both definitions and both callers under the game profile:

```sh
PYTHONPATH=scripts python -m homm3.core.cc_wrap \
  --out build/vc6/variadic-member-probe.obj \
  --src scripts/homm3/vc6/probes/variadic_member.cpp -- \
  /nologo /c /O2 /Ob2 /Oy- /Op /ML /Gr /GX
llvm-objdump -dr build/vc6/variadic-member-probe.obj
```

The two callee bodies have the same 21 instruction bytes (excluding alignment),
including the `va_arg` read and plain `ret`. Both callers have the same
16 bytes: push EDX, push ECX, push the receiver, call, add 12 to ESP, return.
The call relocation names correctly differ. Treat the free-function form as
a negative control for the inference “stack receiver implies free function,”
not as an interchangeable source declaration.

Dreamcast's `CChatManager::PlayerDropMsg` and `AddChat` publics explicitly
preserve member ownership and ellipsis (`QAAXPBDZZ`), with their receiver
also present in the debug parameter records. Their x86 stack convention does
not contradict those facts. Restore the canonical member and its source
calls before diagnosing caller-budget or register-allocation differences;
do not hide the body behind an alternate free declaration.
