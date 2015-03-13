TOXY - TuiO tcp proXY

# Introduction #

Toxy (TuiO proXY) is a simple server implementation based on liblo.
In simple words it forwards all messages from tcp to udp, as flash
is not capable of udp communication this is a helper app that adds that
possibility.

# Details #
For now it's used with nuigroup touchlib (AS3 code comes from there).
It sits in the tray (on windows) and changes the tray icon on the connection
status of the tuio ports (enabled/disabled/connected..). On linux and other
console fronteds it outputs everything on std-out.
For windows users (especialy touchlib) there is possibility to add your tuio
provider app (OSC.exe or anything else) in the config dialog, and afterwards start that
app from tray icon.
Benefit of this is that toxy eats the dos-console window so your
screen will be free of unwanted windows.

More about **TUIO**, _specs_ _,_ _details_ _and_ _software_, you can find on official page
http://www.tuio.org