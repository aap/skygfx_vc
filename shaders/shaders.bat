for /R "./ps/" %%f in (*.hlsl) do "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_2_0 /nologo /E main /Fo ../resources/release/%%~nf.cso %%f
for /R "./vs/" %%f in (*.hlsl) do "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_2_0 /nologo /E main /Fo ../resources/release/%%~nf.cso %%f

"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_2_0 /nologo /E mainVS /Fo ../resources/release/vehiclePass1VS.cso ./pvs/vehiclePass2VS.hlsl
"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_2_0 /nologo /E mainVS /Fo ../resources/release/vehiclePass2VS.cso ./pvs/vehiclePass2VS.hlsl