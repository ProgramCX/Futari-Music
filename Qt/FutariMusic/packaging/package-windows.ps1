param(
    [string]$ClientVersion,
    [string]$ServerCompatVersion,
    [string]$QtRoot,
    [string]$MinGWRoot,
    [string]$CMakeExe,
    [string]$InnoCompiler,
    [int]$ParallelJobs = 4
)

$ErrorActionPreference = 'Stop'
$sourceDir = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$repoDir = (Resolve-Path (Join-Path $sourceDir '..\..')).Path
$cmakeLists = Get-Content (Join-Path $sourceDir 'CMakeLists.txt') -Raw

function Get-CMakeDefault([string]$Name) {
    $pattern = 'set\(' + [regex]::Escape($Name) + '\s+"([^"]+)"\s+CACHE'
    $match = [regex]::Match($cmakeLists, $pattern)
    if (-not $match.Success) { throw "Could not find the CMake default for $Name." }
    return $match.Groups[1].Value
}

function Get-CMakeCacheValue([string]$CachePath, [string]$Name) {
    if (-not (Test-Path -LiteralPath $CachePath)) { return $null }
    $match = [regex]::Match((Get-Content $CachePath -Raw), "(?m)^${Name}:[^=]+=([^\r\n]+)")
    if ($match.Success) { return $match.Groups[1].Value }
    return $null
}

if ([string]::IsNullOrWhiteSpace($ClientVersion)) { $ClientVersion = Get-CMakeDefault 'FUTARI_CLIENT_VERSION' }
if ([string]::IsNullOrWhiteSpace($ServerCompatVersion)) { $ServerCompatVersion = Get-CMakeDefault 'FUTARI_SERVER_COMPAT_VERSION' }
if ($ClientVersion -notmatch '^\d+\.\d+\.\d+(-[A-Za-z0-9.-]+)?$') {
    throw "Invalid client version: $ClientVersion"
}
if ($ServerCompatVersion -notmatch '^(\d+\.\d+)(?:\.\d+(?:\.[A-Za-z][A-Za-z0-9.]*)?)?$') {
    throw "Invalid server compatibility version: $ServerCompatVersion"
}
$serverLine = $Matches[1]

$cacheCandidates = @(
    (Join-Path $sourceDir 'build-verify-mingw\CMakeCache.txt'),
    (Join-Path $repoDir 'build\windows\cmake\CMakeCache.txt')
)
if ([string]::IsNullOrWhiteSpace($QtRoot)) { $QtRoot = $env:FUTARI_QT_ROOT }
if ([string]::IsNullOrWhiteSpace($MinGWRoot)) { $MinGWRoot = $env:FUTARI_MINGW_ROOT }
foreach ($cachePath in $cacheCandidates) {
    if ([string]::IsNullOrWhiteSpace($QtRoot)) {
        $qt6Dir = Get-CMakeCacheValue $cachePath 'Qt6_DIR'
        if ($qt6Dir) { $QtRoot = Split-Path (Split-Path (Split-Path $qt6Dir -Parent) -Parent) -Parent }
    }
    if ([string]::IsNullOrWhiteSpace($MinGWRoot)) {
        $compiler = Get-CMakeCacheValue $cachePath 'CMAKE_CXX_COMPILER'
        if ($compiler) { $MinGWRoot = Split-Path (Split-Path $compiler -Parent) -Parent }
    }
}
if ([string]::IsNullOrWhiteSpace($QtRoot) -or -not (Test-Path (Join-Path $QtRoot 'bin\windeployqt.exe'))) {
    throw 'Set -QtRoot or FUTARI_QT_ROOT to the Qt MinGW installation (the folder containing bin\windeployqt.exe).'
}
if ([string]::IsNullOrWhiteSpace($MinGWRoot) -or -not (Test-Path (Join-Path $MinGWRoot 'bin\g++.exe'))) {
    throw 'Set -MinGWRoot or FUTARI_MINGW_ROOT to the Qt MinGW compiler installation.'
}

if ([string]::IsNullOrWhiteSpace($CMakeExe)) {
    $cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($cmakeCommand) { $CMakeExe = $cmakeCommand.Source }
}
if ([string]::IsNullOrWhiteSpace($CMakeExe)) {
    $knownCMake = 'D:\Qt\Tools\CMake_64\bin\cmake.exe'
    if (Test-Path -LiteralPath $knownCMake) { $CMakeExe = $knownCMake }
}
if ([string]::IsNullOrWhiteSpace($CMakeExe) -or -not (Test-Path -LiteralPath $CMakeExe)) {
    throw 'CMake was not found. Add it to PATH or pass -CMakeExe.'
}
$cmake = $CMakeExe
if ([string]::IsNullOrWhiteSpace($InnoCompiler)) {
    $innoCommand = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($innoCommand) { $InnoCompiler = $innoCommand.Source }
}
if ([string]::IsNullOrWhiteSpace($InnoCompiler)) {
    throw 'Inno Setup compiler ISCC.exe was not found. Install Inno Setup or pass -InnoCompiler.'
}

$buildRoot = Join-Path $repoDir 'build\windows'
$cmakeBuild = Join-Path $buildRoot 'cmake'
$stageDir = Join-Path $buildRoot 'stage'
$outputDir = Join-Path $repoDir 'build'
$outputBaseName = "FutariMusic-$serverLine-$ClientVersion-windows-x64"
$executable = Join-Path $cmakeBuild 'bin\appFutariMusic.exe'
$windeployqt = Join-Path $QtRoot 'bin\windeployqt.exe'
$innoScript = Join-Path $sourceDir 'packaging\windows\FutariMusic.iss'

New-Item -ItemType Directory -Force -Path $buildRoot, $outputDir | Out-Null
$env:Path = "$(Join-Path $MinGWRoot 'bin');$(Join-Path $QtRoot 'bin');$env:Path"
& $cmake -S $sourceDir -B $cmakeBuild -G 'MinGW Makefiles' `
    "-DCMAKE_PREFIX_PATH=$QtRoot" `
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$cmakeBuild/bin" `
    "-DFUTARI_CLIENT_VERSION=$ClientVersion" `
    "-DFUTARI_SERVER_COMPAT_VERSION=$ServerCompatVersion" `
    '-DFUTARI_WINDOWS_INSTALL_ARGUMENTS=/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-' `
    '-DCMAKE_BUILD_TYPE=Release'
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }

& $cmake --build $cmakeBuild --config Release --parallel $ParallelJobs
if ($LASTEXITCODE -ne 0) { throw 'Windows client build failed.' }
if (-not (Test-Path -LiteralPath $executable)) { throw "Built executable not found: $executable" }

if (Test-Path -LiteralPath $stageDir) {
    $resolvedStage = [System.IO.Path]::GetFullPath($stageDir)
    $resolvedBuildRoot = [System.IO.Path]::GetFullPath($buildRoot).TrimEnd('\') + '\'
    if (-not $resolvedStage.StartsWith($resolvedBuildRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a staging directory outside build/: $resolvedStage"
    }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
Copy-Item -LiteralPath $executable -Destination $stageDir
& $windeployqt --release --compiler-runtime --no-translations --qmldir (Join-Path $sourceDir 'qml') `
    (Join-Path $stageDir 'appFutariMusic.exe')
if ($LASTEXITCODE -ne 0) { throw 'Qt runtime deployment failed.' }

$env:FUTARI_STAGE_DIR = $stageDir
$env:FUTARI_OUTPUT_DIR = $outputDir
$env:FUTARI_OUTPUT_NAME = $outputBaseName
$env:FUTARI_CLIENT_VERSION = $ClientVersion
& $InnoCompiler $innoScript
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup packaging failed.' }

$packagePath = Join-Path $outputDir "$outputBaseName.exe"
if (-not (Test-Path -LiteralPath $packagePath)) { throw "Installer was not created: $packagePath" }
Write-Output "Created $packagePath"
