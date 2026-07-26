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
RID = "windows-x64"
COMMIT = "0123456789abcdef" * 2 + "01234567"
ROOT_NAME = f"lua-cpp-{VERSION}-{RID}"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def create_release_asset_set(
    directory: Path,
    *,
    version: str = VERSION,
    rid: str = RID,
    commit: str = COMMIT,
) -> dict[str, Path]:
    root_name = f"lua-cpp-{version}-{rid}"
    package = directory / root_name
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

    generator = Path(__file__).parent / "generate_sbom.py"
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
            root_name,
            "--version",
            version,
            "--commit",
            commit,
        ],
        check=True,
    )
    external_sbom = directory / f"{root_name}.spdx.json"
    external_sbom.write_bytes(internal_sbom.read_bytes())
    archive_path = directory / f"{root_name}.zip"
    with zipfile.ZipFile(archive_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(package.rglob("*")):
            if path.is_file():
                archive.write(path, Path(root_name) / path.relative_to(package))

    checksum_path = directory / f"{root_name}.SHA256SUMS"
    checksum_path.write_text(
        f"{sha256(archive_path)}  {archive_path.name}\n"
        f"{sha256(external_sbom)}  {external_sbom.name}\n",
        encoding="ascii",
    )
    manifest_path = directory / f"{root_name}.manifest.json"
    manifest_path.write_text(
        json.dumps(
            {
                "schemaVersion": 1,
                "version": version,
                "runtimeIdentifier": rid,
                "commit": commit,
                "archive": archive_path.name,
                "sbom": external_sbom.name,
                "checksums": checksum_path.name,
            }
        ),
        encoding="utf-8",
    )
    return {
        "package": package,
        "internal_sbom": internal_sbom,
        "sbom": external_sbom,
        "archive": archive_path,
        "checksums": checksum_path,
        "manifest": manifest_path,
    }


def run_validator(
    script: Path,
    directory: Path,
    *,
    check: bool,
    version: str = VERSION,
    rid: str = RID,
    commit: str = COMMIT,
) -> subprocess.CompletedProcess[str]:
    root_name = f"lua-cpp-{version}-{rid}"
    return subprocess.run(
        [
            sys.executable,
            str(script),
            "--archive",
            str(directory / f"{root_name}.zip"),
            "--sbom",
            str(directory / f"{root_name}.spdx.json"),
            "--checksums",
            str(directory / f"{root_name}.SHA256SUMS"),
            "--manifest",
            str(directory / f"{root_name}.manifest.json"),
            "--expected-version",
            version,
            "--expected-rid",
            rid,
            "--expected-commit",
            commit,
        ],
        check=check,
        capture_output=True,
        text=True,
    )


def main() -> int:
    tool_directory = Path(__file__).parent
    validator = tool_directory / "validate_release_artifacts.py"
    with tempfile.TemporaryDirectory() as temporary:
        directory = Path(temporary)
        assets = create_release_asset_set(directory)
        package = assets["package"]
        internal_sbom = assets["internal_sbom"]
        external_sbom = assets["sbom"]
        archive_path = assets["archive"]
        checksum_path = assets["checksums"]
        manifest_path = assets["manifest"]

        run_validator(validator, directory, check=True)

        original_manifest = manifest_path.read_text(encoding="utf-8")
        extra_manifest = json.loads(original_manifest)
        extra_manifest["unexpected"] = True
        manifest_path.write_text(json.dumps(extra_manifest), encoding="utf-8")
        rejected = run_validator(validator, directory, check=False)
        if rejected.returncode == 0 or "field set mismatch" not in rejected.stderr:
            raise AssertionError("validator accepted an extra manifest field")
        manifest_path.write_text(
            original_manifest[:-1] + ', "version": "9.9.9"}',
            encoding="utf-8",
        )
        rejected = run_validator(validator, directory, check=False)
        if rejected.returncode == 0 or "duplicate field" not in rejected.stderr:
            raise AssertionError("validator accepted a duplicate manifest field")
        manifest_path.write_text(original_manifest, encoding="utf-8")

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
