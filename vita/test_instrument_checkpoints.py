import unittest
from instrument_checkpoints import instrument

class Checkpoints(unittest.TestCase):
    def test_loop_entry_and_comment_exclusion(self):
        source = '''RECOMP_FUNC void example(uint8_t* rdram, recomp_context* ctx) {
    const char *text = "goto nonexistent;";
    // fake: goto fake;
loop:
    ctx->r8++;
    if(ctx->r8<10) { ctx->r9++; goto loop; }
    goto done;
done:
    return;
}
'''
        generated, functions, targets = instrument(source)
        self.assertEqual((functions, targets), (1, 1))
        self.assertEqual(generated.count('duck_vita_thread_checkpoint(rdram, ctx);'), 2)
        self.assertIn('ctx->r9++; goto loop;', generated)
        self.assertIn('loop:\n    duck_vita_thread_checkpoint', generated)
        self.assertNotIn('done:\n    duck_vita_thread_checkpoint', generated)

    def test_rejects_duplicate_instrumentation_and_bad_jump(self):
        source = 'RECOMP_FUNC void f(uint8_t* rdram, recomp_context* ctx) {\nreturn;\n}\n'
        generated, _, _ = instrument(source)
        with self.assertRaises(ValueError): instrument(generated)
        with self.assertRaises(ValueError): instrument(source.replace('return;', 'goto missing;'))

if __name__ == '__main__':
    unittest.main()
