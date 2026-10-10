# Sourced by every devshell hook once HOMM3_DIR is set, before any tool runs.
#
# A shell entered from another project (HoMM1, HoMM2) leaves its MSVC_DIR,
# PYTHONPATH and HOMM*_ variables behind. Python resolves the toolchain from
# MSVC_DIR first, so an inherited one made the hook's compilation database
# regenerate the shared clang header mirror from that project's headers
# while a concurrent build was parsing against it. Drop the foreign state and
# export this checkout's toolchain before anything reads it.
for homm3_name in $(env | sed -n 's/^\(HOMM[A-Za-z0-9_]*\)=.*/\1/p'); do
    case $homm3_name in
        HOMM3_*) ;;
        *) unset "$homm3_name" ;;
    esac
done
unset homm3_name
export HOMM3_TOOLCHAIN="${HOMM3_TOOLCHAIN:-$HOMM3_DIR/build/homm3-toolchain-vc6-sp3}"
export MSVC_DIR="$HOMM3_TOOLCHAIN/msvc"
export PYTHONPATH="$HOMM3_DIR/scripts"
