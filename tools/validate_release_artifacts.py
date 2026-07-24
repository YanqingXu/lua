#!/usr/bin/env python3
"""Validate a Lua C++ release archive, manifest, checksums, and SPDX SBOM."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import zipfile
from pathlib import Path, PurePosixPath


def sha256_bytes(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def sha1_bytes(content: bytes) -> str:
    return hashlib.sha1(content, usedforsecurity=False).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def fail(message: str) -> None:
    raise RuntimeError(message)


def validate_member_name(name: str, expected_root: str) -> PurePosixPath:
    normalized = name.replace("\\", "/")
    path = PurePosixPath(normalized)
    if normalized.startswith("/") or re.match(r"^[A-Za-z]:", normalized):
        fail(f"archive member uses an absolute path: {name}")
    if any(part in ("", ".", "..") for part in path.parts):
        fail(f"archive member uses an unsafe path: {name}")
    if not path.parts or path.parts[0] != expected_root:
        fail(f"archive member is outside the expected root {expected_root}: {name}")
    return path


def read_checksum_manifest(path: Path) -> dict[str, str]:
    entries: dict[str, str] = {}
    for line_number, raw_line in enumerate(path.read_text(encoding="ascii").splitlines(), 1):
        if not raw_line.strip():
            continue
        match = re.fullmatch(r"([0-9a-fA-F]{64})  ([^/\\]+)", raw_line)
        if match is None:
            fail(f"invalid checksum line {line_number}: {raw_line!r}")
        checksum, name = match.groups()
        if name in entries:
            fail(f"duplicate checksum entry: {name}")
        entries[name] = checksum.lower()
    return entries


def validate_sbom(
    payload: dict[str, object],
    archive_files: dict[str, bytes],
    expected_name: str,
    expected_version: str,
    expected_commit: str,
) -> None:
    if payload.get("spdxVersion") != "SPDX-2.3":
        fail("SBOM is not SPDX 2.3")
    if payload.get("dataLicense") != "CC0-1.0":
        fail("SBOM dataLicense must be CC0-1.0")

    packages = payload.get("packages")
    if not isinstance(packages, list) or len(packages) != 1:
        fail("SBOM must describe exactly one package")
    package = packages[0]
    if not isinstance(package, dict):
        fail("SBOM package entry is invalid")
    if package.get("name") != expected_name:
        fail("SBOM package name does not match the archive")
    if package.get("versionInfo") != expected_version:
        fail("SBOM package version does not match the manifest")
    namespace = payload.get("documentNamespace")
    if not isinstance(namespace, str) or f"/{expected_commit.lower()}/" not in namespace:
        fail("SBOM namespace does not identify the release commit")

    files = payload.get("files")
    if not isinstance(files, list):
        fail("SBOM files entry is missing")
    expected_files = {
        relative: content
        for relative, content in archive_files.items()
        if relative != "share/lua_cpp/sbom.spdx.json"
    }
    described_files: dict[str, str] = {}
    described_ids: set[str] = set()
    for entry in files:
        if not isinstance(entry, dict):
            fail("SBOM file entry is invalid")
        file_name = entry.get("fileName")
        identifier = entry.get("SPDXID")
        checksums = entry.get("checksums")
        if not isinstance(file_name, str) or not file_name.startswith("./"):
            fail("SBOM fileName must be package-relative")
        relative = file_name[2:]
        if relative in described_files:
            fail(f"SBOM contains duplicate file: {relative}")
        if not isinstance(identifier, str) or identifier in described_ids:
            fail(f"SBOM has an invalid or duplicate SPDXID for {relative}")
        if (
            not isinstance(checksums, list)
            or len(checksums) != 1
            or not isinstance(checksums[0], dict)
            or checksums[0].get("algorithm") != "SHA256"
            or not isinstance(checksums[0].get("checksumValue"), str)
        ):
            fail(f"SBOM has an invalid SHA-256 entry for {relative}")
        described_files[relative] = checksums[0]["checksumValue"].lower()
        described_ids.add(identifier)

    if set(described_files) != set(expected_files):
        missing = sorted(set(expected_files) - set(described_files))
        extra = sorted(set(described_files) - set(expected_files))
        fail(f"SBOM file set mismatch; missing={missing}, extra={extra}")
    for relative, content in expected_files.items():
        actual = sha256_bytes(content)
        if described_files[relative] != actual:
            fail(f"SBOM checksum mismatch: {relative}")

    verification_code = package.get("packageVerificationCode")
    if not isinstance(verification_code, dict):
        fail("SBOM package verification code is missing")
    verification_hashes = sorted(sha1_bytes(content) for content in expected_files.values())
    expected_verification = hashlib.sha1(
        "".join(verification_hashes).encode("ascii"),
        usedforsecurity=False,
    ).hexdigest()
    if verification_code.get("packageVerificationCodeValue") != expected_verification:
        fail("SBOM package verification code does not match the archived files")

    relationships = payload.get("relationships")
    if not isinstance(relationships, list):
        fail("SBOM relationships entry is missing")
    contained_ids = {
        entry.get("relatedSpdxElement")
        for entry in relationships
        if isinstance(entry, dict)
        and entry.get("spdxElementId") == "SPDXRef-Package-LuaCpp"
        and entry.get("relationshipType") == "CONTAINS"
    }
    if contained_ids != described_ids:
        fail("SBOM CONTAINS relationships do not match its file entries")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--sbom", type=Path, required=True)
    parser.add_argument("--checksums", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--expected-version", required=True)
    parser.add_argument("--expected-rid", required=True)
    parser.add_argument("--expected-commit", required=True)
    args = parser.parse_args()

    for path in (args.archive, args.sbom, args.checksums, args.manifest):
        if not path.is_file():
            fail(f"release artifact is missing: {path}")

    manifest = json.loads(args.manifest.read_text(encoding="utf-8-sig"))
    expected_root = f"lua-cpp-{args.expected_version}-{args.expected_rid}"
    expected_names = {
        "archive": args.archive.name,
        "sbom": args.sbom.name,
        "checksums": args.checksums.name,
    }
    if manifest.get("schemaVersion") != 1:
        fail("release manifest schemaVersion must be 1")
    if manifest.get("version") != args.expected_version:
        fail("release manifest version mismatch")
    if manifest.get("runtimeIdentifier") != args.expected_rid:
        fail("release manifest runtime identifier mismatch")
    if str(manifest.get("commit", "")).lower() != args.expected_commit.lower():
        fail("release manifest commit mismatch")
    for key, value in expected_names.items():
        if manifest.get(key) != value:
            fail(f"release manifest {key} does not identify {value}")

    checksums = read_checksum_manifest(args.checksums)
    expected_checksums = {
        args.archive.name: sha256_file(args.archive),
        args.sbom.name: sha256_file(args.sbom),
    }
    if checksums != expected_checksums:
        fail("release checksum manifest does not exactly match the archive and SBOM")

    archive_files: dict[str, bytes] = {}
    seen_members: set[str] = set()
    with zipfile.ZipFile(args.archive) as archive:
        for info in archive.infolist():
            member = validate_member_name(info.filename, expected_root)
            normalized = member.as_posix()
            if normalized in seen_members:
                fail(f"archive contains a duplicate member: {normalized}")
            seen_members.add(normalized)
            if info.is_dir():
                continue
            relative = PurePosixPath(*member.parts[1:]).as_posix()
            archive_files[relative] = archive.read(info)

    required_files = {
        "CHANGELOG.md",
        "SECURITY.md",
        "include/lauxlib.h",
        "include/lua.h",
        "include/lua_cpp_version.h",
        "include/lua_runtime.h",
        "include/lualib.h",
        "lib/cmake/LuaCpp/LuaCppConfig.cmake",
        "lib/cmake/LuaCpp/LuaCppConfigVersion.cmake",
        "lib/cmake/LuaCpp/LuaCppTargets.cmake",
        "share/lua_cpp/LICENSE",
        "share/lua_cpp/release/rc-notes-0.1.0.md",
        "share/lua_cpp/release/release-checklist.md",
        "share/lua_cpp/sbom.spdx.json",
    }
    missing = sorted(required_files - set(archive_files))
    if missing:
        fail(f"release archive is missing required files: {missing}")
    if not any(path.startswith("lib/") and "/cmake/" not in path for path in archive_files):
        fail("release archive contains no link library")
    if not any(
        path.startswith(("bin/", "lib/"))
        and path.lower().endswith((".dll", ".dylib", ".so"))
        for path in archive_files
    ):
        fail("release archive contains no shared library")

    external_sbom = args.sbom.read_bytes()
    internal_sbom = archive_files["share/lua_cpp/sbom.spdx.json"]
    if external_sbom != internal_sbom:
        fail("external SBOM is not byte-identical to the archived SBOM")
    sbom_payload = json.loads(external_sbom.decode("utf-8"))
    validate_sbom(
        sbom_payload,
        archive_files,
        expected_root,
        args.expected_version,
        args.expected_commit,
    )

    print(
        "Release artifacts valid: "
        f"{expected_root}, {len(archive_files)} files, checksums and SPDX coverage verified."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
