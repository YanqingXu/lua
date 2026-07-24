#!/usr/bin/env python3
"""Contract tests for release artifact validation."""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path


VERSION = "0.1.0-rc.1"
RID = "fixture-x64"
COMMIT = "0123456789abcdef"
ROOT_NAME = f"lua-cpp-{VERSION}-{RID}"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_validator(script: Path, directory: Path, *, check: bool) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            sys.executable,
            str(script),
            "--archive",
            str(directory / f"{ROOT_NAME}.zip"),
            "--sbom",
            str(directory / f"{ROOT_NAME}.spdx.json"),
            "--checksums",
            str(directory / f"{ROOT_NAME}.SHA256SUMS"),
            "--manifest",
            str(directory / f"{ROOT_NAME}.manifest.json"),
            "--expected-version",
            VERSION,
            "--expected-rid",
            RID,
            "--expected-commit",
            COMMIT,
        ],
        check=check,
        capture_output=True,
        text=True,
    )


def main() -> int:
    tool_directory = Path(__file__).parent
    validator = tool_directory / "validate_release_artifacts.py"
    generator = tool_directory / "generate_sbom.py"
    with tempfile.TemporaryDirectory() as temporary:
        directory = Path(temporary)
        package = directory / ROOT_NAME
        fixture_files = {
            "CHANGELOG.md": "changes\n",
            "SECURITY.md": "policy\n",
            "include/lauxlib.h": "/* lauxlib */\n",
            "include/lua.h": "/* lua */\n",
            "include/lua_cpp_version.h": "/* version */\n",
            "include/lua_runtime.h": "/* runtime */\n",
            "include/lualib.h": "/* lualib */\n",
            "lib/cmake/LuaCpp/LuaCppConfig.cmake": "# config\n",
            "lib/cmake/LuaCpp/LuaCppConfigVersion.cmake": "# version\n",
            "lib/cmake/LuaCpp/LuaCppTargets.cmake": "# targets\n",
            "lib/liblua_core.a": "static\n",
            "lib/liblua_public_api.so": "shared\n",
            "share/lua_cpp/LICENSE": "MIT\n",
            "share/lua_cpp/release/rc-notes-0.1.0.md": "notes\n",
            "share/lua_cpp/release/release-checklist.md": "checklist\n",
        }
        for relative, content in fixture_files.items():
            path = package / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding="utf-8")

        internal_sbom = package / "share/lua_cpp/sbom.spdx.json"
        subprocess.run(
            [
                sys.executable,
                str(generator),
                "--root",
                str(package),
                "--output",
                str(internal_sbom),
                "--name",
                ROOT_NAME,
                "--version",
                VERSION,
                "--commit",
                COMMIT,
            ],
            check=True,
        )
        external_sbom = directory / f"{ROOT_NAME}.spdx.json"
        external_sbom.write_bytes(internal_sbom.read_bytes())
        archive_path = directory / f"{ROOT_NAME}.zip"
        with zipfile.ZipFile(archive_path, "w", zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(package.rglob("*")):
                if path.is_file():
                    archive.write(path, Path(ROOT_NAME) / path.relative_to(package))

        checksum_path = directory / f"{ROOT_NAME}.SHA256SUMS"
        checksum_path.write_text(
            f"{sha256(archive_path)}  {archive_path.name}\n"
            f"{sha256(external_sbom)}  {external_sbom.name}\n",
            encoding="ascii",
        )
        manifest_path = directory / f"{ROOT_NAME}.manifest.json"
        manifest_path.write_text(
            json.dumps(
                {
                    "schemaVersion": 1,
                    "version": VERSION,
                    "runtimeIdentifier": RID,
                    "commit": COMMIT,
                    "archive": archive_path.name,
                    "sbom": external_sbom.name,
                    "checksums": checksum_path.name,
                }
            ),
            encoding="utf-8",
        )

        run_validator(validator, directory, check=True)

        altered_sbom = json.loads(external_sbom.read_text(encoding="utf-8"))
        altered_sbom["packages"][0]["packageVerificationCode"]["packageVerificationCodeValue"] = "0" * 40
        altered_text = json.dumps(altered_sbom, indent=2, sort_keys=True) + "\n"
        internal_sbom.write_text(altered_text, encoding="utf-8")
        external_sbom.write_text(altered_text, encoding="utf-8")
        with zipfile.ZipFile(archive_path, "w", zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(package.rglob("*")):
                if path.is_file():
                    archive.write(path, Path(ROOT_NAME) / path.relative_to(package))
        checksum_path.write_text(
            f"{sha256(archive_path)}  {archive_path.name}\n"
            f"{sha256(external_sbom)}  {external_sbom.name}\n",
            encoding="ascii",
        )
        rejected = run_validator(validator, directory, check=False)
        if rejected.returncode == 0 or "package verification code" not in rejected.stderr:
            raise AssertionError("validator accepted an invalid SPDX package verification code")

        external_sbom.write_text("{}\n", encoding="utf-8")
        rejected = run_validator(validator, directory, check=False)
        if rejected.returncode == 0 or "checksum manifest" not in rejected.stderr:
            raise AssertionError("validator accepted a modified external SBOM")

    print("Release artifact validator contract passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
