#!/usr/bin/env bash
# ============================================================================
#  PackageGame.sh — Package Asteroid Survivor into a standalone application
#
#  Usage:
#    ./PackageGame.sh                                      (auto-detect engine)
#    ./PackageGame.sh /Users/Shared/Epic\ Games/UE_5.7     (explicit path)
#
#  macOS:  Output goes to Build/MacPackage
#  Linux:  Output goes to Build/LinuxPackage
# ============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UPROJECT="${SCRIPT_DIR}/AsteroidSurvivor.uproject"

# Detect platform
if [[ "$(uname)" == "Darwin" ]]; then
    PLATFORM="Mac"
    OUTPUT_DIR="${SCRIPT_DIR}/Build/MacPackage"
else
    PLATFORM="Linux"
    OUTPUT_DIR="${SCRIPT_DIR}/Build/LinuxPackage"
fi

CONFIG="Shipping"

# --- Resolve engine root ---------------------------------------------------
find_engine() {
    # Explicit argument takes priority
    if [[ -n "${1:-}" ]]; then
        echo "$1"
        return
    fi

    # Check UE_ROOT environment variable
    if [[ -n "${UE_ROOT:-}" ]] && [[ -d "${UE_ROOT}" ]]; then
        echo "${UE_ROOT}"
        return
    fi

    # Common macOS paths
    local search_paths=(
        "/Users/Shared/Epic Games/UE_5.7"
        "/Users/Shared/Epic Games/UE_5.6"
        "/Users/Shared/Epic Games/UE_5.5"
        "$HOME/UnrealEngine"
        "/opt/UnrealEngine"
    )

    for p in "${search_paths[@]}"; do
        if [[ -f "${p}/Engine/Build/BatchFiles/RunUAT.sh" ]]; then
            echo "$p"
            return
        fi
    done

    return 1
}

UE_ROOT="$(find_engine "${1:-}")" || {
    echo ""
    echo "ERROR: Could not find Unreal Engine installation."
    echo ""
    echo "Please either:"
    echo "  1. Pass the engine path as an argument:"
    echo "     ./PackageGame.sh \"/Users/Shared/Epic Games/UE_5.7\""
    echo "  2. Set the UE_ROOT environment variable:"
    echo "     export UE_ROOT=\"/Users/Shared/Epic Games/UE_5.7\""
    echo ""
    exit 1
}

RUNUAT="${UE_ROOT}/Engine/Build/BatchFiles/RunUAT.sh"
if [[ ! -f "${RUNUAT}" ]]; then
    echo "ERROR: RunUAT.sh not found at \"${RUNUAT}\""
    echo "       Check that your engine path is correct."
    exit 1
fi

echo ""
echo "============================================================"
echo "  Packaging Asteroid Survivor"
echo "============================================================"
echo "  Engine : ${UE_ROOT}"
echo "  Project: ${UPROJECT}"
echo "  Output : ${OUTPUT_DIR}"
echo "  Platform: ${PLATFORM}"
echo "  Config : ${CONFIG}"
echo "============================================================"
echo ""

"${RUNUAT}" BuildCookRun \
    -project="${UPROJECT}" \
    -noP4 \
    -platform="${PLATFORM}" \
    -clientconfig="${CONFIG}" \
    -serverconfig="${CONFIG}" \
    -cook \
    -allmaps \
    -build \
    -stage \
    -pak \
    -archive \
    -archivedirectory="${OUTPUT_DIR}" \
    -utf8output

echo ""
echo "============================================================"
echo "  SUCCESS! Packaged game is in:"
echo "  ${OUTPUT_DIR}"
echo ""
echo "  Share the entire folder with your friend."
echo "  They can launch the game from the executable inside."
echo "============================================================"
echo ""
