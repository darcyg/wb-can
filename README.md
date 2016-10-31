# CANbus-MQTT bridge

## Summary
Service which enables communucation for [HomA](https://github.com/binarybucks/homA) based controllers with CAN bus devices.
It's originally designed to be used with [Wiren Board](contactless.ru) smart home, and uses [extended MQTT conventions](https://github.com/contactless/homeui/blob/contactless/conventions.md).

Now bridge supports [Hapcan](hapcan.com) relays only, but can be extended easily to all Hapcan units and maybe some other CANbus devices. 
(It's only matter of bus communication dumps and API, 'cause I don't have any devices except those relays).

## Usage
Target build with [wbdev](https://github.com/contactless/wirenboard) `wbdev make`. 
For tests run `make ifup` (from root) for dummy interface and use candump, cansend.
 
Cli flags are:
* -p <port> - mqtt port (default: 1883)
* -h <host> - mqtt host (default: localhost)
* -v lvl - verbose level 0-4 (default: 0)

## Internals notes
* CAN bus doesn't have any scan ability, so in general some config needed. But not now - Hapcan supports scan frame. Service send scan frame on startup to obtain device list. Config may appear later (on demand).
* Bistable relay can save own On-Off state. The question is where is verified state while unsync (on power loss for example). Now it is on relay side.

