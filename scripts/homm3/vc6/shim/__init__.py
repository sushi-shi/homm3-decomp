"""homm3.vc6.shim - the C2-slot pass-through/instrumentation DLL.

v1 (this package): passthru.c logs the argv the CL driver hands to the back
end and forwards inertly to the real C2.DLL; build.py builds the overlay
toolchain and proves inertness with a byte-identity gate.  Phase 3 extends
the same slot with in-process hooks (inliner budget reads).

See docs/vc6/shim.md for the RE'd InvokeCompilerPass ABI and the gate design.
"""
