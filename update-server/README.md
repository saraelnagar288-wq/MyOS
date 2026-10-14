# MyOS Update Server (initial implementation)

This directory documents the update metadata contract. The release workflow builds the bootable ISO and attaches three assets to each versioned GitHub Release:

- `MyOS.iso` — bootable image
- `MyOS.iso.sha256` — SHA-256 checksum
- `latest.json` — version and download metadata

## Publish a stable release

1. Merge the update-server branch after reviewing the workflow.
2. Create and push a version tag, for example: `git tag v0.1.0 && git push origin v0.1.0`.
3. Open **Actions → Release MyOS Update** and confirm the job completed.
4. Verify the release assets and checksum before distributing the ISO.

The workflow requires the repository's built-in GitHub Actions token to have `contents: write`. No personal access token or secret is needed.

## Manifest fields

- `product`, `version`, `tag`, `channel`
- `architecture` (`x86_64`)
- `artifact`, `size_bytes`, `sha256`
- `download_url`, `release_url`
- `install_mode`, `automatic_install_supported`

## Important limitations

- This private repository's release assets and manifest are not anonymously accessible. A MyOS installation cannot fetch them without an authenticated access design. **Never embed a GitHub personal access token in the OS image.** For public device updates, use a separate public release/metadata repository or a dedicated update service.
- SHA-256 detects accidental corruption but does not authenticate who published an update. Before implementing in-OS updates, add detached signatures (for example Ed25519) and pin the public verification key in the OS.
- This workflow publishes a bootable ISO, not a safe in-place updater. The current kernel has no verified A/B system partition or rollback mechanism. Automatic installation remains disabled until those mechanisms and network/TLS support are implemented and tested in QEMU.
- The existing build workflow and source files are not modified by this addition.
