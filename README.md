# lapRF-Gateway

This tool to use 'LapRF Personal' on Ethernet connection to LiveTime. (Like LapRF 8-Way)
This program works on raspberrypi. 
(May be works for other Linux board or PC)

```
+----------------+           +-----------------+                +------------+
| lapRF Personal |--- USB ---|   RaspberryPi   |--- Ethernet ---| WindowsPC  |
|                |           | (LapRF Gateway) |                | (LiveTime) |
+----------------+           +-----------------+                +------------+
```
There is a need to specify the IP address in Raspberrypi and WindowsPC. The setting method is not described here.


## Build & Install
```
$ git clone .....
$ cd ....
$ make
$ sudo cp laprf /usr/local/bin/
```

## Exec
1. Run LiveTime and set raspberrypi IP address
1. Connect to Raspberrypi from Windows PC with ssh
2. Run laprf
```
$ laprf
```

## Debug Monitor
Can view debug message.
Connect to TCP port 5404. (ex. use telnet client)

## Thanks
Thanks to ImmersionRC and LapRFUtilities.
https://github.com/ImmersionRC/LapRFUtilities

