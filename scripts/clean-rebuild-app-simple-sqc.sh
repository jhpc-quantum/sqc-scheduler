#!/usr/bin/env bash
#
# clean-rebuild-app-simple-sqc.sh - Remove installed artifacts of simple-sqc
#                                   (not its dependencies), then rebuild and
#                                   reinstall via build-simple-sqc.sh.
#
# Usage:
#   ./clean-rebuild-app-simple-sqc.sh [--clean-only]
#
#   --clean-only: only remove installed artifacts, skip the rebuild.
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/build-common.sh"

usage() {
    cat <<EOF
Usage: $(basename "$0") [--clean-only]
EOF
    exit 1
}

CLEAN_ONLY=0
for arg in "$@"; do
    case "${arg}" in
        --clean-only)
            CLEAN_ONLY=1
            ;;
        *)
            usage
            ;;
    esac
done

clean_simple_sqc
[ "${CLEAN_ONLY}" -eq 1 ] \
    || "${BUILD_SCRIPT_DIR}/build-simple-sqc.sh" \
    || die "rebuild of simple-sqc failed"

if [ "${CLEAN_ONLY}" -eq 1 ]; then
    log_info "Clean of simple-sqc completed."
else
    log_info "Clean rebuild of simple-sqc completed."
fi
