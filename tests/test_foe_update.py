#!/usr/bin/env python3
"""Host tests of production FoE callbacks, SOES dispatcher and image validator.

Flash is a word-addressed mock. These do not validate TI instruction execution,
FAPI timing/status encodings, ECC behavior or physical reset retention.
"""
import ctypes as C
import importlib.util
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("packer", ROOT / "tools/pack_foe_image.py")
packer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packer)
PAYLOAD = bytes([0x76, 0x12, 0x0A, 0x00]) + bytes(range(256)) * 8
IMAGE = packer.pack(PAYLOAD)


def build(folder, integrated):
    # Keep quoted esc.h resolution pointed at the test shim on a FULL checkout.
    shutil.copyfile(ROOT / "external/SOES/soes/esc_foe.c", folder / "esc_foe.c")
    output = folder / f"foe{integrated}.so"
    subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                    "-Wno-unused-parameter", "-fPIC",
                    "-dynamiclib" if sys.platform == "darwin" else "-shared",
                    f"-DFOE_UPDATE_INTEGRATED={integrated}",
                    "-I" + str(ROOT / "tests/stubs"), "-I" + str(ROOT),
                    "-I" + str(ROOT / "external/SOES"),
                    "-I" + str(folder),
                    "-I" + str(ROOT / "external/SOES/soes"), str(ROOT / "tests/foe_harness.c"),
                    str(ROOT / "boot/boot_image.c"), "-o", str(output)], check=True)
    lib = C.CDLL(str(output))
    lib.test_request.argtypes = [C.c_char_p, C.c_uint32]
    lib.test_request_n.argtypes = [C.c_char_p, C.c_uint32]
    lib.test_data.argtypes = [C.c_char_p, C.c_uint32, C.c_uint32]
    return lib


class Update(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.lib = build(Path(cls.tmp.name), 1)
        cls.disabled = build(Path(cls.tmp.name), 0)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def setUp(self):
        self.lib.test_reset(1)

    def open(self):
        self.assertEqual(self.lib.test_request(b"f28p65x.bin", 0xF280), 4)

    def upload(self, image=IMAGE, stop=None):
        self.open()
        end = len(image) if stop is None else stop
        last = 0
        for start in range(0, end, 116):
            chunk = image[start:min(start + 116, end)]
            last = self.lib.test_data(chunk, len(chunk), start // 116 + 1)
            if start + len(chunk) < len(image):
                self.assertEqual(last, 4)
        return last

    def test_success_and_cold_revalidation(self):
        self.assertEqual(self.upload(), 4)
        self.assertEqual(self.lib.test_valid(), 1)
        self.assertEqual(self.lib.test_count(2), 1)
        self.lib.test_reset(0)
        self.assertEqual(self.lib.test_valid(), 1)

    def test_default_rejects_before_erase(self):
        self.disabled.test_reset(1)
        self.assertEqual(self.disabled.test_request(b"f28p65x.bin", 0xF280), 5)
        self.assertEqual(self.disabled.test_count(1), 0)

    def test_exact_filename_password_and_boot_state(self):
        for name in [b"", b"f", b"f28p65x", b"f28p65x.binX", b"x" * 40]:
            self.assertEqual(self.lib.test_request(name, 0xF280), 5)
        self.assertEqual(self.lib.test_request_n(b"f28p65x.bin\0bad", 14), 5)
        self.assertEqual(self.lib.test_count(1), 0)
        self.assertEqual(self.lib.test_request(b"f28p65x.bin", 0), 5)
        self.lib.test_state(2)
        self.assertEqual(self.lib.test_request(b"f28p65x.bin", 0xF280), 5)
        self.assertEqual(self.lib.test_count(1), 0)

    def test_empty_upload_and_zero_terminal_error(self):
        self.open()
        self.assertEqual(self.lib.test_data(b"", 0, 1), 5)
        self.lib.test_reset(1)
        self.open()
        self.assertEqual(self.lib.test_data(IMAGE[:116], 116, 1), 4)
        self.assertEqual(self.lib.test_data(b"", 0, 2), 5)
        self.assertEqual(self.lib.test_valid(), 0)

    def test_failed_full_buffer_flush_has_no_ack(self):
        self.open()
        for i in range(8):
            self.assertEqual(self.lib.test_data(IMAGE[i*116:(i+1)*116], 116, i+1), 4)
        self.lib.test_fault(0)
        self.assertEqual(self.lib.test_data(IMAGE[928:1044], 116, 9), 5)
        self.assertEqual(self.lib.test_count(2), 0)
        self.assertEqual(self.lib.test_valid(), 0)

    def test_failed_tail_flush_has_no_ack(self):
        self.open()
        self.lib.test_fault(0)
        self.assertEqual(self.lib.test_data(IMAGE[:7], 7, 1), 5)
        self.assertEqual(self.lib.test_count(2), 0)

    def test_failed_invalidation_does_not_erase_payload(self):
        self.lib.test_fault(1)
        self.assertEqual(self.lib.test_request(b"f28p65x.bin", 0xF280), 5)
        self.assertEqual(self.lib.test_count(1), 1)
        self.assertEqual(self.lib.test_count(0), 0)

    def test_payload_corruption(self):
        image = bytearray(IMAGE)
        image[20] ^= 1
        self.assertEqual(self.upload(bytes(image)), 5)
        self.assertEqual(self.lib.test_valid(), 0)

    def test_manifest_fields_with_valid_header_crc(self):
        for offset, value in [(0, 0), (4, 2), (8, 42), (12, 0),
                              (12, 0xFFFFFFFF), (12, 5), (16, 0x80000), (24, 1)]:
            self.lib.test_reset(1)
            image = bytearray(IMAGE)
            struct.pack_into("<I", image, packer.CAPACITY + offset, value)
            struct.pack_into("<I", image, packer.CAPACITY + 28,
                             zlib.crc32(image[packer.CAPACITY:packer.CAPACITY+28]))
            self.assertEqual(self.upload(bytes(image)), 5)
            self.assertEqual(self.lib.test_valid(), 0)

    def test_bad_header_crc_and_nonempty_fill(self):
        for index in [packer.CAPACITY + 31, len(PAYLOAD) + 100]:
            self.lib.test_reset(1)
            image = bytearray(IMAGE)
            image[index] ^= 1
            self.assertEqual(self.upload(bytes(image)), 5)

    def test_reset_during_upload_never_activates(self):
        for count in [116, 928, 1044, (len(IMAGE) // 116) * 116]:
            self.lib.test_reset(1)
            self.upload(stop=count)
            self.lib.test_reset(0)
            self.assertEqual(self.lib.test_valid(), 0)

    def test_previous_image_invalidated_on_new_upload(self):
        self.assertEqual(self.upload(), 4)
        self.open()
        self.lib.test_reset(0)
        self.assertEqual(self.lib.test_valid(), 0)

    def test_torn_commit_marker(self):
        for words in range(8):
            self.lib.test_reset(1)
            self.lib.test_marker_cut(words)
            self.assertEqual(self.upload(), 5)
            self.lib.test_reset(0)
            self.assertEqual(self.lib.test_valid(), 0)

    def test_corruption_after_commit(self):
        for word in [0, 50, packer.CAPACITY // 2 + 14, 0x1FC00]:
            self.lib.test_reset(1)
            self.assertEqual(self.upload(), 4)
            self.lib.test_corrupt(word)
            self.assertEqual(self.lib.test_valid(), 0)

    def test_bad_packet_and_master_abort_and_mailbox_stop(self):
        self.open()
        self.assertEqual(self.lib.test_data(IMAGE[:116], 116, 2), 5)
        self.assertEqual(self.lib.test_data(IMAGE[:116], 116, 1), 5)
        self.open()
        self.lib.test_error()
        self.assertEqual(self.lib.test_data(IMAGE[:116], 116, 1), 5)
        self.open()
        self.lib.test_link_reset()
        self.assertEqual(self.lib.test_data(IMAGE[:116], 116, 1), 5)
        self.assertEqual(self.lib.test_valid(), 0)

    def test_extra_transport_data_rejected(self):
        self.assertEqual(self.upload(IMAGE + b"xx"), 5)

    def test_packer_limits(self):
        self.assertEqual(len(IMAGE), packer.WIRE_SIZE)
        for data in [b"", b"abc", b"\0" * 4, b"\xff" * 4, b"x" * (packer.CAPACITY+2)]:
            with self.assertRaises(ValueError):
                packer.pack(data)


if __name__ == "__main__":
    unittest.main(verbosity=2)
