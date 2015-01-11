def options(opt):
  opt.load('compiler_cxx')

def configure(conf):
  conf.env.CXX = '/opt/llvm/bin/clang++'
  #conf.env.CXX = '/opt/gcc/bin/g++-4.9'
  conf.env.LINKFLAGS = '-Wl,-rpath,/opt/gcc/lib64'
  conf.env.CXXFLAGS = ['-std=gnu++14','-Wall','-Wno-narrowing','-O2','-Wmissing-braces']
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
      target="freq.dat freq_w_pos.dat pos_totals.dat word_lookup.dat words.dat totals.dat")

  gen("normalize",
      rule="./normalize", 
      source="word_lookup.dat words.dat",
      target="words_w_lower.dat lower_lookup.dat word_lower.dat")

  gen("tally_freqs_lower_pre",
      rule="./tally_freqs_lower_pre", 
      source="freq.dat word_lower.dat",
      target="freqs_lower_pre.dat")

  gen("tally_freqs_excl",
      rule="./tally_freqs_excl", 
      source="freq_w_pos.dat word_lower.dat",
      target="freqs_lower_exclude.dat")
  
  gen("tally_freqs_lower",
      rule="./tally_freqs_lower", 
      source="freqs_lower_pre.dat freqs_lower_exclude.dat",
      target="freqs_lower.dat counted.dat")

  gen("gather",
      rule="./gather",
      source="freqs_lower.dat counted.dat lower_lookup.dat",
      target="freq_all.dat freq_recent.dat")
