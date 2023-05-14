local BUILD_DIR = path.join("build", _ACTION)

local VENDOR_DIR = "vendor/"

solution "openwarcraft3"
	location(BUILD_DIR)
	
	configurations { "Release", "Debug" }
	
	if os.is64bit() then
		platforms "x86_64"
	end

	filter "configurations:Release"
		optimize "Full"
	filter "configurations:Debug*"
		defines
		{
			"_DEBUG",
		}
		optimize "Debug"
		symbols "On"
	filter "platforms:x86"
		architecture "x86"
	filter "platforms:x86_64"
		architecture "x86_64"
	filter "system:macosx"
		defines { "OW3_PLATFORM_OSX" }

		platforms { "MacOSX-x64", "MacOSX-ARM" }
		xcodebuildsettings {
			["MACOSX_DEPLOYMENT_TARGET"] = "10.9",
			["ALWAYS_SEARCH_USER_PATHS"] = "YES", -- This is the minimum version of macos we'll be able to run on
		};

	filter {"system:windows"}
		defines { "OW3_PLATFORM_WINDOWS" }

	filter {"platforms:MacOSX-x64"}
		architecture "x64"

	filter {"platforms:MacOSX-ARM"}
		architecture "ARM"

project "openwarcraft3"
	kind "ConsoleApp"
	language "C"
	debugdir "data"
	exceptionhandling "Off"
	rtti "Off"
	editandcontinue "Off"
	defines {
        "GL_GLEXT_PROTOTYPES"
    }
	files {"src/**.c", "src/**.h", "src/**.inl", "premake5.lua", "README.md"}

	includedirs
	{
		"src",
		path.join(VENDOR_DIR, "stormlib/src"),
        path.join(VENDOR_DIR, "sdl/include"),
        "/opt/homebrew/Cellar/jpeg/9e/include",
	}
	filter "system:windows"
		links { "gdi32", "kernel32", "psapi" }
	filter "system:linux"
		links { "dl", "GL", "pthread", "X11" }
	filter "system:macosx"
		links { "QuartzCore.framework", "Metal.framework", "Cocoa.framework", "IOKit.framework", "CoreVideo.framework" }
		linkoptions { 
			"-lc++",
		}