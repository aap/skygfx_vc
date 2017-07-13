workspace "skygfx_vc"
   configurations { "Release", "DebugIII", "DebugVC" }
   location "build"
   
   files { "resources/*.*" }
   files { "shaders/*.*" }
   files { "src/*.*" }
   
   includedirs { "resources" }
   includedirs { "shaders" }
   includedirs { "src" }
   includedirs { os.getenv("RWSDK34") }
   
   includedirs { "../rwd3d9/source" }
   libdirs { "../rwd3d9/libs" }
   links { "rwd3d9.lib" }
   
   prebuildcommands {
	"for /R \"../shaders/ps/\" %%f in (*.hlsl) do \"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T ps_2_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f",
	"for /R \"../shaders/vs/\" %%f in (*.hlsl) do \"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T vs_2_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f",
   }
	  
project "skygfx_vc"
   kind "SharedLib"
   language "C++"
   targetname "skygfx"
   targetdir "bin/%{cfg.buildcfg}"
   targetextension ".asi"
   characterset ("MBCS")

   filter "configurations:DebugIII"
      defines { "DEBUG" }
      symbols "On"
      debugdir "C:/Users/aap/games/gta3"
      debugcommand "C:/Users/aap/games/gta3/gta3.exe"
      postbuildcommands "copy /y \"$(TargetPath)\" \"C:\\Users\\aap\\games\\gta3\\scripts\skygfx.asi\""

   filter "configurations:DebugVC"
      defines { "DEBUG" }
      symbols "On"
      debugdir "C:/Users/aap/games/gtavc"
      debugcommand "C:/Users/aap/games/gtavc/gta_vc.exe"
      postbuildcommands "copy /y \"$(TargetPath)\" \"C:\\Users\\aap\\games\\gtavc\\scripts\skygfx.asi\""

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
	  flags { "StaticRuntime" }
