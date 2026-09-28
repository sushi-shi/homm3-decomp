"""Compatibility import; comparison canonicalization lives in homm3.compare."""
import sys
from homm3.compare import canonicalize as _implementation
if __name__ == "__main__":
    raise SystemExit(_implementation.main())
sys.modules[__name__] = _implementation
