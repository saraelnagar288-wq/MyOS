#!/usr/bin/env python3
"""Build and validate the MyOS release manifest used by the update pipeline.

This tool only describes downloadable release assets. It never installs or writes
an image to a target disk.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from urllib.parse import urlparse

VERSION_RE = re.compile(r"^v?(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_checksum(path: Path) -> str:
    fields = path.read_text(encoding="utf-8").strip().split()
    if not fields or not SHA256_RE.fullmatch(fields[0]):
        raise ValueError(f"{path}: expected a 64-character lowercase SHA-256 digest")
    return fields[0]


def validate_manifest(manifest_path: Path, image_path: Path, checksum_path: Path) -> None:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    required = {
        "product", "version", "tag", "channel", "architecture", "artifact",
        "size_bytes", "sha256", "download_url", "checksum_url", "release_url",
        "install_mode", "automatic_install_supported",
    }
    missing = sorted(required - manifest.keys())
    if missing:
        raise ValueError("manifest is missing required fields: " + ", ".join(missing))
    if manifest["product"] != "MyOS" or manifest["channel"] != "stable":
        raise ValueError("unexpected product or release channel")
    if manifest["architecture"] != "x86_64" or manifest["artifact"] != "MyOS.iso":
        raise ValueError("unsupported architecture or artifact")
    if not VERSION_RE.fullmatch(str(manifest["version"])):
        raise ValueError("manifest version must be a stable semantic version such as 0.2.0")
    if manifest["tag"] != "v" + manifest["version"]:
        raise ValueError("manifest tag must match the version (v + version)")
    if manifest["install_mode"] != "manual" or manifest["automatic_install_supported"] is not False:
        raise ValueError("automatic installation must remain disabled")
    if not image_path.is_file() or image_path.stat().st_size <= 0:
        raise ValueError(f"image is missing or empty: {image_path}")
    actual_size = image_path.stat().st_size
    actual_hash = sha256_file(image_path)
    checksum_hash = read_checksum(checksum_path)
    if checksum_hash != actual_hash:
        raise ValueError("SHA-256 checksum file does not match the ISO")
    if manifest["sha256"] != actual_hash:
        raise ValueError("manifest SHA-256 does not match the ISO")
    if manifest["size_bytes"] != actual_size:
        raise ValueError("manifest size_bytes does not match the ISO")
    for key in ("download_url", "checksum_url", "release_url"):
        parsed = urlparse(str(manifest[key]))
        if parsed.scheme != "https" or not parsed.netloc:
            raise ValueError(f"{key} must be an absolute HTTPS URL")
    print(f"Manifest verified: v{manifest['version']} | {actual_size} bytes | SHA-256 {actual_hash}")


def create(args: argparse.Namespace) -> None:
    version_match = VERSION_RE.fullmatch(args.version)
    if not version_match:
        raise ValueError("version must be X.Y.Z or vX.Y.Z (stable semantic version)")
    version = args.version.removeprefix("v")
    image = Path(args.image)
    checksum = Path(args.checksum)
    output = Path(args.output)
    if not image.is_file() or image.stat().st_size <= 0:
        raise ValueError(f"image is missing or empty: {image}")
    digest = sha256_file(image)
    if read_checksum(checksum) != digest:
        raise ValueError("checksum file does not match the ISO")
    repo = args.repository.strip("/")
    server = args.server_url.rstrip("/")
    if not re.fullmatch(r"[^/\s]+/[^/\s]+", repo):
        raise ValueError("repository must be in owner/name format")
    if urlparse(server).scheme != "https" or not urlparse(server).netloc:
        raise ValueError("server URL must be an absolute HTTPS URL")
    tag = "v" + version
    base = f"{server}/{repo}/releases"
    manifest = {
        "schema_version": 1,
        "product": "MyOS",
        "version": version,
        "tag": tag,
        "channel": "stable",
        "architecture": "x86_64",
        "artifact": "MyOS.iso",
        "size_bytes": image.stat().st_size,
        "sha256": digest,
        "download_url": f"{base}/download/{tag}/MyOS.iso",
        "checksum_url": f"{base}/download/{tag}/MyOS.iso.sha256",
        "release_url": f"{base}/tag/{tag}",
        "install_mode": "manual",
        "automatic_install_supported": False,
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    validate_manifest(output, image, checksum)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    make = subparsers.add_parser("create", help="create and validate latest.json")
    make.add_argument("--image", required=True)
    make.add_argument("--checksum", required=True)
    make.add_argument("--version", required=True)
    make.add_argument("--repository", required=True)
    make.add_argument("--server-url", required=True)
    make.add_argument("--output", required=True)
    make.set_defaults(func=create)
    check = subparsers.add_parser("validate", help="validate an existing manifest and ISO")
    check.add_argument("--manifest", required=True)
    check.add_argument("--image", required=True)
    check.add_argument("--checksum", required=True)
    check.set_defaults(func=lambda a: validate_manifest(Path(a.manifest), Path(a.image), Path(a.checksum)))
    args = parser.parse_args()
    try:
        args.func(args)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"manifest validation failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
