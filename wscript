def options(opt):
  opt.load('compiler_cxx')

def configure(conf):
  #conf.env.CXX = '/opt/llvm/bin/clang++'
  conf.env.CXX = '/opt/gcc/bin/g++-4.9'
  conf.env.LINKFLAGS = '-Wl,-rpath,/opt/gcc/lib64'
  conf.env.CXXFLAGS = ['-std=gnu++14','-Wall','-Wno-narrowing','-O2','-Wno-missing-braces']
  conf.load('compiler_cxx')

def noop(tsk):
    pass

def build(bld):
  bld.objects(source="common.cpp", target="common")

  bld.program(use="common", source="dump.cpp", target="dump")

  def gen(prog, **args):
      bld.program(use="common", source=prog+".cpp", target=prog)
      bld(name="do-"+prog, source=prog, rule = noop)
      bld(after="do-"+prog, **args)
 
  gen("import",
      rule="zcat /aux/ngrams/data/googlebooks-eng-all-1gram-20120701-*.gz | ./import",
      #rule="zcat /aux/ngrams/data/googlebooks-eng-all-1gram-20120701-q.gz | ./import",
      target="Freqs.dat FreqsPos.dat PosTotals.dat Totals.dat WordLookup.dat words.dat")

  gen("normalize",
      rule="./normalize", 
      source="WordLookup.dat words.dat",
      target="ToLower.dat LowerLookup.dat words_w_lower.dat")

  gen("tally_freqs_excl",
      rule="./tally_freqs_excl", 
      source="FreqsPos.dat",
      target="FreqsExclude.dat")

  gen("tally_freqs_lower",
      rule="./tally_freqs_lower | tee tally_freqs_lower.log", 
      source="Freqs.dat FreqsExclude.dat ToLower.dat",
      target="FreqsLower.dat Counted.dat")
  
  gen("gather",
      rule="./gather",
      source="FreqsLower.dat Counted.dat LowerLookup.dat",
      target="FreqAll.dat FreqRecent.dat")
