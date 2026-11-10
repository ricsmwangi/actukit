"""Ensure the C++ binary is built before tests run."""

import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "src" / "actukit-mack"


@pytest.fixture(scope="session", autouse=True)
def build_binary():
    """Build the mack binary once per session if not present."""
    if not BIN.exists():
        subprocess.run(["make"], cwd=ROOT, check=True)
    return BIN
