Concerning the Visual Studio Solution + Project files:

This example directory includes Visual Studio project files that 
     target x86 (32bit) Windows.

If you are building for x86_64 (aka x64, 64bit), then you will 
   need to change the 'Platform' type in the project configuration.  

To do this in Visual Studio 2010,

1) open the example solution file (we will use examples\hello_cpp.sln), 
   'upgrade' if required
2) right click on the top-level solution (hello_cpp), select 'Properties'
3) Select "Configuration Properties-> Configuration" in the 
   "Property Pages" dialog
4) Click the "Configuration Manager..." button in the top right corner 
   of the dialog
5) on the line for 'hello_pub', click on the drop-down arrow under the 
   "Platform" column (it will initially show 'Win32')
6) Select "<New...>" from the list -- this will open a 'New Project Platform'
    dialog box
7) Select 'x64' in the 'New Platform' menu, un-check "Create new solution
    platforms"
8) Click "OK"
9) Back at the 'Configuration Manager' dialog box, repeat this for the 
   'hello_sub' line
10) Click "Close" and confirm save changes
11) Click "OK" on the 'hello_cpp' Property Pages

'Clean' the project(s) to make sure any old (32bit) obj files are removed
'Build' should work OK now for 64bit target.

