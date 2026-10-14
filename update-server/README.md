# MyOS Update Server and release manifest

## Current scope

The update center in the kernel is a UI placeholder. This repository currently builds release assets and a metadata manifest; it does **not** provide a running HTTP update server, an in-OS network client, TLS, signature verification, disk installation, or rollback. The kernel must continue to report online checking as unavailable and automatic installation as disabled.

## Release pipeline

For a stable release, use a tag such as `v0.2.0`:

1. Confirm the **Build MyOS** workflow passes.
2. Merge the reviewed changes to the default branch.
3. Push a stable semantic-version tag: `git tag v0.2.0 && git push origin v0.2.0`.
4. Verify **Release MyOS Update** completes and inspect `MyOS.iso`, `MyOS.iso.sha256`, and `latest.json` in the GitHub Release.

A manual workflow run creates preview artifacts only. It does not publish a release. The workflow uses GitHub's built-in token; never embed a personal access token or any other secret in the ISO or repository source.

## Manifest validation

`manifest.py` creates and validates `latest.json`. It checks stable version syntax, product/channel/architecture/artifact fields, HTTPS URLs, ISO size, and the SHA-256 value against both the checksum file and the actual image. The manifest explicitly sets `install_mode: manual` and `automatic_install_supported: false`.

Local validation example (after building the ISO):

```sh
sha256sum MyOS/core/MyOS.iso > MyOS/core/MyOS.iso.sha256
python3 update-server/manifest.py create \
  --image MyOS/core/MyOS.iso \
  --checksum MyOS/core/MyOS.iso.sha256 \
  --version 0.2.0 \
  --repository OWNER/REPOSITORY \
  --server-url https://github.com \
  --output dist/latest.json
python3 update-server/manifest.py validate \
  --manifest dist/latest.json \
  --image MyOS/core/MyOS.iso \
  --checksum MyOS/core/MyOS.iso.sha256
```

## Important access and security limits

- This repository is private. Standard GitHub Release URLs may require authentication; they are not a public update endpoint. Before an in-OS client is built, choose either a deliberately public release/metadata host or a properly authenticated service. Do not put a GitHub token in MyOS.
- SHA-256 detects accidental corruption but does **not** authenticate the publisher. Before downloads are trusted, sign metadata (for example with Ed25519), keep the signing private key outside the repository and CI logs, and pin the verification public key in MyOS.
- Implement and test networking/TLS, strict download-size limits, signature/hash checks, and failure handling.
- Design A/B system slots and verified rollback before enabling installation.
- Test invalid versions, malformed manifests, incorrect checksums, interrupted downloads, and failed boots in QEMU.
- Never write to a user's disk automatically. Keep installation disabled until disk detection, partitioning, file copying, and recovery have been implemented and tested.

## What this change does not claim

Passing manifest validation only proves that the generated metadata agrees with the release ISO and checksum. It does not mean an online update server, signed metadata, or an in-OS updater is live.
