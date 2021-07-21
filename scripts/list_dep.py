#!/usr/bin/python3
# run from Radium-Engine/external directory
# This files generates information about Radium externals, and is used to generate the Documentation,

from pathlib import Path
import re

p = Path('.')

filenames=list(p.glob('**/CMakeLists.txt'))

name = re.compile(r'Add\([ ]*(.*)')
git = re.compile(r'GIT_REPOSITORY (.*)')
tag = re.compile(r'GIT_TAG (.*)')
option = re.compile(r'(-D.*)')
optionfilter = re.compile(r'indent')
installdir = re.compile(r'set\([^ ]* (${CMAKE_INSTALL_PREFIX}.*)\)')
installdir = re.compile(r'set\([^ ]* (.*)\)')

dep = {}
for filename in filenames:
    with open(filename, "r") as f:
        currentname="dummy"
        for line in f:
            match = name.search(line)
            if match:
                currentname=match.group(1)
                dep[currentname]={}
            for key,regex  in zip(['git', 'tag', 'option','installdir'], [git, tag, option, installdir]):
                match = regex.search(line)
                if match and not optionfilter.search(line) and currentname != 'dummy':
                    if key in dep[currentname]:
                        dep[currentname][key] += " "+match.group(1)
                    else:
                        dep[currentname][key] = match.group(1)
            if  currentname != 'dummy' and not 'installdir' in dep[currentname]:
                dep[currentname]['installdir']="default"

for key in dep:
    print(f" *  `{key}_DIR`")

print("\n\nRadium is compiled and tested with specific version of dependencies, as given in the external's folder CMakeLists.txt and state here for the record\n\n")

for key in dep:
    print(f" *  {key}: {dep[key]['git']}, [{dep[key]['tag']}],\n    *  with options `{dep[key]['option'] if 'option' in dep[key] else None }`")


print("\n\nConfigure your Radium build with:")

for key in dep:
    print(f"{key} {dep[key]['installdir']}")
