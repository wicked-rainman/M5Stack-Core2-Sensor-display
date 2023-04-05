# M5Stack-Core2 UDP Sensor display
M5Stack Core2 UDP connected sensor display

This code drives an M5Stack Core2 display.
It's connected to a WiFi network that remote sensors broadcast on, using UDP port 6124,
These sensors also output diagnostic messages (such as WiFi signal strength,OTP status Etc) on UDP port 6125
A Time Atomic International (GPS) DTG is broadcast on UDP port 6123
Locally connected sensors can be added to the I2C port for indoor sensors. These are enumerated and any provided drivers invoked.

These three broadcasts are read and the payload output to the built in display, along with the Core2 current battery state.
<p align="center">
<img src="./images/Core2.jpg" width="700" height="800">
</p
