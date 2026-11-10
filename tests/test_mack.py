"""Golden tests — locks our Mack implementation to Mack (1993) published values.

If any of these break, the math is wrong. Do not 'fix' the test to match the
code; fix the code to match the paper.
"""

import re
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "src" / "actukit-mack"
TRIANGLE = ROOT / "examples" / "mack_triangle.csv"

# ── Source of truth: Mack (1993) "Distribution-Free Calculation of the
# Standard Error of IBNR Reserves", ASTIN Bulletin 23(2).
MACK_TOTAL_RESERVE = 18_680_856.0
MACK_SE_TOTAL = 2_447_097.90
MACK_FACTORS = [
    3.490607,
    1.747333,
    1.457413,
    1.173852,
    1.103824,
    1.086269,
    1.053874,
    1.076555,
    1.017725,
]

# Tolerances — generous enough for float rounding at this scale.
TOL_RESERVE = 100.0
TOL_FACTOR = 1e-4

_TOTAL_RE = re.compile(r"Total reserve = ([\d.]+)\s+S\.E\. = ([\d.]+)")
_FACTOR_RE = re.compile(r"f\[(\d+)\] = ([\d.]+)\s+sigma2\[(\d+)\] = ([\d.]+)")


@pytest.fixture(scope="module")
def output() -> str:
    return subprocess.check_output([str(BIN), str(TRIANGLE)], text=True)


# ── Headline results ────────────────────────────────────────────────
def test_total_reserve_matches_paper(output):
    m = _TOTAL_RE.search(output)
    assert m, "could not parse 'Total reserve' line"
    got = float(m.group(1))
    assert got == pytest.approx(
        MACK_TOTAL_RESERVE, abs=TOL_RESERVE
    ), f"reserve {got:,.2f} != Mack's {MACK_TOTAL_RESERVE:,.2f}"


def test_standard_error_matches_paper(output):
    m = _TOTAL_RE.search(output)
    assert m, "could not parse 'Total reserve' line"
    got = float(m.group(2))
    assert got == pytest.approx(
        MACK_SE_TOTAL, abs=TOL_RESERVE
    ), f"S.E. {got:,.2f} != Mack's {MACK_SE_TOTAL:,.2f}"


# ── Intermediate quantities ─────────────────────────────────────────
def test_development_factors(output):
    found = {}
    for m in _FACTOR_RE.finditer(output):
        found[int(m.group(1))] = float(m.group(2))
    assert len(found) == len(
        MACK_FACTORS
    ), f"expected {len(MACK_FACTORS)} factors, got {len(found)}"
    for j, expected in enumerate(MACK_FACTORS):
        assert found[j] == pytest.approx(
            expected, abs=TOL_FACTOR
        ), f"f[{j}] = {found[j]:.6f}, expected {expected:.6f}"


def test_sigma2_extrapolation(output):
    """Mack eq 4.3: sigma^2[n-2] = min(sigma^2[n-3]^2 / sigma^2[n-4], ...)"""
    found = {}
    for m in _FACTOR_RE.finditer(output):
        found[int(m.group(3))] = float(m.group(4))
    s6, s7 = found[6], found[7]
    expected = min(s7 * s7 / s6, s6, s7)
    assert found[8] == pytest.approx(
        expected, rel=1e-6
    ), f"sigma2[8] = {found[8]:.4f}, expected {expected:.4f}"


def test_reserve_identity(output):
    """Sum of per-origin reserves must equal total (no numerical drift)."""
    per_origin = []
    for line in output.splitlines():
        if "|" in line and line.strip().startswith(tuple("0123456789")):
            parts = [p.strip() for p in line.split("|")]
            if len(parts) >= 3:
                try:
                    per_origin.append(float(parts[2]))
                except ValueError:
                    continue
    total = float(_TOTAL_RE.search(output).group(1))
    assert sum(per_origin) == pytest.approx(
        total, abs=1.0
    ), f"sum(per_origin)={sum(per_origin):,.2f} != total={total:,.2f}"
