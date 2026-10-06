import os
import sys

from homm3.core import root as _root

# Run the selected checkout's own tools (see root.foreign_code_reexec).
_plan = _root.foreign_code_reexec(_root.code_root(__file__))
if _plan is not None:
    _environment, _warning = _plan
    if _warning:
        print(_warning, file=sys.stderr)
    os.execve(sys.executable, [sys.executable, "-m", "homm3", *sys.argv[1:]], _environment)

from homm3.cli import main  # noqa: E402

sys.exit(main())
