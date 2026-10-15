#!/usr/bin/env python3
"""Validate a MyOS release manifest and the downloaded ISO without installing it.

This tool is intended for CI and host-side inspection. It does not fetch URLs,
authenticate the publisher, or install/write an image to any disk.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
from urllib.parse import urlparse


class ManifestError(ValueError):
    """Raised when release metadata or its artifact is invalid."""


_SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
_REQUIRED = {
    "product", "version", "tag", "channel", "architecture", "artifact",
    "size_bytes", "sha256", "download_url", "checksum_url", "release_url",
    "automatic_install_supported",
}


def _https_url(value: object, field: str) -> None:
    if not isinstance(value, str):
        raise ManifestError(f"{field} must be a URL string")
    parsed = urlparse(value)
    if parsed.scheme != "https" or not parsed.netloc or parsed.username or parsed.password:
        raise ManifestError(f"{field} must be an absolute HTTPS URL without credentials")


def verify_manifest(manifest_path: Path, artifact_path: Path,
                    checksum_path: Path | None = None) -> dict:
    """Validate metadata, image size/hash, and optional standard sha256sum file."""
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ManifestError(f"Cannot read valid JSON manifest: {exc}") from exc

    if not isinstance(manifest, dict):
        raise ManifestError("Manifest root must be a JSON object")
    missing = sorted(_REQUIRED - manifest.keys())
    if missing:
        raise ManifestError("Missing required fields: " + ", ".join(missing))
    if manifest["product"] != "MyOS":
        raise ManifestError("Unexpected product")
    if not isinstance(manifest["version"], str) or not manifest["version"].strip():
        raise ManifestError("version must be a non-empty string")
    if manifest["tag"] != "v" + manifest["version"].removeprefix("v"):
        raise ManifestError("tag must match the version")
    if manifest["channel"] != "stable" or manifest["architecture"] != "x86_64":
        raise ManifestError("Unsupported channel or architecture")
    if manifest["artifact"] != "MyOS.iso":
        raise ManifestError("Unexpected artifact name")
    size = manifest["size_bytes"]
    if isinstance(size, bool) or not isinstance(size, int) or size <= 0:
        raise ManifestError("size_bytes must be a positive integer")
    digest = manifest["sha256"]
    if not isinstance(digest, str) or not _SHA256_RE.fullmatch(digest):
        raise ManifestError("sha256 must be 64 lowercase hexadecimal characters")
    for field in ("download_url", "checksum_url", "release_url"):
        _https_url(manifest[field], field)
    if manifest["automatic_install_supported"] is not False:
        raise ManifestError("Automatic installation must remain disabled until a signed, tested updater exists")

    try:
        data = artifact_path.read_bytes()
    except OSError as exc:
        raise ManifestError(f"Cannot read artifact: {exc}") from exc
    if not data:
        raise ManifestError("Artifact is empty")
    if len(data) != size:
        raise ManifestError(f"Artifact size mismatch: manifest={size}, actual={len(data)}")
    actual = hashlib.sha256(data).hexdigest()
    if actual != digest:
        raise ManifestError(f"Artifact SHA-256 mismatch: manifest={digest}, actual={actual}")

    if checksum_path is not None:
        try:
            parts = checksum_path.read_text(encoding="utf-8").strip().split()
        except (OSError, UnicodeError) as exc:
            raise ManifestError(f"Cannot read checksum file: {exc}") from exc
        if not parts or not _SHA256_RE.fullmatch(parts[0]):
            raise ManifestError("Checksum file must start with a 64-character SHA-256 digest")
        if parts[0] != actual:
            raise ManifestError("Checksum file does not match the artifact")
        if len(parts) > 1 and Path(parts[-1].lstrip("*")) .name != artifact_path.name:
            raise ManifestError("Checksum filename does not match the artifact")

    return {"product": "MyOS", "version": manifest["version"],
            "size_bytes": size, "sha256": actual, "verified": True}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path, help="path to latest.json")
    parser.add_argument("artifact", type=Path, help="path to MyOS.iso")
    parser.add_argument("--checksum", type=Path, help="optional MyOS.iso.sha256 file")
    args = parser.parse_args()
    try:
        result = verify_manifest(args.manifest, args.artifact, args.checksum)
    except ManifestError as exc:
        parser.error(str(exc))
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
