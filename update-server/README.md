# MyOS Updates: release pipeline and staged updater plan

## What this PR implements

- A native **My Settings → System Updates** screen in the framebuffer desktop.
- A clickable **Check for Updates** control and keyboard shortcut **U** to open the screen.
- A release workflow that builds `MyOS.iso` and publishes `MyOS.iso`, `MyOS.iso.sha256`, and `latest.json` for version tags such as `v0.1.0`.
- A clear status message: the in-OS online check is not implemented yet. The UI does not claim that it downloaded or installed anything.

## Build and publish a release

1. Confirm the build workflow passes on the branch to be released.
2. Merge the reviewed changes to the default branch.
3. Create and push a version tag, e.g. `git tag v0.1.0 && git push origin v0.1.0`.
4. Verify the **Release MyOS Update** workflow and inspect all three release assets.

A manual workflow run builds preview artifacts only; it does not publish a release unless triggered by a version tag. The workflow uses GitHub's built-in token, not a personal access token.

## Manifest contract

`latest.json` includes the product, version/tag, stable channel, x86_64 architecture, artifact name, file size, SHA-256 digest, download/release URLs, and an explicit `automatic_install_supported: false` flag.

## Required before a real in-OS update

The current MyOS kernel does not yet have a network stack/TLS client, an authenticated update endpoint, a filesystem-backed updater, or a verified A/B boot/rollback mechanism. This PR therefore does **not** download or install an update from inside MyOS.

Before enabling online updates:
1. Decide on a public metadata/artifact endpoint or a proper authenticated service. This repository is private; a MyOS image must never contain a personal access token.
2. Sign release metadata (e.g. Ed25519) and pin the verification public key in MyOS. SHA-256 alone only detects corruption; it does not authenticate the publisher.
3. Implement and test networking/TLS and a downloader with strict size/hash/signature checks.
4. Design A/B system slots and automatic rollback if the new slot fails to boot.
5. Test failure cases in QEMU before exposing any install action. Never write to a user's disk automatically.

## Cursor change

This branch is based on the Windows-style cursor/smoother PS/2 polling change from PR #5, so the cursor work is included in the proposed combined changeset. Both UI and mouse behavior still need a successful build and QEMU movement/click test before merging.
