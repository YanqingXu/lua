#!/usr/bin/env python3
"""Validate a Lua C++ release archive, manifest, checksums, and SPDX SBOM."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import zipfile
from collections.abc import Mapping
from pathlib import Path, PurePosixPath


SUPPORTED_RELEASE_RIDS = ("windows-x64", "linux-x64", "macos-arm64")
PACKAGE_MANIFEST_FIELDS = {
    "schemaVersion",
    "version",
    "runtimeIdentifier",
    "commit",
    "archive",
    "sbom",
    "checksums",
}
_VERSION_RE = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+(?:-rc\.[0-9]+)?")
_SHA_RE = re.compile(r"[0-9a-fA-F]{40}")


class ReleaseArtifactError(RuntimeError):
    """Raised when a release package asset set is inconsistent."""


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
    raise ReleaseArtifactError(message)


def _json_object_without_duplicates(
    pairs: list[tuple[str, object]],
) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            fail(f"JSON object contains duplicate field: {key}")
        result[key] = value
    return result


def _reject_json_constant(value: str) -> object:
    fail(f"JSON contains unsupported constant: {value}")


def read_json_object(path: Path, label: str) -> Mapping[str, object]:
    try:
        payload = json.loads(
            path.read_text(encoding="utf-8-sig"),
            object_pairs_hook=_json_object_without_duplicates,
            parse_constant=_reject_json_constant,
        )
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        fail(f"{label} is not readable strict JSON: {path}: {error}")
    if not isinstance(payload, Mapping):
        fail(f"{label} root must be a JSON object")
    return payload


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


def validate_release_artifacts(
    *,
    archive: Path,
    sbom: Path,
    checksums: Path,
    manifest: Path,
    expected_version: str,
    expected_rid: str,
    expected_commit: str,
) -> Mapping[str, object]:
    """Deeply validate one canonical release package asset set."""

    if _VERSION_RE.fullmatch(expected_version) is None:
        fail("expected release version is invalid")
    if expected_rid not in SUPPORTED_RELEASE_RIDS:
        fail(f"unsupported release runtime identifier: {expected_rid}")
    expected_commit = expected_commit.lower()
    if _SHA_RE.fullmatch(expected_commit) is None:
        fail("expected release commit must be exactly 40 hexadecimal characters")

    expected_root = f"lua-cpp-{expected_version}-{expected_rid}"
    expected_paths = {
        archive: f"{expected_root}.zip",
        sbom: f"{expected_root}.spdx.json",
        checksums: f"{expected_root}.SHA256SUMS",
        manifest: f"{expected_root}.manifest.json",
    }
    for path, expected_name in expected_paths.items():
        if not path.is_file():
            fail(f"release artifact is missing: {path}")
        if path.name != expected_name:
            fail(
                f"release artifact filename mismatch: expected {expected_name}, "
                f"found {path.name}"
            )

    manifest_payload = read_json_object(manifest, "release manifest")
    actual_fields = set(manifest_payload)
    if actual_fields != PACKAGE_MANIFEST_FIELDS:
        fail(
            "release manifest field set mismatch; "
            f"missing={sorted(PACKAGE_MANIFEST_FIELDS - actual_fields)}, "
            f"extra={sorted(actual_fields - PACKAGE_MANIFEST_FIELDS)}"
        )
    expected_names = {
        "archive": archive.name,
        "sbom": sbom.name,
        "checksums": checksums.name,
    }
    if manifest_payload.get("schemaVersion") != 1:
        fail("release manifest schemaVersion must be 1")
    if manifest_payload.get("version") != expected_version:
        fail("release manifest version mismatch")
    if manifest_payload.get("runtimeIdentifier") != expected_rid:
        fail("release manifest runtime identifier mismatch")
    if manifest_payload.get("commit") != expected_commit:
        fail("release manifest commit mismatch")
    for key, value in expected_names.items():
        if manifest_payload.get(key) != value:
            fail(f"release manifest {key} does not identify {value}")

    checksum_entries = read_checksum_manifest(checksums)
    expected_checksums = {
        archive.name: sha256_file(archive),
        sbom.name: sha256_file(sbom),
    }
    if checksum_entries != expected_checksums:
        fail("release checksum manifest does not exactly match the archive and SBOM")

    archive_files: dict[str, bytes] = {}
    seen_members: set[str] = set()
    try:
        with zipfile.ZipFile(archive) as zipped:
            for info in zipped.infolist():
                member = validate_member_name(info.filename, expected_root)
                normalized = member.as_posix()
                if normalized in seen_members:
                    fail(f"archive contains a duplicate member: {normalized}")
                seen_members.add(normalized)
                if info.is_dir():
                    continue
                relative = PurePosixPath(*member.parts[1:]).as_posix()
                archive_files[relative] = zipped.read(info)
    except zipfile.BadZipFile as error:
        fail(f"release archive is not a readable ZIP: {archive}: {error}")

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

    external_sbom = sbom.read_bytes()
    internal_sbom = archive_files["share/lua_cpp/sbom.spdx.json"]
    if external_sbom != internal_sbom:
        fail("external SBOM is not byte-identical to the archived SBOM")
    sbom_payload = read_json_object(sbom, "release SBOM")
    validate_sbom(
        dict(sbom_payload),
        archive_files,
        expected_root,
        expected_version,
        expected_commit,
    )

    return {
        "root": expected_root,
        "file_count": len(archive_files),
        "manifest": dict(manifest_payload),
    }


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

    result = validate_release_artifacts(
        archive=args.archive,
        sbom=args.sbom,
        checksums=args.checksums,
        manifest=args.manifest,
        expected_version=args.expected_version,
        expected_rid=args.expected_rid,
        expected_commit=args.expected_commit,
    )
    print(
        "Release artifacts valid: "
        f"{result['root']}, {result['file_count']} files, "
        "checksums and SPDX coverage verified."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
