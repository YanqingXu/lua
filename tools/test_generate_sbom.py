#!/usr/bin/env python3
"""Contract test for the repository SPDX generator."""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    script = Path(__file__).with_name("generate_sbom.py")
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary) / "package"
        root.mkdir()
        (root / "include").mkdir()
        (root / "include" / "lua.h").write_text("/* fixture */\n", encoding="utf-8")
        (root / "LICENSE").write_text("MIT\n", encoding="utf-8")
        output = Path(temporary) / "fixture.spdx.json"
        subprocess.run(
            [
                sys.executable,
                str(script),
                "--root",
                str(root),
                "--output",
                str(output),
                "--name",
                "lua-cpp-fixture",
                "--version",
                "0.1.0-rc.1",
                "--commit",
                "0123456789abcdef",
            ],
            check=True,
        )
        payload = json.loads(output.read_text(encoding="utf-8"))
        assert payload["spdxVersion"] == "SPDX-2.3"
        assert payload["dataLicense"] == "CC0-1.0"
        assert len(payload["packages"]) == 1
        assert payload["packages"][0]["licenseDeclared"] == "MIT"
        assert len(payload["files"]) == 2
        for entry in payload["files"]:
            relative = entry["fileName"].removeprefix("./")
            expected = hashlib.sha256((root / relative).read_bytes()).hexdigest()
            assert entry["checksums"] == [{"algorithm": "SHA256", "checksumValue": expected}]
        file_sha1_values = sorted(
            hashlib.sha1(path.read_bytes(), usedforsecurity=False).hexdigest()
            for path in root.rglob("*")
            if path.is_file()
        )
        expected_verification = hashlib.sha1(
            "".join(file_sha1_values).encode("ascii"),
            usedforsecurity=False,
        ).hexdigest()
        assert payload["packages"][0]["packageVerificationCode"] == {
            "packageVerificationCodeValue": expected_verification
        }
        contains = [
            relationship
            for relationship in payload["relationships"]
            if relationship["relationshipType"] == "CONTAINS"
        ]
        assert len(contains) == 2
    print("SPDX SBOM generator contract passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
