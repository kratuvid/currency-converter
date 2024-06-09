#!/usr/bin/env python3

import subprocess
import os
import sys

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

release_flags = ['-O3', '-DNDEBUG']
debug_flags = ['-g', '-DDEBUG']

dirs = {
    'sources': 'src',
    'build': 'build',
    'objects': 'obj',
    'sys-bmi': 'sys-bmi'
}

cxx = ['clang++']
cxx_flags = ['-fdiagnostics-color=always', '-std=c++23', '-Wno-experimental-header-units']
ld_flags = []

sys_modules = ['cstdint', 'print']

# Properties: is module, primary dependencies
primaries = {
    'cc': [[True, []],
           ['cc.cppm', 'fused.cppm']],
    'main': [[False, ['cc']],
             ['main.cpp']]
}

targets = {
    'main': [ 'main', 'cc' ]
}

class Builder:
    args_nature = {
        'help': False,
        'release': False
    }

    def __init__(self):
        self.handle_arguments()

        self.make_directories()
        self.make_sys_modules()
        self.make_targets()

    def make_targets(self):
        self.primaries_checked = set()
        self.primaries_updated = set()

        for target in targets:
            target_path = Builder.join_paths([ self.dirs['build'], target ])

            for primary in targets[target]:
                self.secondaries_checked = set()
                self.secondaries_updated = set()
                self.module_flags = []

                self.make_primary(primary)

            if not Builder.is_exists(target_path) or len(self.primaries_updated) != 0:
                eprint('> Target:', target)
                self.link(target, [])

    def make_primary(self, primary):
        if primary in self.primaries_checked:
            return
        self.primaries_checked.add(primary)

        primary_sources = primaries[primary][1]
        is_module = primaries[primary][0][0]
        deps = primaries[primary][0][1]

        for dep in deps:
            self.make_primary(dep)

        if is_module:
            # MIU is module interface unit
            primary_miu = primary_sources[0]
            module_flags = []

            for unit_index in range(1, len(primary_sources)):
                source = primary_sources[unit_index]
                if source.endswith('.cppm'):
                    basename = source.removesuffix('.cppm')
                    bmi_name = primary + ':' + basename
                    bmi_path = Builder.join_paths([ self.dirs['objects'], primary, basename + '.pcm' ])
                    module_flags += ['-fmodule-file=' + bmi_name + '=' + bmi_path]
                self.make_secondary((primary, source), self.module_flags)

            self.make_secondary((primary, primary_miu), self.module_flags + module_flags)

            bmi_path = Builder.join_paths([ self.dirs['objects'], primary, primary + '.pcm' ])
            module_flags += ['-fmodule-file=' + primary + '=' + bmi_path]
            self.module_flags += module_flags

        else:
            for unit in primary_sources:
                self.make_secondary((primary, unit), self.module_flags)

        if len(self.secondaries_updated) != 0:
            self.primaries_updated.add(primary)

    def make_secondary(self, secondary, extra_flags):
        if secondary in self.secondaries_checked:
            return
        self.secondaries_checked.add(secondary)

        primary = secondary[0]
        unit = secondary[1]

        basename = Builder.removesuffixes(unit, ['.cpp', '.cppm'])
        path = Builder.join_paths([ dirs['sources'], primary, unit ])
        object_path = Builder.join_paths([ self.dirs['objects'], primary, basename + '.o' ])
        bmi_path = Builder.join_paths([ self.dirs['objects'], primary, basename + '.pcm' ])
        is_bmi = unit.endswith('.cppm')

        if (not Builder.is_exists(object_path) or Builder.is_later(path, object_path)) or (is_bmi and (not Builder.is_exists(bmi_path) or Builder.is_later(path, bmi_path))):
            eprint('> Source:', '/'.join([primary, unit]))
            self.compile(path, object_path, extra_flags)
            self.secondaries_updated.add(secondary)

    def make_sys_modules(self):
        for module in sys_modules:
            target = Builder.join_paths([ self.dirs['sys-bmi'], module + '.pcm' ])
            if not Builder.is_exists(target):
                eprint('> System BMI:', module)
                Builder.run(cxx + cxx_flags + ['-Wno-pragma-system-header-outside-header', '--precompile', '-xc++-system-header', module, '-o', target])

    def make_directories(self):
        os.makedirs(self.dirs['build'], exist_ok=True)
        os.makedirs(self.dirs['objects'], exist_ok=True)
        for primary in primaries:
            os.makedirs(Builder.join_paths([ self.dirs['objects'], primary ]), exist_ok=True)
        os.makedirs(self.dirs['sys-bmi'], exist_ok=True)

    def compile(self, source, target, extra_flags):
        Builder.run(cxx + self.type_flags + self.base_flags + cxx_flags + extra_flags + ['-fmodule-output', '-c', source, '-o', target])

    def link(self, target, extra_flags):
        target_path = Builder.join_paths([ self.dirs['build'], target ])

        objects = []
        for primary in targets[target]:
            for file in primaries[primary][1]:
                objects += [Builder.join_paths([ self.dirs['objects'], primary, Builder.replacesuffixes(file, ['.cpp', '.cppm'], '.o') ])]

        Builder.run(cxx + self.type_flags + self.base_flags + ld_flags + extra_flags + objects + ['-o', target_path])
    
    def removesuffixes(path, formers):
        for former in formers:
            path = path.removesuffix(former)
        return path

    def replacesuffixes(path, formers, later):
        return Builder.removesuffixes(path, formers) + later

    def join_paths(args):
        return '/'.join(args)

    def is_later(path, path2):
        return os.path.getmtime(path) > os.path.getmtime(path2)

    def is_exists(path):
        return os.path.exists(path)

    def run(args):
        eprint(' '.join(args))
        status = subprocess.run(args)
        if status.returncode != 0:
            raise Exception(f'Last command failed to code {status.returncode}')

    def handle_arguments(self):
        i = 1
        self.args = {}
        while i < len(sys.argv):
            this = sys.argv[i]
            if this in self.args_nature:
                if self.args_nature[this] is True:
                    if i == len(sys.argv)-1:
                        raise Exception(f'Argument \'{this}\' needs a parameter')
                    self.args[this] = sys.argv[i+1]
                    i += 1
                else:
                    self.args[this] = True
            i += 1

        if 'help' in self.args:
            eprint(self.args_nature)
            exit(1)

        self.type = 'release' if 'release' in self.args else 'debug'
        self.type_flags = release_flags if 'release' in self.args else debug_flags

        self.dirs = {'build': Builder.join_paths([ dirs['build'], self.type ])}
        self.dirs['objects'] = Builder.join_paths([ self.dirs['build'], dirs['objects'] ])
        self.dirs['sys-bmi'] = Builder.join_paths([ dirs['build'], dirs['sys-bmi'] ])

        self.base_flags = []
        for module in sys_modules:
            target = Builder.join_paths([ self.dirs['sys-bmi'], module + '.pcm' ])
            self.base_flags += ['-fmodule-file=' + target]


if __name__ == '__main__':
    instance = Builder()