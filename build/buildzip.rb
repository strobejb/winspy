require 'zip/zip'
require 'fileutils'
require 'win32api'
require 'pp'

$ignore  = [ '.git' ]

def zipfile(zipfile, filespec, dest, ignore=[])

  spec = filespec
  spec = "#{filespec}/**/**" if File.directory?(filespec)

  Dir[spec].reject{ |f| f1=f.sub(dest+'/',''); ignore.count{|ig| File.fnmatch(ig,f1)} > 0 || f == zipfile || File.directory?(f) }.each do |file|
    
    d = dest
    d = File.join(dest, file.sub(filespec+'/','')) if File.directory?(filespec)
    puts "Adding:    #{d}"
    zipfile.add(d, file)

  end
end

def ziptext(zipfile, dest)

 puts "Adding:    #{dest}"

 zipfile.get_output_stream(dest) do |f|
      yield f
 end  

end

def version(file)

  s = ""

  # get the amount of space required to read in the version info
  vsize = Win32API.new('version.dll', 'GetFileVersionInfoSize', ['P', 'P'], 'L').call(file, s)

  ver = {}

  if vsize > 0
    result = ' ' * vsize

    # read the version
    Win32API.new('version.dll', 'GetFileVersionInfo', ['P', 'L', 'L', 'P'], 'L').call(file, 0, vsize, result)

    rstring = result.unpack('v*').map{ |s| s.chr if s < 256} * ''

    r = /FileVersion\D+([\d\.]+)\000/.match(rstring)
    ver[:filever] = r[1] if r

    r = /ProductVersion\D+([\d|\.]+)/.match(rstring)
    ver[:prodver] = r[1] if r

  end

  return ver
end

def build(platform)

  $bindir = "../bin/#{platform}/Release"

  winspybin = File.join($bindir, 'winspy.exe')

  # get the fileversion from exe resource section
  ver = version(winspybin)

  if ver.empty?
    puts "No version information: #{winspybin}"
    exit
  end

  puts "-" * 80
  puts "Packaging: #{winspybin} - version #{ver[:filever]}" 

  begin
    Dir.mkdir('out')
  rescue
  end

  zipname = "out/winspy-#{platform}-#{ver[:filever]}.zip"

  # delete any existing zip
  FileUtils.rm zipname, :force=>true

  # build the zip!
  Zip::ZipFile.open(zipname, Zip::ZipFile::CREATE) do |zf|

    zipfile(zf, '../README.md',      'WinSpy/README.TXT',  $ignore)
    zipfile(zf, '../LICENCE.TXT',    'WinSpy/LICENCE.TXT', $ignore)
    zipfile(zf, winspybin,           'WinSpy/winspy.exe',  $ignore)

    ziptext(zf, 'WinSpy/VERSION.TXT') do |f| 
      f.puts "WinSpy:   #{ver[:filever]}\r" 
      f.puts "Platform: #{platform}\r"
      f.puts "Built:    #{Time.now.strftime('%Y/%m/%d %H:%M:%S')}\r"
    end
  end

  len = File.size(zipname)
  puts ""
  puts "Done:      #{zipname}"
  puts "           #{len} bytes"
  puts ""

end

case ARGV[0]
when 'x86'
 build('x86')
when 'amd64'
 build('amd64')
else
 puts "Usage: buildzip.rb <x86|amd64>\n" 
end


