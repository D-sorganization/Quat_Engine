"""Architecture & Design-by-Contract tests for QuatEngine.

These tests enforce the layered architecture invariants described in SPEC.md
section 4 (Architecture Overview) and verify that public contracts declared
in key headers remain stable. They use a lightweight include-grep approach
rather than a full C++ parser, which is sufficient because the codebase
uses quoted relative includes (``#include "../math/Vec3.h"``) that are easy
to match.

Layered architecture (lower layers MUST NOT depend on higher layers)::

    math  <-  core  <-  (renderer | input)  <-  game  <-  demo

Also validates Design-by-Contract invariants for ``src/contracts.py`` and
the public surface of ``src/game/Weapons.h``.
"""

from __future__ import annotations

import importlib.util
import logging
import re
import sys
from pathlib import Path

import pytest

logger = logging.getLogger(__name__)

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC = REPO_ROOT / "src"


def _load_contracts_module():
    """Load ``src/contracts.py`` directly — there is no ``src/__init__.py``."""
    spec = importlib.util.spec_from_file_location("qe_contracts", SRC / "contracts.py")
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules["qe_contracts"] = module
    spec.loader.exec_module(module)
    return module


# Quoted relative include like `#include "../math/Vec3.h"` or `#include "../../core/Rng.h"`.
_REL_INCLUDE = re.compile(r'^\s*#\s*include\s+"((?:\.\./)+)([^"]+)"', re.MULTILINE)


def _iter_headers(subdir: str) -> list[Path]:
    """Return all .h/.hpp/.cpp files under ``src/<subdir>`` recursively."""
    base = SRC / subdir
    if not base.exists():
        return []
    return [
        p
        for p in base.rglob("*")
        if p.is_file() and p.suffix.lower() in {".h", ".hpp", ".cpp", ".cc"}
    ]


def _resolved_includes(path: Path) -> list[str]:
    """Return the set of layer-prefixed includes from a single file.

    For example, a file at ``src/game/Weapons.h`` containing
    ``#include "../core/Rng.h"`` yields ``["core/Rng.h"]``.
    """
    text = path.read_text(encoding="utf-8", errors="ignore")
    results: list[str] = []
    for match in _REL_INCLUDE.finditer(text):
        results.append(match.group(2))
    return results


def _forbidden_layer_includes(
    source_subdir: str, forbidden_prefixes: tuple[str, ...]
) -> list[tuple[Path, str]]:
    """Find any includes in ``source_subdir`` that reference forbidden layers."""
    logger.debug(
        "Checking for forbidden layer includes",
        extra={
            "source_subdir": source_subdir,
            "forbidden_prefixes": forbidden_prefixes,
        },
    )
    violations: list[tuple[Path, str]] = []
    for file in _iter_headers(source_subdir):
        for inc in _resolved_includes(file):
            if inc.startswith(forbidden_prefixes):
                logger.warning(
                    "Found forbidden layer include",
                    extra={"file": str(file), "include": inc},
                )
                violations.append((file, inc))
    if violations:
        logger.error(
            "Forbidden layer includes detected",
            extra={
                "source_subdir": source_subdir,
                "violation_count": len(violations),
            },
        )
    return violations


# ── Architecture invariants ────────────────────────────────────────────────


def test_math_has_no_upward_dependencies():
    """``src/math`` is the foundation — it must not include from any other layer."""
    logger.info("Testing math layer has no upward dependencies")
    forbidden = ("core/", "renderer/", "input/", "game/", "demo/")
    violations = _forbidden_layer_includes("math", forbidden)
    if not violations:
        logger.info("Math layer dependency check passed")
    assert not violations, f"math/ has forbidden upward includes: {violations}"


def test_core_depends_only_on_math():
    """``src/core`` may depend on math but not on renderer/input/game/demo."""
    forbidden = ("renderer/", "input/", "game/", "demo/")
    violations = _forbidden_layer_includes("core", forbidden)
    assert not violations, f"core/ has forbidden upward includes: {violations}"


def test_renderer_does_not_depend_on_game_or_demo():
    """``src/renderer`` must stay game-agnostic."""
    forbidden = ("game/", "demo/", "input/")
    violations = _forbidden_layer_includes("renderer", forbidden)
    assert not violations, f"renderer/ has forbidden upward includes: {violations}"


def test_input_does_not_depend_on_game_or_demo():
    """``src/input`` is a platform abstraction — no game/demo references."""
    forbidden = ("game/", "demo/", "renderer/")
    violations = _forbidden_layer_includes("input", forbidden)
    assert not violations, f"input/ has forbidden upward includes: {violations}"


def test_game_does_not_depend_on_demo():
    """``src/game`` modules must not depend on the demo wiring layer."""
    forbidden = ("demo/",)
    violations = _forbidden_layer_includes("game", forbidden)
    assert not violations, f"game/ has forbidden upward includes: {violations}"


def test_all_expected_layers_exist():
    """The directories referenced by the layering rules must all exist."""
    for subdir in ("math", "core", "renderer", "input", "game", "demo"):
        assert (SRC / subdir).is_dir(), f"Missing expected layer: src/{subdir}"


# ── Design-by-Contract: src/contracts.py ───────────────────────────────────


def test_require_decorator_enforces_precondition():
    """``require`` must raise ``ValueError`` when the predicate is falsy."""
    contracts = _load_contracts_module()

    @contracts.require(lambda x: x > 0, message="x must be positive")
    def sqrt_positive(x: float) -> float:
        return x**0.5

    assert sqrt_positive(4) == 2.0
    with pytest.raises(ValueError, match="x must be positive"):
        sqrt_positive(-1)


def test_ensure_decorator_enforces_postcondition():
    """``ensure`` must raise ``RuntimeError`` when the result predicate is falsy."""
    contracts = _load_contracts_module()

    @contracts.ensure(lambda result: result >= 0, message="must be non-negative")
    def maybe_negative(x: int) -> int:
        return x

    assert maybe_negative(5) == 5
    with pytest.raises(RuntimeError, match="must be non-negative"):
        maybe_negative(-1)


# ── Weapons.h public contract surface ──────────────────────────────────────

WEAPONS_H = SRC / "game" / "Weapons.h"
INPUT_MANAGER_H = SRC / "input" / "InputManager.h"
TPS_INPUT_H = SRC / "game" / "tps" / "TPSInput.h"


def _weapons_source() -> str:
    assert WEAPONS_H.is_file(), f"Expected {WEAPONS_H} to exist"
    return WEAPONS_H.read_text(encoding="utf-8")


@pytest.mark.parametrize(
    "signature",
    [
        "void update(float dt)",
        "bool can_fire() const",
        "void fire()",
        "void reload()",
        "void switch_weapon(int index)",
        "void next_weapon()",
        "void prev_weapon()",
        "int current_index() const",
        "int weapon_count() const",
        "bool is_reloading() const",
        "float reload_progress() const",
        "float cooldown_progress() const",
    ],
)
def test_weapon_manager_exposes_public_signature(signature: str):
    """Each public WeaponManager method must remain declared in Weapons.h."""
    source = _weapons_source()
    assert signature in source, (
        f"Weapons.h no longer exposes `{signature}`. "
        "If this was intentional, update tests and SPEC.md."
    )


@pytest.mark.parametrize(
    "weapon_type",
    ["Pistol", "Shotgun", "RailGun", "RocketLauncher", "MiniGun"],
)
def test_weapon_type_enum_values_preserved(weapon_type: str):
    """The WeaponType enum must keep its five documented variants."""
    source = _weapons_source()
    assert re.search(rf"\b{weapon_type}\b", source), (
        f"WeaponType::{weapon_type} missing from Weapons.h"
    )


def test_weapons_header_documents_invariants():
    """Weapons.h must keep its public doc comment documenting weapon types.

    Negative-check: if someone strips the documentation, this test fails so
    the loss of documented behaviour is flagged in review.
    """
    source = _weapons_source()
    for phrase in ("Pistol", "Shotgun", "RailGun", "RocketLauncher", "MiniGun"):
        assert phrase in source, f"Weapons.h missing documented type '{phrase}'"
    # The header should retain the file-level docstring.
    assert "@file Weapons.h" in source
    assert "@brief" in source


def test_weapons_header_reloading_guard_invariant():
    """Weapons.h reload() must keep the `if (reloading_) return;` guard.

    Documented contract: calling ``reload()`` while already reloading is a
    no-op. Removing this guard would break the negative path tested in
    ``test_weapons.cpp``.
    """
    source = _weapons_source()
    assert "if (reloading_) return;" in source, (
        "reload() lost its reloading-while-reloading no-op guard"
    )


def test_weapons_header_switch_weapon_bounds_check():
    """``switch_weapon`` must keep its out-of-range guard (clamps via early return)."""
    source = _weapons_source()
    # The bounds check can be written in a few ways — accept any of them.
    patterns = [
        r"if\s*\(\s*index\s*<\s*0\s*\|\|\s*index\s*>=\s*static_cast<int>\(weapons_\.size\(\)\)\s*\)\s*return",
    ]
    assert any(re.search(p, source) for p in patterns), (
        "switch_weapon lost its out-of-range bounds check"
    )


def test_weapons_header_fire_respects_can_fire():
    """``fire()`` must early-return when ``can_fire()`` is false.

    This is the empty-ammo silent-fail contract covered by the C++ negative
    test ``test_weapons_fire_with_empty_ammo_silently_fails``.
    """
    source = _weapons_source()
    assert re.search(
        r"void\s+fire\s*\(\s*\)\s*\{\s*if\s*\(\s*!can_fire\(\)\s*\)\s*return", source
    ), "fire() no longer short-circuits on !can_fire()"


# ── Scroll-wheel audit contract (issue #206) ───────────────────────────────


def test_repo_has_no_editable_value_widget_framework_markers():
    """QuatEngine ships no editable value widgets that wheel input could mutate.

    The repo currently uses SDL gameplay input only. If a future change adds an
    immediate-mode GUI or widget toolkit with editable controls, this test
    forces an explicit audit instead of silently inheriting wheel-driven value
    changes from the framework defaults.
    """

    forbidden_markers = (
        "#include <imgui",
        '#include "imgui',
        "ImGui::Input",
        "ImGui::Slider",
        "ImGui::Combo",
        "QSpinBox",
        "QDoubleSpinBox",
        "QComboBox",
        "QSlider",
        "QDial",
        "QLineEdit",
        "nk_property",
        "nk_combo",
        "nk_slide",
    )

    offenders: list[tuple[str, str]] = []
    for file in _iter_headers(""):
        text = file.read_text(encoding="utf-8", errors="ignore")
        for marker in forbidden_markers:
            if marker in text:
                offenders.append((str(file.relative_to(REPO_ROOT)), marker))

    assert not offenders, (
        "Editable UI widget markers were found in QuatEngine source. "
        "Audit their wheel behavior before merging: "
        f"{offenders}"
    )


def test_mouse_wheel_is_routed_through_gameplay_scroll_only():
    """Mouse wheel input must stay a logical gameplay signal, not UI mutation."""

    input_manager = INPUT_MANAGER_H.read_text(encoding="utf-8")
    assert "case SDL_MOUSEWHEEL:" in input_manager
    assert "scroll_ += static_cast<float>(event.wheel.y);" in input_manager
    assert "float zoom() const" in input_manager
    assert "return scroll_ + gp;" in input_manager


def test_tps_wheel_exception_is_documented_as_weapon_selection():
    """The only approved wheel exception is gameplay weapon selection."""

    source = TPS_INPUT_H.read_text(encoding="utf-8")
    assert "D-pad / Mouse Wheel   → Weapon select direct" in source
