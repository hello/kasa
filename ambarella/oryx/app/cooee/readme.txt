* About this demo
It demos how to start/stop Broadcom easy setup protocols, including 3rd-party ones such as airkiss.


* Usage on Linux
Usage: (type setup -h)
# setup -h
-h: show help message
-d: show debug message
-k <v>: set 16-char key for all protocols
-p <v>: bitmask of protocols to enable
  0x0001 - cooee
  0x0002 - neeze
  0x0004 - akiss
  0x0010 - changhong

To start cooee+neeze (also works for qqconnect):
# setup -p 3

To start cooee+akiss (also works for qqconnect):
# setup -p 7

A sample output:
# setup -p 1
state: 0 --> 1
state: 1 --> 2
state: 2 --> 3
state: 3 --> 5
ssid: Broadcom2g
password: 12345678
sender ip: 192.168.43.149
sender port: 0
security: wpa2


* Usage on WICED
Copy all files of jni/* to <WICED_ROOT>/apps/snip/setup/, then you can build it with:
$ make snip.setup-xxxxx

You should carefully check which protocols to enable in application_start() in main_wiced.c

When application is started, you will get following message on WICED console after
successfully configured:

WWD SDIO interface initialised
WLAN MAC Address : 04:E6:76:61:95:F0
WLAN Firmware    : wl0: May 12 2015 09:26:33 version 5.90.195.r2 (TOB) FWID 01-5de4b5f9 es3.c2.n1.ch1
Enabled, waiting ...
state [1]
state [2]
state [2]
state [2]
state [2]
state [2]
state [2]
state [2]
state [3]
state [3]

Event Found => Success
SSID        : BRCMGUEST
PASSWORD    : password1234
BSSID       : b8:62:1f:7c:63:22
AP Channel  : 6
AP Security : WPA2-PSK AES
Storing received credentials in DCT
easy setup done.
Joining : BRCMGUEST
Successfully joined : BRCMGUEST
Obtaining IPv4 address via DHCP
IPv4 network ready IP: 10.148.8.106
Network up success


* About API
You can enable various protocols with following APIs before calling easy_setup_start():
easy_setup_enable_cooee();
easy_setup_enable_neeze();
easy_setup_enable_akiss();
easy_setup_enable_changhong();

or just with one call:
easy_setup_enable_protocols(uint16 proto_mask);

where proto_mask is bitmask of various protocols.

For example, to start cooee+akiss:
easy_setup_enable_cooee(); /* also works for qqconnect */
easy_setup_enable_neeze(); /* included in new qqconnect */
easy_setup_enable_akiss();
easy_setup_start();
/* get ssid/password */
// easy_setup_get_xxx();
easy_setup_stop();

if needed, you can set decryption key for various protocols:
cooee_set_key("0123456789abcdef"); /* cooee decryption key */
akiss_set_key("fedcba9876543210"); /* set akiss decryption key */

With received ssid/password, it's time to feed it to wpa_supplicant:

#!/system/bin/sh

id=$(wpa_cli -i wlan0 add_network)
wpa_cli -i wlan0 set_network $id ssid \"Broadcom2g\"
wpa_cli -i wlan0 set_network $id psk \"12345678\"
wpa_cli -i wlan0 set_network $id key_mgmt WPA-PSK
wpa_cli -i wlan0 set_network $id priority 0
wpa_cli -i wlan0 enable_network $id
wpa_cli -i wlan0 save_config

or you can write one entry to wpa_supplicant.conf and reload it:
network={
    ssid="Broadcom2g"
    psk="12345678"
    key_mgmt=WPA-PSK
}


* About COOEE and QQ connect
In sender side, both Broadcom demo and QQ have enabled sending cooee+neeze, 
with different keys to encrypt to payload:

(Broadcom demo)
  <-- (enc(cooee_key, cooee_payload+neeze_payload)) --

(QQ)
  <-- (enc(cooee_key_qqcon, cooee_payload+neeze_payload)) --

With following setting in device, it's ready to receive both cooee demo and QQ:
easy_setup_enable_cooee();
easy_setup_enable_neeze();
cooee_set_key(cooee_key); // if you have set cooee key in sender
cooee_set_key_qqcon(cooee_key_qqcon); // key for qqcon
neeze_set_key(cooee_key); // the same as cooee key, if you have set it
neeze_set_key_qqcon(cooee_key_qqcon); // key for qqcon


* About changhong
You can get changhong sec_mode byte with following API:
int changhong_get_sec_mode(&mode);
