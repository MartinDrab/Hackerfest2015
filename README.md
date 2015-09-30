# Hackerfest2015
Demos presented on Hackerfest 2015

* No driver files are digitally signed with trusted certificate so you either need to sign them by yourself, or sign them by a test signing certificate and configure your system to support this feature.

* Rename dllreghider.dll to reghider.dll and the RHGUI.exe should work.

* You should follow special instructions in order to install the ndisprot6 driver. Look into the ndisprot6\ndisprot.htm file for them.
* After you install the driver, you also must update the ImageFIleName value in its service registry key to point to the ndisprot6.sys file, not the ndisprot.sys one (because it does not exist). I am probably missing something in the INF file.
