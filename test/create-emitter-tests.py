import sys
import yaml
import hashlib

NS = 'Emitter'
DEFINE = 'YAML_GEN_TESTS'
EVENT_COUNT = 5

def encode_stream(line):
    for c in line:
        if c == '\n':
            yield '\\n'
        elif c == '"':
            yield '\\"'
        elif c == '\t':
            yield '\\t'
        elif ord(c) < 0x20:
            yield '\\x' + hex(ord(c))
        else:
            yield c

def encode(line):
    return ''.join(encode_stream(line))

def doc_start(implicit=False):
    if implicit:
        return {'emit': '', 'handle': 'DOC_START()'}
    else:
        return {'emit': 'YAML::BeginDoc', 'handle': 'DOC_START()'}

def doc_end(implicit=False):
    if implicit:
        return {'emit': '', 'handle': 'DOC_END()'}
    else:
        return {'emit': 'YAML::EndDoc', 'handle': 'DOC_END()'}

def scalar(value, tag='', anchor='', anchor_id=0):
    emit = []
    if tag:
        emit += ['YAML::VerbatimTag("%s")' % encode(tag)]
    if anchor:
        emit += ['YAML::Anchor("%s")' % encode(anchor)]
    if tag:
        out_tag = encode(tag)
    else:
        if value == encode(value):
            out_tag = '?'
        else:
            out_tag = '!'
    emit += ['"%s"' % encode(value)]
    return {'emit': emit, 'handle': 'SCALAR("%s", %s, "%s")' % (out_tag, anchor_id, encode(value))}

def comment(value):
    return {'emit': 'YAML::Comment("%s")' % value, 'handle': ''}

def seq_start(tag='', anchor='', anchor_id=0):
    emit = []
    if tag:
        emit += ['YAML::VerbatimTag("%s")' % encode(tag)]
    if anchor:
        emit += ['YAML::Anchor("%s")' % encode(anchor)]
    if tag:
        out_tag = encode(tag)
    else:
        out_tag = '?'
    emit += ['YAML::BeginSeq']
    return {'emit': emit, 'handle': 'SEQ_START("%s", %s)' % (out_tag, anchor_id)}

def seq_end():
    return {'emit': 'YAML::EndSeq', 'handle': 'SEQ_END()'}

def map_start(tag='', anchor='', anchor_id=0):
    emit = []
    if tag:
        emit += ['YAML::VerbatimTag("%s")' % encode(tag)]
    if anchor:
        emit += ['YAML::Anchor("%s")' % encode(anchor)]
    if tag:
        out_tag = encode(tag)
    else:
        out_tag = '?'
    emit += ['YAML::BeginMap']
    return {'emit': emit, 'handle': 'MAP_START("%s", %s)' % (out_tag, anchor_id)}

def map_end():
    return {'emit': 'YAML::EndMap', 'handle': 'MAP_END()'}

def gen_templates():
    yield [[doc_start(), doc_start(True)],
           [scalar('foo'), scalar('foo\n'), scalar('foo', 'tag'), scalar('foo', '', 'anchor', 1)],
           [doc_end(), doc_end(True)]]
    yield [[doc_start(), doc_start(True)],
           [seq_start()],
           [[], [scalar('foo')], [scalar('foo', 'tag')], [scalar('foo', '', 'anchor', 1)], [scalar('foo', 'tag', 'anchor', 1)], [scalar('foo'), scalar('bar')], [scalar('foo', 'tag', 'anchor', 1), scalar('bar', 'tag', 'other', 2)]],
           [seq_end()],
           [doc_end(), doc_end(True)]]
    yield [[doc_start(), doc_start(True)],
           [map_start()],
           [[], [scalar('foo'), scalar('bar')], [scalar('foo', 'tag', 'anchor', 1), scalar('bar', 'tag', 'other', 2)]],
           [map_end()],
           [doc_end(), doc_end(True)]]
    yield [[doc_start(True)],
           [map_start()],
           [[scalar('foo')], [seq_start(), scalar('foo'), seq_end()], [map_start(), scalar('foo'), scalar('bar'), map_end()]],
           [[scalar('foo')], [seq_start(), scalar('foo'), seq_end()], [map_start(), scalar('foo'), scalar('bar'), map_end()]],
           [map_end()],
           [doc_end(True)]]
    yield [[doc_start(True)],
           [seq_start()],
           [[scalar('foo')], [seq_start(), scalar('foo'), seq_end()], [map_start(), scalar('foo'), scalar('bar'), map_end()]],
           [[scalar('foo')], [seq_start(), scalar('foo'), seq_end()], [map_start(), scalar('foo'), scalar('bar'), map_end()]],
           [seq_end()],
           [doc_end(True)]]

def expand(template):
    if len(template) == 0:
        pass
    elif len(template) == 1:
        for item in template[0]:
            if isinstance(item, list):
                yield item
            else:
                yield [item]
    else:
        for car in expand(template[:1]):
            for cdr in expand(template[1:]):
                yield car + cdr
            

def gen_events():
    for template in gen_templates():
        for events in expand(template):
            base = list(events)
            for i in range(0, len(base)+1):
                cpy = list(base)
                cpy.insert(i, comment('comment'))
                yield cpy

def gen_tests():
    for events in gen_events():
        name = 'test' + hashlib.sha1(''.join(yaml.dump(event) for event in events)).hexdigest()[:20]
        yield {'name': name, 'events': events}
        

def create_emitter_tests(out):
    out.write('#ifdef %s\n' % DEFINE)
    out.write('namespace %s {\n' % NS)

    tests = list(gen_tests())

    for test in tests:
        out.write('TEST %s(YAML::Emitter& out)\n' % test['name'])
        out.write('{\n')
        for event in test['events']:
            emit = event['emit']
            if isinstance(emit, list):
                for e in emit:
                    out.write('    out << %s;\n' % e)
            elif emit:
                out.write('    out << %s;\n' % emit)
        out.write('\n')
        out.write('    HANDLE(out.c_str());\n')
        for event in test['events']:
            handle = event['handle']
            if handle:
                out.write('    EXPECT_%s;\n' % handle)
        out.write('    DONE();\n')
        out.write('}\n')

    out.write('}\n')
    out.write('#endif // %s\n\n' % DEFINE)

    out.write('void RunGenEmitterTests(int& passed, int& total)\n')
    out.write('{\n')
    out.write('#ifdef %s\n' % DEFINE)
    for test in tests:
        out.write('    RunGenEmitterTest(&Emitter::%s, "%s", passed, total);\n' % (test['name'], encode(test['name'])))
    out.write('#else // %s\n' % DEFINE)
    out.write('   (void)passed; (void)total;\n')
    out.write('#endif // %s\n' % DEFINE)
    out.write('}\n')

if __name__ == '__main__':
    create_emitter_tests(sys.stdout)
