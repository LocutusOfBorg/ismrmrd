#    Script for installing the required dependencied for 
#    the ISMRM Raw Data Format on Windows.
#    
#    Prerequisites:
#        - Windows 7 (64-bit)
#        - Visual Studio (C/C++) installed
#

function download_file($url,$destination) {
    #Let's set up a webclient for all the files we have to download
    $client = New-Object System.Net.WebClient
    $client.DownloadFile($url,$destination)
}

function unzip($zipPath, $destination){
    $shell = new-object -com shell.application;
    $zip = $shell.NameSpace($zipPath);
    if ((Test-Path -path $destination) -ne $True)
    {
        New-Item $destination -type directory
    }
    foreach($item in $zip.items()){
        $shell.Namespace($destination).copyhere($item)
    }
}

function Set-VS-Environment () {
    $file = (Get-ChildItem Env:VS100COMNTOOLS).Value + "vsvars32.bat"
    $cmd = "`"$file`" & set"
    cmd /c $cmd | Foreach-Object {
       $p, $v = $_.split('=')
       Set-Item -path env:$p -value $v
   }
}

function add_path($pathname) {
    if ($env:path  -match [regex]::escape($pathname)) {
        Write-Host "Path $path already exists"
    } else {
        setx PATH "$env:path;$pathname" -m
    }
}

Write-Host "ISMRMRD Raw Data Format Dependencies Installation"

$library_location = "C:\MRILibraries"
$download_location = "C:\MRILibraries\downloads"


#Let's first check if we have the library folder and if not create it
if ((Test-Path -path $library_location) -ne $True)
{
    Write-Host "Library location: " $library_location " not found, creating"
    New-Item $library_location -type directory
}
else
{
    Write-Host "Library location: " $library_location " found."
}

#Now check if we have the library folder and if not create it
if ((Test-Path -path $download_location) -ne $True)
{
    Write-Host "Download location: " $download_location " not found, creating"
    New-Item $download_location -type directory
}
else
{
    Write-Host "Download location: " $download_location " found."
}

#Download and install CMAKE
download_file "http://www.cmake.org/files/v2.8/cmake-2.8.9-win32-x86.exe" ($download_location + "\cmake-2.8.9-win32-x86.exe")
& ($download_location + "\cmake-2.8.9-win32-x86.exe")

#Download and install Git
download_file "http://msysgit.googlecode.com/files/Git-1.7.11-preview20120710.exe" ($download_location + "\Git-1.7.11-preview20120710.exe")
& ($download_location + "\Git-1.7.11-preview20120710.exe")

#Download, unzip, and install HDF5
download_file "http://www.hdfgroup.org/ftp/HDF5/current/bin/windows/HDF5189-win64-vs10-shared.zip" ($download_location + "\HDF5189-win64-vs10-shared.zip")
unzip ($download_location + "\HDF5189-win64-vs10-shared.zip")  "$download_location\hdf5_binaries"
& "$download_location\hdf5_binaries\HDF5-1.8.9-win64.exe"

#Download, install HDFView
download_file "http://www.hdfgroup.org/ftp/HDF5/hdf-java/hdfview/hdfview_install_win64.exe" ($download_location + "\hdfview_install_win64.exe")
& ($download_location + "\hdfview_install_win64.exe")

#Download and install CodeSynthesis XSD
download_file "http://www.codesynthesis.com/download/xsd/3.3/windows/i686/xsd-3.3.msi" ($download_location + "\xsd-3.3.msi")
& ($download_location + "\xsd-3.3.msi")

#Download and install boost
download_file "http://boostpro.com/download/x64/boost_1_51_setup.exe" ($download_location + "\boost_1_51_setup.exe")
& ($download_location + "\boost_1_51_setup.exe")

#FFTW
download_file "ftp://ftp.fftw.org/pub/fftw/fftw-3.3.2-dll64.zip" ($download_location + "\fftw-3.3.2-dll64.zip")
Set-VS-Environment
unzip ($download_location + "\fftw-3.3.2-dll64.zip")  "$library_location\fftw3"
cd "$library_location\fftw3"
& lib "/machine:X64" "/def:libfftw3-3.def"
& lib "/machine:X64" "/def:libfftw3f-3.def"
& lib "/machine:X64" "/def:libfftw3l-3.def"


#Message reminding to set paths
Write-Host "Please ensure that paths to the following locations are in your PATH environment variable: "
Write-Host "    - Boost libraries    (typically C:\Program Files\boost\boost_1_51\lib)"
Write-Host "    - Code Synthesis XSD (typically C:\Program Files (x86)\CodeSynthesis XSD 3.3\bin\;C:\Program Files (x86)\CodeSynthesis XSD 3.3\bin64\)"
Write-Host "    - FFTW libraries     (typically C:\MRILibraries\fftw3)"
Write-Host "    - HDF5 libraries     (typically C:\Program Files\HDF Group\HDF5\1.8.9\bin)"
Write-Host "    - ISMRMRD            (typically C:\Program Files\ismrmrd\bin;C:\Program Files\ismrmrd\bin)"


#Now download and compile ISMRMRD
download_file "http://sourceforge.net/projects/ismrmrd/files/src/ismrmrd_latest.zip" ($download_location + "\ismrmrd_latest.zip")
unzip ($download_location + "\ismrmrd_latest.zip")  "$library_location\ismrmrd"
cd "$library_location\ismrmrd"
New-Item "build" -type directory
cd build
& cmake "-G" "Visual Studio 10 Win64" "-DBOOST_ROOT=C:/Program Files/boost/boost_1_51" "-DXERCESC_INCLUDE_DIR=C:/Program Files (x86)/CodeSynthesis XSD 3.3/include/xercesc" "-DXERCESC_LIBRARIES=C:/Program Files (x86)/CodeSynthesis XSD 3.3/lib64/vc-10.0/xerces-c_3.lib" "-DXSD_DIR=C:/Program Files (x86)/CodeSynthesis XSD 3.3" "-DFFTW3_INCLUDE_DIR=C:/MRILibraries/fftw3" "-DFFTW3F_LIBRARY=C:/MRILibraries/fftw3/libfftw3f-3.lib" "../" 
msbuild .\ISMRMRD.sln /p:Configuration=Release