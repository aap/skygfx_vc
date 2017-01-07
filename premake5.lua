workspace "skygfx_vc"
   configurations { "Release", "Debug" }
   location "build"
   
   files { "external/*.*" }
   files { "resources/*.*" }
   files { "shaders/*.*" }
   files { "source/*.*" }
   
   includedirs { "external" }
   includedirs { "resources" }
   includedirs { "shaders" }
   includedirs { "source" }
   includedirs { os.getenv("RWSDK34") }
   
   includedirs { "external/rwd3d9/source" }
   libdirs { "external/rwd3d9/libs" }
   links { "rwd3d9.lib" }
   
   prebuildcommands {
	"for /R \"../shaders/ps/\" %%f in (*.hlsl) do \"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T ps_2_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f",
	"for /R \"../shaders/vs/\" %%f in (*.hlsl) do \"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T vs_2_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f",
	"\"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T vs_2_0 /nologo /E mainVS /Fo ../resources/cso/vehiclePass1VS.cso ../shaders/pvs/vehiclePass1VS.hlsl",
	"\"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T vs_2_0 /nologo /E mainVS /Fo ../resources/cso/vehiclePass2VS.cso ../shaders/pvs/vehiclePass2VS.hlsl"
   }
	  
project "skygfx_vc"
   kind "SharedLib"
   language "C++"
   targetname "skygfx"
   targetdir "bin/%{cfg.buildcfg}"
   targetextension ".asi"
   characterset ("MBCS")

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
	  flags { "StaticRuntime" }
	  