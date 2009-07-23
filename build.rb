#!/usr/bin/env ruby

# Usage: ruby build.rb [option] [pcsx2, plugins, <plugin name>] [dev,debug,release] [all, install, clean]
# If you don't specify pcsx2, plugins, or a plugin name, it will assume you want to rebuild everything.
# If you don't specify dev or debug, it assumes a release build.
# If it isn't all, install, or clean, it assumes install.

# If you want other options, add them to $pcsx2_build_types. This is still a work in progress...
# --arcum42

require "fileutils.rb"
include FileUtils

$main_dir = Dir.pwd
$pcsx2_install_dir =  "#{$main_dir}/bin"
$plugin_install_dir =  "#{$main_dir}/bin/plugins"

$pcsx2_dir = "#{$main_dir}/pcsx2"
$plugins_dir = "#{$main_dir}/plugins"

$pcsx2_prefix = " --prefix #{$main_dir}"
$plugins_prefix = " --prefix #{$plugin_install_dir}"

$plugin_list=["CDVDnull",  "dev9null", "FWnull", "USBnull", "SPU2null", "zerogs", "zzogl", "zeropad", "zerospu2", "PeopsSPU2", "CDVDiso", "CDVDisoEFP",  "CDVDlinuz"]
$full_plugin_list=["CDVDnull",  "dev9null", "FWnull", "USBnull", "SPU2null", "zerogs", "zzogl", "zeropad", "zerospu2", "PeopsSPU2", "CDVDiso", "CDVDisoEFP",  "CDVDlinuz","GSnull","PadNull","onepad"]

$pcsx2_build_types = {
	"dev" => " --enable-devbuild ",
	"debug" => " --enable-debug ",
	"release" => " "
	}
	
$pcsx2_release_params=["dev","debug","release"]
$make_params=["all", "clean","install"]

$build_report =""
$build_counter = 0

def plugin_src_dir(plugin_name)
	name = "#{$plugins_dir}/#{plugin_name}/"
	case plugin_name
		when "CDVDiso" then
			name +=  "src"
		when "CDVDisoEFP" then 
			name +=  "src/Linux"
		when "CDVDlinuz"
			name +=  "Src/Linux"
		when "zerogs", "zzogl"
			name +=  "opengl"
	end
	
	return name
end

def announce(my_program)
	print "---------------\n"
	print "Building #{my_program}\n"
	print "---------------\n"
end

def make(options)
	system("make #{options}")
	($? == 0)
end

def rebuild(options)
	system("aclocal")
	system("automake")
	system("autoconf")
	system("chmod +x configure")
	system("./configure #{options}")
	make "clean"
end

def install(build_name)
	ret = make "install"
	
	case build_name
		# If the package isn't inclined to obey simple instructions...
		when "CDVDisoEFP" then 
			system("cp #{plugin_src_dir(build_name)}/cfgCDVDisoEFP #{$plugin_install_dir}")
			system("cp #{plugin_src_dir(build_name)}/libCDVDisoEFP.so #{$plugin_install_dir}")
		when "CDVDlinuz" then 
			system("cp #{plugin_src_dir(build_name)}/cfgCDVDlinuz #{$plugin_install_dir}")
			system("cp #{plugin_src_dir(build_name)}/libCDVDlinuz.so #{$plugin_install_dir}")
		when "PeopsSPU2" then 
			system("cp #{plugin_src_dir(build_name)}/libspu2Peops*.so* #{$plugin_install_dir}")
			
		# Copy the shaders over. Shouldn't the makefile do this?
		when "zzogl","zerogs" then
			system("cp #{plugin_src_dir(build_name)}/Win32/ps2hw.dat #{$plugin_install_dir}")
			
		#And while we have the opportunity...
		when "pcsx2" then 
		svn_revision = `svn info | grep Revision:`
		svn_revision = /[0-9]+/.match(svn_revision)
		system("cp #{$pcsx2_install_dir}/pcsx2 #{$pcsx2_install_dir}/pcsx2-#{svn_revision}")
	end
	
	ret
end

def build(build_name, make_parameter)
	completed = true
	
	announce "#{build_name.capitalize}"
	
	if build_name != "pcsx2" then
		build_dir = plugin_src_dir(build_name)
	else
		build_dir = "#{$pcsx2_dir}"
	end
	
	Dir.chdir build_dir
	
	case make_parameter
		when "all" then
			if build_name == "pcsx2"
				rebuild($pcsx2_prefix)
			else
				rebuild($plugins_prefix)
			end
			completed = install(build_name)
			
		when "clean" then
			make "clean"
			
		else
			completed = install(build_name)
		end
		
	Dir.chdir $main_dir
	
	if completed then
		$build_report += "#{build_name} was built successfully.\n"
		$build_counter += 1
	else
		$build_report += "#{build_name} was not built successfully.\n"
	end
			
end

build_parameter = "all"
make_parameter = ""
build_items = Array.new([])

ARGV.each do |x|
	make_parameter = x if $make_params.include?(x)
	
	build_items.push(x) if $full_plugin_list.include?(x) or (x == "pcsx2")
	$pcsx2_prefix = $pcsx2_build_types[x] + $pcsx2_prefix if $pcsx2_release_params.include?(x)
	
	if (x == "plugins") then
		x = $plugin_list 
		build_items.push(x)
	end
end

if build_items.empty? then
	build_items.push($plugin_list)
	build_items.push("pcsx2")
end

build_items.flatten!

build_items.each do |x|
	build(x,make_parameter)
end

print "\n--\n"
print "Build Summary:\n"
print $build_report 
print "\n"
print "#{$build_counter}/#{build_items.count} Successful.\n"