"""Check a move of globals into owner structs (plan, slices B2 and B3).

    python3 -I tools/phase5/owner_structs.py BASE [FILE...]

Run in the repository, with clang++ on the path. For each memory file that
changed since the commit BASE (or the FILEs named), it reads the file at BASE
and in the working tree through clang's syntax tree, and fails unless

- the variables and the owner structs' fields are the same set, each with the
  same type, array bounds included, the same linkage and the same initial
  value. A field counts as a variable of the file that defines its instance;
- no global needs a dynamic initialiser (clang's -Wglobal-constructors), so
  every instance is constant-initialised.

An owner struct is one whose header marks it "Campaign state owned by ..." or
"Combat state owned by ...". Types are compared as clang spells them, with
constants evaluated, so uint16_t[SLOTS] and uint16_t[8] are the same type and
uint16_t[8] and uint16_t[9] are not.
"""
import json,os,re,subprocess,sys,tempfile

MEMORY='src/Original/Memory'
OWNERS=set()  # the owner structs of the tree being read
MARK=re.compile(r'/\* (?:Campaign|Combat) state owned by [\w.]+ \(plan, slice B\d[a-e]?\)\. \*/\nstruct (\w+)\n')

def run(*args):
    return subprocess.run(args,check=True,capture_output=True).stdout

def clang(root,path,*more):
    return subprocess.run(['clang++','-std=c++20','-fsyntax-only','-I'+root+'/src/include','-I'+root+'/src',*more,
        root+'/'+path],capture_output=True)

def value(node):
    """An initialiser's tree, without the parts that name a place in a file.
    None where everything in it is zero without being written."""
    if node.get('kind')=='MemberExpr':  # an owner's instance.field is the variable the field was
        base=node['inner'][0]
        if base.get('kind')=='DeclRefExpr' and base['type']['qualType'] in OWNERS: return ((('kind','DeclRefExpr'),('ref',node['name'])),())
    if node.get('kind') in ('ImplicitValueInitExpr','CXXConstructExpr') and not node.get('inner'): return None
    keep={k:node[k] for k in ('kind','value','opcode','castKind','name') if k in node}
    if 'referencedDecl' in node: keep['ref']=node['referencedDecl'].get('name')
    # array_filler is the filler, then the elements that were written
    kids=[value(n) for n in node.get('inner',[])+node.get('array_filler',[])[1:]]
    while node.get('kind')=='InitListExpr' and kids and kids[-1] is None: kids.pop()
    if node.get('kind')=='InitListExpr' and not kids: return None
    return (tuple(sorted(keep.items())),tuple(kids))

def variables(root,path,owners):
    """{name: (type, linkage, initial value)} of the file-scope variables
    `path` defines, an owner struct's instance read as its fields."""
    if not os.path.exists(root+'/'+path): return {}
    dump=clang(root,path,'-Xclang','-ast-dump=json')
    if dump.returncode: sys.exit(dump.stderr.decode()[:2000])
    OWNERS.clear(); OWNERS.update(owners)
    records,found,main={},{},os.path.join(root,path)
    unit,current=json.loads(dump.stdout)['inner'],None
    for node in unit:
        loc=node.get('loc',{}); loc=loc.get('expansionLoc',loc)
        current=loc.get('file',current)  # the dump names the file only where it changes
        if node['kind']=='CXXRecordDecl' and node.get('completeDefinition') and node.get('name') in owners:
            records[node['name']]=[n for n in node['inner'] if n['kind']=='FieldDecl']
        if node['kind']!='VarDecl' or current!=main or node.get('storageClass')=='extern': continue
        init=node.get('inner',[None])[0] if 'init' in node else None
        kind=node['type']['qualType']
        if kind in records:
            values=init.get('inner',[]) if init else []
            if init and init['kind']!='InitListExpr': values=[]  # trivial default construction
            for i,f in enumerate(records[kind]):
                add(found,path,f['name'],f['type']['qualType'],'extern',value(values[i]) if i<len(values) else None)
        else:
            add(found,path,node['name'],kind,node.get('storageClass','extern'),value(init) if init else None)
    return found

def add(found,path,name,*what):
    if name in found: sys.exit(f'{path}: {name} is defined twice')
    found[name]=what

def owners(root):
    out=set()
    for d,_,files in os.walk(root+'/src/include'):
        for f in files:
            if f.endswith('.h'): out|=set(MARK.findall(open(os.path.join(d,f),encoding='latin1').read()))
    return out

def main():
    if len(sys.argv)<2: sys.exit(__doc__)
    base=run('git','rev-parse',sys.argv[1]+'^{commit}').decode().strip()
    here=run('git','rev-parse','--show-toplevel').decode().strip()
    files=sys.argv[2:] or [f for f in run('git','-C',here,'diff','--name-only',base,'--',MEMORY).decode().split() if f.endswith('.cpp')]
    if not files: sys.exit('no memory file changed')
    bad=0
    with tempfile.TemporaryDirectory() as old:
        tar=subprocess.run(['git','-C',here,'archive',base,'src'],check=True,capture_output=True).stdout
        subprocess.run(['tar','-x','-C',old],input=tar,check=True)
        # clang spells a file's path as it was given it: give it resolved ones
        old=os.path.realpath(old)
        before_owners,after_owners=owners(old),owners(here)
        for path in files:
            before,after=variables(old,path,before_owners),variables(here,path,after_owners)
            if not before and os.path.exists(here+'/'+path): bad+=1;print(f'{path}: no variables found at {base[:9]}, nothing was compared')
            for name in sorted(set(before)|set(after)):
                if before.get(name)!=after.get(name):
                    bad+=1;print(f'{path}: {name}: before {before.get(name)}, after {after.get(name)}')
            dynamic=os.path.exists(here+'/'+path) and clang(here,path,'-Wglobal-constructors','-Werror=global-constructors')
            if dynamic and dynamic.returncode: bad+=1;print(dynamic.stderr.decode()[:2000])
            print(f'{path}: {len(after)} variables compared, {sum(v[2] is not None for v in after.values())} with initial values')
    sys.exit(1 if bad else 0)

main()
