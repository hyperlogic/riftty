require 'rake/clean'

$CC = 'clang'

# used by tags task
class Dir
  # for each file in path, including sub-directories,
  # yield the full path of the filename to a block
  def Dir.for_each_rec path, &block
    if File.directory?(path)
      foreach path do |file|
        if file[0,1] != '.'
          for_each_rec File.expand_path(file, path), &block
        end
      end
    else
      yield path
    end
  end
end

# different flags for c and cpp
$C_FLAGS = ['-Wall',
            `sdl2-config --cflags`.chomp,
            `freetype-config --cflags`.chomp,
            '-DDARWIN',
            "-Iglyphblaster/src",
            "-Iabaci/src",
            "-Imintty/",
            "-I../OculusSDK/LibOVR/Include"
           ]

$CPP_FLAGS = ['-Wall',
            '--std=c++11',
            `sdl2-config --cflags`.chomp,
            `freetype-config --cflags`.chomp,
            '-DDARWIN',
            "-Iglyphblaster/src",
            "-Iabaci/src",
            "-Imintty/",
            "-I../OculusSDK/LibOVR/Include",
            "-fno-rtti"
           ]

$DEBUG_C_FLAGS = ['-g',
                  '-DDEBUG'
                 ]

$OPT_C_FLAGS = ['-O3', '-DNDEBUG'];

$L_FLAGS = [`sdl2-config --libs`.chomp,
            `freetype-config --libs`.chomp,
            '-lstdc++',
            '-lharfbuzz',
            "-L../OculusSDK/LibOVR/Lib/Mac/Release/",
            "-lovr",
            '-framework Cocoa',
            '-framework OpenGL',
            '-framework CoreServices',
            '-framework ApplicationServices',
            '-framework IOKit'
           ]

$OBJECTS = ['riftty.o',
            'pty.o',
            'keyboard.o',
            'joystick.o',
            'render.o',
            'shader.o',
            'appconfig.o',

            # mintty
            'mintty/config.o',
            'mintty/term.o',
            'mintty/termline.o',
            'mintty/termout.o',
            'mintty/termclip.o',
            'mintty/minibidi.o',
            'mintty/charset.o',
            'mintty/child.o',
            'mintty/std.o',
            'mintty/win.o',

            # glyphblaster
            'glyphblaster/src/cache.o',
            'glyphblaster/src/context.o',
            'glyphblaster/src/font.o',
            'glyphblaster/src/glyph.o',
            'glyphblaster/src/text.o',
            'glyphblaster/src/texture.o',
           ]

$GEN_HEADERS = ['FullbrightShader.h',
                'FullbrightTexturedShader.h',
                'PhongTexturedShader.h']

$DEPS = $OBJECTS.map {|f| f[0..-3] + '.d'}
$EXE = 'riftty'

def flags_for src
  if File.extname(src) == '.cpp'
    $CPP_FLAGS.join ' '
  else
    $C_FLAGS.join ' '
  end
end

# Use the compiler to build makefile rules for us.
# This will list all of the pre-processor includes this source file depends on.
def make_deps t
  sh "#{$CC} -MM -MF #{t.name} #{flags_for t.source} -c #{t.source}"
end

# Compile a single compilation unit into an object file
def compile obj, src
  sh "#{$CC} #{flags_for src} -c #{src} -o #{obj}"
end

# Link all the object files to create the exe
def do_link exe, objects
  sh "#{$CC} #{objects.join ' '} -o #{exe} #{$L_FLAGS.join ' '}"
end

# generate makefile rules from source code
rule '.d' => '.cpp' do |t|
  make_deps t
end
rule '.d' => '.c' do |t|
  make_deps t
end
rule '.d' => '.m' do |t|
  make_deps t
end

# adds .o rules so that objects will be recompiled if any of the contributing source code has changed.
task :add_deps => $DEPS do

  $GEN_HEADERS.each do |header|
    file header => "gen_shaders.rb" do |t|
      sh './gen_shaders.rb'
    end
  end

  $OBJECTS.each do |obj|
    dep = obj[0..-3] + '.d'
    raise "Could not find dep file for object #{obj}" unless dep

    # open up the .d file, which is a makefile rule (built by make_deps)
    deps = []
    File.open(dep, 'r') {|f| f.each {|line| deps |= line.split}}
    deps.reject! {|x| x == '\\'}  # remove '\\' entries

    # Add a new file rule which will build the object file from the source file.
    # Note: this object file depends on all the pre-processor includes as well
    file obj => deps[1,deps.size] do |t|
      compile t.name, t.prerequisites[0]
    end
  end
end

file :build_objs => $OBJECTS do
end

file :gen_shaders do
  rm_rf $EXE
  sh "./gen_shaders.rb"
end

file $EXE => [:add_deps, :build_objs] do
  do_link $EXE, $OBJECTS
end

task :build => $EXE

task :add_opt_flags do
  $C_FLAGS += $OPT_C_FLAGS
end
task :add_debug_flags do
  $C_FLAGS += $DEBUG_C_FLAGS
end

desc "Optimized Build"
task :opt => [:add_opt_flags, :gen_shaders, $EXE]

desc "Debug Build"
task :debug => [:add_debug_flags, :gen_shaders, $EXE]

desc "Optimized Build, By Default"
task :default => [:opt]

desc "ctags for emacs"
task :tags do
  sh "rm TAGS"

  PATHS = [File.expand_path('.')]
  # ends in .h, .cpp or .c (case insensitive match)
  SRC_PATTERN = /\.[hH]\z|\.[cC][pP][pP]\z|\.[cC]\z/
  src_files = []

  # fill temp_file with all the source files in PATHS
  PATHS.each do |path|
    Dir.for_each_rec(path) do |f|
      if SRC_PATTERN =~ f
        src_files << f
      end
    end
  end

  src_files.each {|f| sh "etags -a #{f}"}
end

desc "make an .app"
task :package do
  package_app_macos "riftty"
end

def package_app_macos app_name
  puts "Packaging #{app_name} for MacOSX"

  make_package_macos app_name

=begin
  # copy application into app dir
  if File.exists? "#{app_name}.rb"
    # simple app with only a single .rb file
    sh "cp #{app_name}.rb package/#{app_name}.app/Contents/MacOS/"
  elsif File.exists? "app/#{app_name}/#{app_name}.rb"
    # copy entire app sub-dir into package
    sh "cp -R app/#{app_name}/ package/#{app_name}.app/Contents/MacOS/app/#{app_name}/"
  else
    raise "Cannot Find app #{app_name}"
  end
=end

  # zip it up
  sh "cd package; zip -rq #{app_name}.zip #{app_name}.app"

  # remove the app dir
  #sh "rm -rf package/#{app_name}.app"

  puts ""
  puts "Finished : package/#{app_name}.zip"
end

def gsub_file filename, pattern, replace
  str = File.open(filename, "r") {|file| file.read}
  str.gsub! pattern, replace
  File.open(filename, "w") {|file| file.write str}
end

def make_package_macos app_name

  # delete the zip and app if it already exists
  sh "rm -rf package/#{app_name}.zip"
  sh "rm -rf package/#{app_name}.app"

  # create the app dir & copy the base resources into it.
  sh "mkdir package/#{app_name}.app"
  sh "cp -r package/mac_app/ package/#{app_name}.app/"

  # Copy riftty into app/Contents/MacOS
  dir_list = ['shader', 'font']
  dir_list.each do |dir|
    sh "cp -r #{dir}/ package/#{app_name}.app/Contents/MacOS/#{dir}"
  end

  # copy exe
  file_list = ['riftty']
  file_list.each do |file|
    sh "cp #{file} package/#{app_name}.app/Contents/MacOS/"
  end

  # make sure wrapper script is executable.
  sh "chmod +x package/#{app_name}.app/Contents/MacOS/launch"

  # get a list of all non-system dylibs referenced
  dylibs = list_dylibs app_name, 'riftty'

  # print out libs
  # puts ""
  # puts "libs"
  # dylibs.each do |lib|
  #   puts "    #{lib}"
  # end
  # puts ""

  # copy dylibs into app dir
  dylibs.each do |lib|
    sh "cp #{lib} package/#{app_name}.app/Contents/MacOS/"
    sh "chmod 777 package/#{app_name}.app/Contents/MacOS/#{File.basename(lib)}"
  end

  # use install_name_tool to fix up the install_names
  (dylibs + ['riftty']).each do |lib|
    otool(lib).each do |sub_lib|
      sh "install_name_tool -change #{sub_lib} @executable_path/#{File.basename sub_lib} package/#{app_name}.app/Contents/MacOS/#{File.basename lib}"
    end
  end

end

# returns the non-system dependencies of a given dylib
def otool filename
  # run otool to get a list of dylib dependencies.
  otool_output = `otool -L #{filename}`

  # strip out everything except the dylib path
  dylibs = otool_output.split("\n").map {|line| line.lstrip}[1..-1]
  dylibs.map! {|line| line.split("(")[0].rstrip}

  # remove any dylibs in /System/Library or /usr/lib
  dylibs.delete_if {|lib| lib =~ /(System\/Library\/)|(usr\/lib\/)/}
  dylibs
end

# recursivly builds a list of a libs runtime dependencies
def list_dylibs app_name, filename, list = []
  otool(filename).each do |lib|
    if !list.include? lib
      list.push lib
      list_dylibs app_name, lib, list
    end
  end
  list
end



CLEAN.include $DEPS, $OBJECTS, $GEN_HEADERS
CLOBBER.include $EXE

