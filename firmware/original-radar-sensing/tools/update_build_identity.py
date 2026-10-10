"""Generate source/config identity, never claim it is the final BIN checksum."""
from pathlib import Path
import hashlib,json
root=Path(__file__).resolve().parents[1]
files=[]
for folder in ('src','dsp','spatial'):
    for pattern in ('*.c','*.h'):
        files+=list((root/folder).glob(pattern))
files=[p for p in files if p.name!='build_identity.h']
files += [root/n for n in ('Makefile','sensing.cfg','application.cmd','tools/update_build_identity.py')]
versions='SDK 03.06.02.00-LTS; ARM CGT 16.9.6.LTS; BIOS 6.73.01.01; XDC 3.50.08.24'
manifest={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
raw=json.dumps({'tool_versions':versions,'sources':manifest},sort_keys=True,separators=(',',':')).encode()
build_id=hashlib.sha256(raw).hexdigest()[:24]
(root/'src/build_identity.h').write_text('#ifndef ORIGINAL_BUILD_IDENTITY_H\n#define ORIGINAL_BUILD_IDENTITY_H\n#define OR_BUILD_ID "'+build_id+'"\n#endif\n')
(root/'BUILD_IDENTITY.json').write_text(json.dumps({'build_id':build_id,'meaning':'source/config plus pinned tool versions, not BIN checksum','tool_versions':versions,'sources':manifest},indent=2)+'\n')
print(build_id)
