"""homm3.vc6 - white-box model of the pinned VC6 SP3 toolchain.

The retail image is matched by whatever CL.EXE / C1XX.DLL / C2.DLL *do*;
this area reverse-engineers that behaviour into a predictive model of the
/Ob2 inliner and the register allocator, and ships solvers that replace
blind spelling sweeps in the match loop.

Subject binaries (RE targets), not the game. The four-part shape is copied
from homm2's /Od stack-slot model: model doc / predictor / real-compiler
oracle / census gate. See docs/vc6/README.md.
"""
