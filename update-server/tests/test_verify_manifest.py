import hashlib
import json
import tempfile
import unittest
from pathlib import Path

from verify_manifest import ManifestError, verify_manifest


class VerifyManifestTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.artifact = self.root / "MyOS.iso"
        self.artifact.write_bytes(b"test ISO bytes")
        digest = hashlib.sha256(self.artifact.read_bytes()).hexdigest()
        self.manifest = {
            "product": "MyOS",
            "version": "0.1.0",
            "tag": "v0.1.0",
            "channel": "stable",
            "architecture": "x86_64",
            "artifact": "MyOS.iso",
            "size_bytes": self.artifact.stat().st_size,
            "sha256": digest,
            "download_url": "https://example.com/releases/download/v0.1.0/MyOS.iso",
            "checksum_url": "https://example.com/releases/download/v0.1.0/MyOS.iso.sha256",
            "release_url": "https://example.com/releases/tag/v0.1.0",
            "automatic_install_supported": False,
        }
        self.manifest_path = self.root / "latest.json"
        self.checksum_path = self.root / "MyOS.iso.sha256"
        self.checksum_path.write_text(f"{digest}  MyOS.iso\n", encoding="utf-8")
        self.write_manifest()

    def tearDown(self):
        self.temp.cleanup()

    def write_manifest(self):
        self.manifest_path.write_text(json.dumps(self.manifest), encoding="utf-8")

    def test_valid_manifest_artifact_and_checksum(self):
        result = verify_manifest(self.manifest_path, self.artifact, self.checksum_path)
        self.assertTrue(result["verified"])
        self.assertEqual(result["sha256"], self.manifest["sha256"])

    def test_rejects_corrupt_artifact(self):
        self.artifact.write_bytes(b"tampered")
        with self.assertRaises(ManifestError):
            verify_manifest(self.manifest_path, self.artifact)

    def test_rejects_size_mismatch(self):
        self.manifest["size_bytes"] += 1
        self.write_manifest()
        with self.assertRaises(ManifestError):
            verify_manifest(self.manifest_path, self.artifact)

    def test_rejects_http_url(self):
        self.manifest["download_url"] = "http://example.com/MyOS.iso"
        self.write_manifest()
        with self.assertRaises(ManifestError):
            verify_manifest(self.manifest_path, self.artifact)

    def test_rejects_automatic_install(self):
        self.manifest["automatic_install_supported"] = True
        self.write_manifest()
        with self.assertRaises(ManifestError):
            verify_manifest(self.manifest_path, self.artifact)

    def test_rejects_wrong_checksum_file(self):
        self.checksum_path.write_text("0" * 64 + "  MyOS.iso\n", encoding="utf-8")
        with self.assertRaises(ManifestError):
            verify_manifest(self.manifest_path, self.artifact, self.checksum_path)

    def test_rejects_missing_required_field(self):
        del self.manifest["architecture"]
        self.write_manifest()
        with self.assertRaises(ManifestError):
            verify_manifest(self.manifest_path, self.artifact)


if __name__ == "__main__":
    unittest.main()
