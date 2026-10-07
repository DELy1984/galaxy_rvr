$ErrorActionPreference = 'Stop'
$vcvars = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path -LiteralPath $vcvars)) { throw "Native test compiler setup not found: $vcvars" }
$out = Join-Path $PSScriptRoot '.build\native-tests'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$test = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\tests\test_r3_command_guard.cpp'))
$command = "call `"$vcvars`" >nul && cd /d `"$out`" && cl /nologo /std:c++14 /W4 /WX /EHsc `"$test`" /Fe:r3-guard-tests.exe && r3-guard-tests.exe"
& $env:ComSpec /c $command
if ($LASTEXITCODE -ne 0) { throw "R3 native tests failed: $LASTEXITCODE" }
