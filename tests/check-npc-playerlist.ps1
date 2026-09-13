param(
    [string]$ProtocolInclude,
    [string]$ProtocolLibrary,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $ProtocolInclude) { $ProtocolInclude = Join-Path $projectRoot '../BedrockProtocol-main/install/include' }
if (-not $ProtocolLibrary) { $ProtocolLibrary = Join-Path $projectRoot '../BedrockProtocol-main/install/lib/Protocol.lib' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $projectRoot 'work/npc-wire-check' }
$ProtocolInclude = (Resolve-Path -LiteralPath $ProtocolInclude).Path
$ProtocolLibrary = (Resolve-Path -LiteralPath $ProtocolLibrary).Path
$compiler = (Get-Command cl.exe -ErrorAction Stop).Source
$python = (Get-Command python.exe -ErrorAction Stop).Source
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$executable = Join-Path $OutputDirectory 'npc_player_list_wire.exe'
$object = Join-Path $OutputDirectory 'npc_player_list_wire.obj'
$source = Join-Path $PSScriptRoot 'npc_player_list_wire.cpp'
& $compiler /nologo /std:c++latest /EHsc /MD /utf-8 /W4 "/I$ProtocolInclude" "/I$(Join-Path $projectRoot 'src')" $source "/Fe:$executable" "/Fo:$object" /link $ProtocolLibrary
if ($LASTEXITCODE -ne 0) { throw 'C++ wire fixture compilation failed.' }
& $executable $OutputDirectory
if ($LASTEXITCODE -ne 0) { throw 'C++ wire fixture failed.' }
& $python (Join-Path $PSScriptRoot 'check_npc_player_list.py') $OutputDirectory
if ($LASTEXITCODE -ne 0) { throw 'Independent 2168 decode failed.' }
Write-Output "Protocol.lib SHA256: $((Get-FileHash -LiteralPath $ProtocolLibrary -Algorithm SHA256).Hash)"
