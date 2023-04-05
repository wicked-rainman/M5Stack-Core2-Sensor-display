# M5Stack-Core2 UDP Sensor display
M5Stack Core2 UDP connected sensor display

This code drives an M5Stack Core2 display.
It's connected to a WiFi network that remote sensors broadcast on, using UDP port 6124,
These sensors also output diagnostic messages (such as WiFi signal strength, battery status Etc) on UDP port 6125
A Time Atomic International (GPS) DTG is broadcast on UDP port 6123

These three broadcasts are read and the payload output on the built in display, along with the Core2 current battery state.

