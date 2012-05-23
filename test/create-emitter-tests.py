import sys
import yaml
import hashlib

NS = 'Emitter'
EVENT_COUNT = 5

EVENTS = [
    {'emit': 'YAML::DocStart', 'handle': 'DOC_START()'},
    {'emit': 'YAML::DocEnd', 'handle': 'DOC_END()'},
]

def gen_events():
    pass

def gen_tests():
    for events in gen_events():
        name = 'test' + hashlib.sha1(''.join(yaml.dump(event) for event in events)).hexdigest()[:20]
        yield {'name': name, 'events': events}
        

def create_emitter_tests(out):
    out.write('namespace %s {\n' % NS)

    for test in gen_tests():
        out.write('TEST %s(YAML::Emitter& out)\n' % test['name'])
        out.write('{\n')
        for event in test['events']:
            emit = event['emit']
            if emit:
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

if __name__ == '__main__':
    create_emitter_tests(sys.stdout)
