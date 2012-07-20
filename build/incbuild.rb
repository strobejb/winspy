require 'pp'

def version_rx(ver)

    return %r{
      (?<name>\s*#{ver}\s*)
      (?<version_major> \d+)\s*,\s*
      (?<version_minor> \d+)\s*,\s*
      (?<version_build> \d+)\s*,\s*
      (?<version_count> \d+)\s*
    }x  
  
end

def versionstr_rx(ver)

  return %r{
    (?<prefix>\s*) 
    VALUE\s+\"(?<name>#{ver})\"\s*,\s*
    \"(?<value>.+)\"
  }x
end

def version_str(vals, sep='.', inccount=true)
   
   vervals = [ vals['VERSION_MAJOR'], vals['VERSION_MINOR'], vals['VERSION_BUILD'] ]
   vervals << vals['VERSION_BUILD_COUNT'] if inccount
   
   return vervals.collect {|v| v.to_s }.join(sep)

   #return "#{vals['VERSION_MAJOR']},#{vals['VERSION_MINOR']},#{vals['VERSION_BUILD']},#{vals['VERSION_BUILD_COUNT']}"
end


def update_resource(filename, vervals)

  text = File.open(filename).read

  newtext = ''

  text.each_line do |line|

    found = false

    # update FILEVERSION with specified version values
    m = line.match(version_rx('FILEVERSION'))
    if m
      found = true
      newtext << "#{m['name']}#{version_str(vervals,',')}\n"
    end

    # update PRODUCTVERSION with specified version values
    m = line.match(version_rx('PRODUCTVERSION'))
    if m
      found = true
      newtext << "#{m['name']}#{version_str(vervals,',')}\n"
    end

    # update FileVersion string
    m = line.match(versionstr_rx('FileVersion'))
    if m
      found = true
      newtext << "#{m['prefix']}VALUE \"#{m['name']}\", \"#{version_str(vervals,'.',true)}\"\n"
    end

    # update ProductVersion string
    m = line.match(versionstr_rx('ProductVersion'))
    if m
      found = true
      newtext << "#{m['prefix']}VALUE \"#{m['name']}\", \"#{version_str(vervals,'.',false)}\"\n"
    end    

    newtext << line unless found
  end

  # now write the file back, this time with modified values
  File.open(filename, 'w').write(newtext)
  
end

#
# get a hash of all #defines in the specified file
# yield to the caller so they can modify them, before
# we write them back to the same file
#
def getvalues(filename)

  vervars = {}

  text = File.open(filename).read

  # parse and extract all the #defines we can find
  text.each_line do |line|
    m = line.match(/#define\s+(?<name>\w+)\s+(?<value>\d+)/)   

    # update our hash with the #define name/value pair
    vervars[m['name']] = m['value'].to_i if m
  end

  # give the caller a chance to change them
  yield vervars
  newtext = ''

  # now parse the file again, but replace the #defines with new values
  text.each_line do |line|
    m = line.match(/#define\s+(?<name>\w+)\s+(?<value>\d+)/)   

    if m
      val = vervars[m['name']]
      newtext << "#define #{m['name']} #{val}\n"
    else
      newtext << line
    end
  end

  # now write the file back, this time with modified values
  File.open(filename, 'w').write(newtext)

  return vervars
end


def main(version_h, resource_rc)

  # increment the version!!!
  vals = getvalues(version_h) do |vals|
    vals['VERSION_BUILD_COUNT'] += 1
    pp vals
  end

  # update the resource with new version information
  puts "\nUpdating resource data"
  update_resource(resource_rc, vals)
  

end


if ARGV.count != 2
  puts "usage:"
  puts "  incbuild.rb <version.h> <resource.rc>"
  puts ""
  exit()
end

main(ARGV[0], ARGV[1])




