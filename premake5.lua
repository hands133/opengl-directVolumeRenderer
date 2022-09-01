workspace "tinyVR"
    architecture "x64"
    startproject "App"

    configurations
    {
        "Debug",
        "Release"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["glfw"] = "tinyVR/vendor/glfw/include"
IncludeDir["glad"] = "tinyVR/vendor/glad/include"
IncludeDir["imgui"] = "tinyVR/vendor/imgui"
IncludeDir["glm"] = "tinyVR/vendor/glm"
IncludeDir["assimp"] = "tinyVR/vendor/assimp/include"

group "Dependencies"
    include "tinyVR/vendor/glfw"
    include "tinyVR/vendor/glad"
    include "tinyVR/vendor/imgui"
group ""

-- project tinyVR, core part of the renderer
project "tinyVR"
    location "tinyVR"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "vrpch.h"
	pchsource "tinyVR/src/vrpch.cpp"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/vendor/glm/glm/**.hpp",
        "%{prj.name}/vendor/glm/glm/**.inl"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS"
    }

    
    includedirs
    {
        "%{prj.name}/src",
        "%{prj.name}/vendor/spdlog/include",
        "%{IncludeDir.glfw}",
        "%{IncludeDir.glad}",
        "%{IncludeDir.imgui}",
        "%{IncludeDir.glm}"
    }

    links
    {
        "glfw",
        "glad",
        "imgui",
        "MSVCRTD.lib",
        "opengl32.lib"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "TINYVR_PLATFORM_WINDOWS",
            "TINYVR_BUILD_DLL",
            -- avoid  collision of glfw and glad
            "GLFW_INCLUDE_NONE"
        }

    filter "configurations:Debug"
        defines "TINYVR_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "TINYVR_RELEASE"
        runtime "Release"
        optimize "on"

-- project app, refer to tinyVR
project "App"
    location "App"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/resources/shaders/**.glsl"
    }

    includedirs
    {
        "tinyVR/vendor/spdlog/include",
        "tinyVR/src",
        "tinyVR/vendor",
        "%{IncludeDir.glm}"
    }

    links
    {
        "tinyVR"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "TINYVR_PLATFORM_WINDOWS"
        }
    
    filter "configurations:Debug"
        defines "TINYVR_DEBUG"
        runtime "Debug"
        symbols "on"
        ignoredefaultlibraries { "libcmtd.lib" }
        
    filter "configurations:Release"
        defines "TINYVR_RELEASE"
        runtime "Release"
        optimize "on"
        -- ignoredefaultlibraries { "libcmtd.lib" }


-- project app, refer to tinyVR
project "IOT"
    location "IOT"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/resources/shaders/**.vs",
        "%{prj.name}/resources/shaders/**.fs",
        "%{prj.name}/resources/models/obj/*.obj"
    }

    includedirs
    {
        "tinyVR/vendor/spdlog/include",
        "tinyVR/src",
        "tinyVR/vendor",
        "%{IncludeDir.glfw}",
        "%{IncludeDir.glad}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.assimp}"
    }

    libdirs
    {
        "%{IncludeDir.assimp}/../lib"
    }

    links
    {
        "tinyVR",
        "assimp-vc142-mt.lib"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "TINYVR_PLATFORM_WINDOWS"
        }
    
    filter "configurations:Debug"
        defines "TINYVR_DEBUG"
        runtime "Debug"
        symbols "on"
        ignoredefaultlibraries { "libcmtd.lib" }
        
    filter "configurations:Release"
        defines "TINYVR_RELEASE"
        runtime "Release"
        optimize "on"
        -- ignoredefaultlibraries { "libcmtd.lib" }

        
-- project app, refer to tinyVR
-- project "GMM_GPU"
--     location "GMM_GPU"
--     kind "ConsoleApp"
--     language "C++"
--     cppdialect "C++17"
--     staticruntime "on"

--     targetdir ("bin/" .. outputdir .. "/%{prj.name}")
--     objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

--     files
--     {
--         "%{prj.name}/src/**.h",
--         "%{prj.name}/src/**.cpp",
--         "%{prj.name}/resources/shaders/**.glsl"
--     }

--     includedirs
--     {
--         "tinyVR/vendor/spdlog/include",
--         "tinyVR/src",
--         "tinyVR/vendor",
--         "%{IncludeDir.glm}"
--     }

--     links
--     {
--         "tinyVR"
--     }

--     filter "system:windows"
--         systemversion "latest"

--         defines
--         {
--             "TINYVR_PLATFORM_WINDOWS"
--         }

--     filter "configurations:Debug"
--         defines "TINYVR_DEBUG"
--         runtime "Debug"
--         symbols "on"
--         ignoredefaultlibraries { "libcmtd.lib" }
        
--     filter "configurations:Release"
--         defines "TINYVR_RELEASE"
--         runtime "Release"
        -- optimize "on"
        -- ignoredefaultlibraries { "libcmtd.lib" }

