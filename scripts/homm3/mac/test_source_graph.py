import json
from pathlib import Path
import tempfile
import unittest

from homm3.mac import source_graph


class SourceGraphTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from clang import cindex as ci
        if not ci.Config.loaded:
            from homm3.analysis.access_facts import load_cindex
            ci = load_cindex()
        cls.ci = ci

    def graph(self, text):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'src').mkdir()
            source = root / 'src/test.cpp'
            source.write_text(text)
            result = source_graph.scan(self.ci, source, ['-xc++', '-std=c++98'], root)
        self.assertEqual(result['diagnostics'], [])
        return source_graph.merge([{'unit': 'test', **result}])

    def test_overloads_receiver_and_declarator(self):
        graph = self.graph('''
            struct A { void f(int); void f(float); void f() const; };
            struct B { void f(int); };
            void caller(A& a, const A& c, B& b) { a.f(1); a.f(1.f); c.f(); b.f(1); }
        ''')
        edges = graph['edges']
        self.assertEqual(len(edges), 4)
        self.assertEqual(len({e['callee'] for e in edges}), 4)
        self.assertEqual([e['expression'] for e in edges], ['a.f(1)', 'a.f(1.f)', 'c.f()', 'b.f(1)'])

    def test_annotation_operators_constructor_and_nested_path(self):
        graph = self.graph('''
            struct A { A(); bool operator==(const A&) const; };
            __attribute__((annotate("mac:0x100 size:0x20"))) void g() {}
            void wrapper() { g(); }
            void caller() { A a; A b; if (a == b) wrapper(); }
        ''')
        nodes = {v['name']: v for v in graph['nodes'].values()}
        self.assertEqual(nodes['g']['mac'], [(0x100, 0x20)])
        self.assertEqual(sum(e['kind'] == 'constructor' for e in graph['edges']), 2)
        self.assertTrue(any(graph['nodes'][e['callee']]['name'] == 'A::operator==' for e in graph['edges']))
        found, truncated = source_graph.paths(graph, nodes['caller']['id'], {nodes['g']['id']})
        self.assertEqual(len(found[nodes['g']['id']]), 2)
        self.assertFalse(truncated)
        self.assertFalse(graph['implicit_operations_complete'])

    def test_redeclaration_prefix_keeps_exact_overload_location(self):
        graph = self.graph('''
            int cost(int x) { return x; }
            int cost(float x) { return int(x); }
            __attribute__((annotate("mac:0x100 size:0x20"))) int cost(int x);
        ''')
        nodes = [n for n in graph['nodes'].values() if n['name'] == 'cost']
        self.assertEqual(len(nodes), 2)
        declared = next(n for n in nodes if 'float' not in n['type'])
        self.assertTrue(any(start < end for _, start, end in declared['declaration_prefixes']))
        self.assertNotEqual(declared['declaration_prefixes'],
                            next(n for n in nodes if 'float' in n['type'])['declaration_prefixes'])

    def test_virtual_and_function_pointer_are_not_direct_paths(self):
        graph = self.graph('struct A { virtual void f(); }; void caller(A& a, void (*p)()) { a.f(); p(); }')
        self.assertEqual([e['dispatch'] for e in graph['edges']], ['virtual', 'unresolved'])
        caller = next(v['id'] for v in graph['nodes'].values() if v['name'] == 'caller')
        found, _ = source_graph.paths(graph, caller, set(graph['nodes']))
        self.assertFalse(found)

    def test_repeated_calls_and_header_deduplication(self):
        graph = self.graph('void g(); void f() { g(); g(); }')
        self.assertEqual(len(graph['edges']), 2)
        record = {'unit': 'a', 'nodes': list(graph['nodes'].values()), 'edges': graph['edges'],
                  'gaps': [], 'diagnostics': []}
        merged = source_graph.merge([record, {**record, 'unit': 'b'}])
        self.assertEqual(len(merged['edges']), 2)
        self.assertEqual(merged['edges'][0]['units'], ['a', 'b'])

    def test_cycles_and_depth_limit(self):
        graph = self.graph('void g(); void f(){ g(); } void g(){ f(); } void h(){ f(); }')
        nodes = {v['name']: v['id'] for v in graph['nodes'].values()}
        found, truncated = source_graph.paths(graph, nodes['h'], {nodes['g']}, max_depth=1)
        self.assertFalse(found)
        self.assertTrue(truncated)
        found, _ = source_graph.paths(graph, nodes['h'], {nodes['g']})
        self.assertEqual(len(found[nodes['g']]), 2)

    def test_incompatible_header_views_do_not_form_a_path(self):
        graph = self.graph('void g(); void f(){ g(); }')
        record = {'unit': 'a', 'nodes': list(graph['nodes'].values()), 'edges': graph['edges'],
                  'gaps': [], 'diagnostics': []}
        merged = source_graph.merge([record, {**record, 'unit': 'b', 'edges': []}])
        nodes = {n['name']: n for n in merged['nodes'].values()}
        self.assertEqual(nodes['f']['body_views'], [['a'], ['b']])
        found, truncated = source_graph.paths(merged, nodes['f']['id'], {nodes['g']['id']})
        self.assertFalse(found)
        self.assertTrue(truncated)

    def test_cache_invalidates_content_and_options(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'header.h'
            header.write_text('old')
            saved = {'key': 'flags', 'inputs': {str(header): source_graph.digest(header)}}
            self.assertTrue(source_graph.cache_valid(saved, 'flags'))
            self.assertFalse(source_graph.cache_valid(saved, 'other-flags'))
            header.write_text('new')
            self.assertFalse(source_graph.cache_valid(saved, 'flags'))
            header.unlink()
            self.assertFalse(source_graph.cache_valid(saved, 'flags'))

    def test_rad_asm_overlay_preserves_game_calls_and_vendor_file(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / 'vendor/bink-0.5a/include/Rad.h'
            header.parent.mkdir(parents=True)
            vendor_text = '''
                void increment(void* var) { __asm {
                    mov eax,[var]
                    lock inc [eax]
                } }
                void decrement(void* var) { __asm {
                    mov eax,[var]
                    lock dec [eax]
                } }
            '''
            header.write_text(vendor_text)
            source = root / 'src/test.cpp'
            source.parent.mkdir()
            source.write_text('''
                #include "vendor/bink-0.5a/include/Rad.h"
                void helper() {}
                void caller() { helper(); }
            ''')
            args = ['-xc++', '-std=c++98', '-fasm-blocks', '-fms-extensions',
                    '-target', 'i686-pc-windows-msvc', '-I' + str(root)]
            result = source_graph.scan(self.ci, source, args, root)
            self.assertEqual(result['diagnostics'], [])
            self.assertEqual([edge['expression'] for edge in result['edges']],
                             ['helper()'])
            self.assertEqual(header.read_text(), vendor_text)
            self.assertEqual(result['inputs'][str(header)], source_graph.digest(header))


if __name__ == '__main__':
    unittest.main()
