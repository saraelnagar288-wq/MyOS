# MyOS update validation

## Host-side manifest verification

Run the verifier against a release manifest and its matching ISO:

```sh
python3 update-server/verify_manifest.py dist/latest.json dist/MyOS.iso --checksum dist/MyOS.iso.sha256
python3 -m unittest discover -s update-server/tests -v
```

The verifier checks required fields, product/version/tag consistency, x86_64 stable channel, HTTPS URLs, artifact size, SHA-256, and the optional checksum file. It deliberately rejects manifests that enable automatic installation. It does not download files, authenticate the publisher, or write to a disk.

## Current feature status

- **Native Welcome / Install / Sign in screens:** UI flow only. Sign in is not secure authentication yet.
- **My Files and Terminal in the kernel:** preview windows only. A real filesystem driver and a shell/userspace are still required.
- **Installer:** intentionally disabled. Do not enable it until block-device discovery, partition handling, filesystem creation, file copying, power-loss handling, and rollback have been implemented and tested.
- **Online update client:** not implemented in the kernel. The release pipeline publishes artifacts; this verifier is a host-side CI tool only.
- **Update authenticity:** SHA-256 detects accidental corruption but does not authenticate the publisher. Signed metadata (for example Ed25519 with a pinned public key), a trusted network/TLS stack, and rollback protection are required before a production updater can consume remote releases.

No code in this change automatically writes to user disks. The CI test suite covers valid metadata and tampered/malformed cases.
