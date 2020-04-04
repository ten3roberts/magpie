tests = { "tests/main.c", "tests/overflow.c" }

function gen_tests()
	for k, v in pairs(tests) do
		b, e = v:find("/[a-z]+");
		b=b+1;
		name = v:sub(b,e);
		print ("generating test", name)
		project ("test_" .. name)
			kind "ConsoleApp"
			language "C"
			targetdir "bin"
			debugdir "tests"

			includedirs "./"
			links "magpie"
			files (v)
			defines "MP_REPLACE_STD"
			links { "m" }

			filter "configurations:Debug"
				defines { "DEBUG=1", "RELEASE=0" }
				optimize "off"
				symbols "on"
	
			 filter "configurations:Release"
				defines { "DEBUG=0", "RELEASE=1" }
				optimize "on"
				symbols "off"

			buildoptions "-Wall"
	end
end

newoption {
	trigger = "test",
	description = "Build the tests",
}

workspace "magpie"
	configurations { "Release", "Debug" }

if _OPTIONS["test"] then  gen_tests() end

project "magpie"
	kind "StaticLib"
	language "C"
	targetdir "bin"
	debugdir "tests"
	
	files { "src/**.h", "src/**.c" }

	-- For posix compliant systems
	filter "system:linux or bsd or hurd or aix or solaris or haiku or macosx"
		defines { "JSON_USE_POSIX" }
		links { "m" }
	
	-- For windows
	filter "system:windows"
		defines { "JSON_USE_WINAPI" }

	filter "configurations:Debug"
		defines { "DEBUG=1", "RELEASE=0" }
		optimize "off"
		symbols "on"

 	filter "configurations:Release"
		defines { "DEBUG=0", "RELEASE=1" }
		optimize "on"
		symbols "off"

	buildoptions "-Wall"
