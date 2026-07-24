#!/usr/bin/env python3
"""Generate a deterministic-file-order SPDX 2.3 JSON SBOM."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha1(path: Path) -> str:
    digest = hashlib.sha1(usedforsecurity=False)
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def spdx_file_id(relative: str) -> str:
    digest = hashlib.sha1(relative.encode("utf-8"), usedforsecurity=False).hexdigest()
    return f"SPDXRef-File-{digest}"


def collect_files(root: Path, output: Path) -> list[Path]:
    files: list[Path] = []
    resolved_output = output.resolve()
    for path in root.rglob("*"):
        if path.is_file() and path.resolve() != resolved_output:
            files.append(path)
    return sorted(files, key=lambda path: path.relative_to(root).as_posix())


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--commit", required=True)
    args = parser.parse_args()

    root = args.root.resolve()
    output = args.output.resolve()
    if not root.is_dir():
        raise RuntimeError(f"SBOM root is not a directory: {root}")
    if not re.fullmatch(r"[0-9A-Za-z.+_-]+", args.version):
        raise RuntimeError("SBOM version contains unsupported characters")
    if not re.fullmatch(r"[0-9A-Fa-f]{7,64}", args.commit):
        raise RuntimeError("SBOM commit must be a hexadecimal revision")

    files = collect_files(root, output)
    if not files:
        raise RuntimeError("SBOM root contains no files")

    file_entries: list[dict[str, object]] = []
    verification_hashes: list[str] = []
    relationships: list[dict[str, str]] = [
        {
            "spdxElementId": "SPDXRef-DOCUMENT",
            "relationshipType": "DESCRIBES",
            "relatedSpdxElement": "SPDXRef-Package-LuaCpp",
        }
    ]
    for path in files:
        relative = path.relative_to(root).as_posix()
        checksum = sha256(path)
        verification_hashes.append(sha1(path))
        identifier = spdx_file_id(relative)
        file_entries.append(
            {
                "SPDXID": identifier,
                "fileName": f"./{relative}",
                "checksums": [{"algorithm": "SHA256", "checksumValue": checksum}],
                "licenseConcluded": "NOASSERTION",
                "licenseInfoInFiles": ["NOASSERTION"],
                "copyrightText": "NOASSERTION",
            }
        )
        relationships.append(
            {
                "spdxElementId": "SPDXRef-Package-LuaCpp",
                "relationshipType": "CONTAINS",
                "relatedSpdxElement": identifier,
            }
        )

    created = datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    namespace_commit = args.commit.lower()
    verification = hashlib.sha1(
        "".join(sorted(verification_hashes)).encode("ascii"),
        usedforsecurity=False,
    ).hexdigest()
    document = {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": f"{args.name}-{args.version}",
        "documentNamespace": (
            f"https://github.com/YanqingXu/lua/spdx/{args.version}/{namespace_commit}/{args.name}"
        ),
        "creationInfo": {
            "created": created,
            "creators": ["Tool: lua-cpp/tools/generate_sbom.py"],
        },
        "packages": [
            {
                "name": args.name,
                "SPDXID": "SPDXRef-Package-LuaCpp",
                "versionInfo": args.version,
                "downloadLocation": "NOASSERTION",
                "filesAnalyzed": True,
                "packageVerificationCode": {
                    "packageVerificationCodeValue": verification
                },
                "checksums": [],
                "licenseConcluded": "MIT",
                "licenseDeclared": "MIT",
                "copyrightText": "NOASSERTION",
                "externalRefs": [
                    {
                        "referenceCategory": "PACKAGE-MANAGER",
                        "referenceType": "purl",
                        "referenceLocator": f"pkg:github/YanqingXu/lua@{args.version}",
                    }
                ],
                "supplier": "NOASSERTION",
            }
        ],
        "files": file_entries,
        "relationships": relationships,
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(document, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"SPDX 2.3 SBOM: {len(files)} files -> {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
