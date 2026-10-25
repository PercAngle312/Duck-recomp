"""Host contract for explicit function-map generator (no private assets)."""
import csv
import tempfile
import unittest
from pathlib import Path
from generate_dispatch import generate

class GeneratorTests(unittest.TestCase):
    def test_sorted_and_prototypes(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td); source=root/"addresses.csv"; output=root/"registry.c"
            source.write_text("vram,symbol\n0x80002000,func_B\n0x80001000,func_A\n")
            self.assertEqual(generate(source,output),2)
            result=output.read_text()
            self.assertLess(result.index("{0x80001000u"), result.index("{0x80002000u"))
            self.assertIn("extern void func_A(uint8_t *, recomp_context *);",result)
    def test_rejects_bad_data(self):
        for entry in ("0x80001001,foo","0x80001000,foo-bar",
                      "0x80001000,x\n0x80001000,y",
                      "0x80001000,x\n0x80002000,x",
                      "1234,x"):
            with self.subTest(entry=entry), tempfile.TemporaryDirectory() as td:
                root=Path(td); source=root/"bad.csv"
                source.write_text("vram,symbol\n"+entry+"\n")
                with self.assertRaises(ValueError):
                    generate(source,root/"out.c")
                self.assertFalse((root/"out.c").exists())

if __name__=="__main__":
    unittest.main()
