<!--* getting_started.md
 *
 * History:
 *   2016-5-23 - [ghzheng] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *-->
![Oryx logo](img/Oryx-Logo.jpg)
#Getting started

##Preparation

There are 3 kinds of things you should prepare:

* [Set up your building machine](#Set up your building machine): You can build codes, download binaries and do other developing things under this evironment.
* [Install Toolchain and AmbaUSB](#Install Toolchain and AmbaUSB): A tool set for building codes and a tool for downloading binaries to the EVK board.
* [Prepare a Ambarella EVK board](#Prepare a Ambarella EVK board): For example, Antman.

##Building and downloading

If you have prepared all things refered at last chapter, you can build the codes int the next step. We will take **S3L SDK** and **Antman** with ov2718 sensor Board for an example.

### Extract SDK package and build it.

Following the commands below:

```shell
 # tar xJvf s3l_linux_sdk_2016xxxx.tar.xz
 # cd s3l_linux_sdk/ambarella
 # source build/env/armv7ahf-linaro-gcc.env
 # cd boards/s3lm_antman/
 # make sync_build_mkcfg
 # make s3lm_2718_config
 # make defconfig_public_linux
 # make -j8
```

### Download firmware to board.

#### Sets on Serial terminal

##### Normal burning

In the serial console, press and hold the **Enter** key on the keyboard and then press the hardware **RESET** button. The development platform will switch to the AMBoot shell as shown in below. At last ,type the **u** to launch the mode of loading.

![image_loadshell](img/loadshell.png)

##### Burning in USB Boot Mode

If there is nothing to print on terminal, you need burn the board in USB Boot Mode. Toggle the USB switch on the debug board from 0 to 1 as shown below:


![image_hardburn](img/antman.JPG)

#### On PC side

Finish building, you will find the firmware: ```s3l_linux_sdk/ambarella/out/s3lm_antman/images/bst_bld_pba_kernel_lnx_release.bin```.
Following the steps shown in below:

**Step 1**: Launch AmbaUSB, select **S3L(H12)** for Boards Filter.

**Step 2**: Select **S3LM.DDR3.NAND** for the board's config menu, and the tool will auto detect the board.

**Step 3**: Select the binary from output location.

**Step 4**: Click the **blue** button.

![image_ambaUSBLoad](img/loadSDK.png)

```
----- Report -----
bst: success
bld: success
hal: success
pri: success
lnx: success
      - Program Terminated -
```

Finally, The serial console of the developnent platform will indicate the upgrading process. When the firmware is downloaded, the message "Program Terminated" will be displayed on the serial console. If users downloaded in USB Boot Mode, please toggle the USB switch back to 0. Refer to the information above.

>Note that there are minor differences in steps for choosing Chip Filter and Board's Config. Users should choose related sets based on chips.(up steps just an example for antman)



##Getting it run

By operations above, users now can play live videos and audios through few simple procedures below:

Firstly, run the command of **apps_launcher** to start the service needed in the **Serial terminal**.

![image_laucher](img/laucher.png)

>The apps_launcher is designed to be a “programmer launcher tool” to launch all related “Air applications”
together and the parent process of all services. It manages the processes of img_svc, media_svc, video_svc, audio_svc, event_svc, net_svc and sys_svc.

Secondly, launch the VLC and open the media in the address of ``` rtsp://10.0.0.2/video=video0,audio=acc```

![imag_vlc](img/vlc.png)

Congratulations! Now you can play videos and make further progress.

## An example of customising your recording

The SDK allows users to customise a variety of configurations in different ways. The main procedures of configuration are shown in below:


![image_path](img/architecture.png)

The mechanisms of making coustomised functional configurations are separeted in two ways:

- Reload configs and restart services after editting config files such as source_buffer.acs, stream_fmt.acs, stream_cfg.acs and so on;
- Real-time control through cmds such as test_video_service_dyn_air_api and so on.


In this chapter, we will show an example of customising configuration items. The processes are shown in below:

![image_struct_path](img/example_struct.png)

There are 3 ways to set configuration items into config files:

* **use AIR api cmds to set**
* **set configuration from web**
* **directly modify the config files**

We advise users use Air api commands or webpage to set configurations for decreasing risks of system errors. Before using these two ways to set, please run **apps_launcher** first.

### Set Buffers configs

Use the command below to set buffers items:

```shell
# test_video_service_cfg_air_api -b 0 -t ENCODE -W 1920 -H 1080 -b 1 -t ENCODE -W 720 -H 480 -b 2 -t ENCODE -W 1280 -H 720
```

```shell
-b: Specify the buffer ID.
-t: The type of source buffer OFF|ENCODE|PREVIEW
-W: Source buffer width
-H: Source buffer height
```

> Note: All commands have the help text for typing: `test_vide_service_cfg_air_api --help`.

Using command to change configuration has the same effects as editing the config file of **`/etc/oryx/video/source_buffer.acs`**.

![image_buffers_path](img/buffer.png)

> The config files of buffers and streams are in directory of `/etc/oryx/video/`. Users may check it for all config files. In this chapter, we just expand what is refered.

### Set Streams configs

Use the command below to set streams items:


```shell
#test_video_service_cfg_air_api -s 0 -t H265 -W 1920 -H 1080 -s 1 -t H264 -W 720 -H 480 -s 2 -t MJPEG -W 1280 -H 720
```

```shell
-s: Specify the stream ID
-t: The type of encode
-W: Stream width
-H: Stream height
```

Using command to change configuration has the same effects as editing the config file of **`/etc/oryx/video/stream_fmt.acs`**.

![image_stream](img/stream_fmt.png)

### Set mp4 recording configs

For recording mp4 files,users should set configuration items along **engine**->**filter**->**muxer**.

Firstly, edit **engine** configuration items in **`/etc/orxy/stream/engine/record-engine.acs`**, shown in below:

![image_engine](img/engine.png)

> Notes: The function of recording can be only set with editing config files.

then, edit **filter** configuration items in **`/etc/oryx/stream/filter/filter-file-muxer.acs`**, shown in below:

![image_filter](img/filter_file.png)

choose **"mp4","mp4-event0"** to **media_type**

Furthermore, edit **muxer** configuration items in **`/etc/oryx/stream/muxer/muxer-mp4.acs`**, shown in below:

![image_muxer_mp4](img/muxer-mp4.png)

Specify the location of storage in the item of **file_location**. If set the flag of **file_location_auto_parse** to **ture**, it will specify storage location by parsing devices of USB and SD card. We choose default video ID 0 and other configuration items for recording in this example.

> Generally, the default path of USB device is /storage/sda and the default path of SD card is /sdcard/.

### Set mp4 event recording configs

We allow you to record a fragment of video, start time is n minutes before users' toggle time and stop time in m minutes after toggle time. We need to edit other config files based on last steps.
Users should edit **`/etc/oryx/stream/filter/filter-avqueue.acs`** to enable event recording. For setting two event recording of video streams, set the **event0** and **event1** to **true**.

![image_avqueue_path](img/avqueue.png)

**history_duration**: the duration before users' trigger event time.

**future_duration**: the duration after uses' trigger event time.

Furthermore, edit **`/etc/oryx/stream/muxer/muxer-mp4-event0.acs`**, most of the items are same as muxer-mp4.acs. For detail configuration as below:

![image_mp4_event0_path](img/mp4-event0.png)



### Set mjpeg recording configs

For recording jpeg files, users should edit other two config files as **`/etc/oryx/stream/filter/filter-direct-muxer.acs`** and **`/etc/org/stream/muxer/muxer-jpeg.acs`**.

For **filter-direct-muxer.acs**:

![image_filter_direct_path](img/filter-direct-muxer.png)

For **muxer-jpeg.acs**:

![image_muxer_jpeg_path](img/muxer-jpeg.png)

### Set mjpeg event recording configs

For recording jpeg event files, users should edit another config file as **`/etc/oryx/stream/muxer/muxer-jpeg-event0.acs`** based on last chapter.

For **muxer-jpeg-event0.acs**:

![image_jpeg_event_path](img/muxer-jpeg-event.png)


### Check the functions

At last, run the cmds of **apps_launcher** to start the services. After that, you can try some cases to check the functions.

* Step 1: Check RTSP live streaming

    For playing video,open VLC with the address of `rtsp://10.0.0.2/video=video0,audio=acc`, also users could specify other video such as video1,video2 at this case.

* Step 2: Check mp4 file

    Type the commands of `test_media_service_air_api -d 1` to start file writing and `test_media_service_air_api -b 1` to stop file writing. If it works well, you can find mp4 files in the path of file_location(e.g.`/sdcard/video/1`).

    > test_media_service_air_api -d 1: the format is **test_media_service_air_api -d (1 << muxer_id)**, muxer_id can be found in /etc/oryx/stream/muxer/muxer-mp4.acs

* Step 3: Check mp4 event recording

    Type the command of `test_media_service_air_api -e h*0` to trigger the event. After that, you can find mp4 event files in the path of file_location(e.g.`/sdcard/video/event0`).

* Step 4: Check mjpeg recording

    Type the command of `test_media_service_air_api -d 256` to start jpeg file writing, `test_media_service_air_api -b 256` to stop jpeg file writing. If it works well, you can find jpeg files in the path of file_location(e.g.`/sdcard/video/jpeg`).

* Step 5: Check mjpeg event recording

    Type the command of `test_media_service_air_api -e m*0*0*0*3` to trigger jpeg event. After that, you can find jpeg files in the path of file_location(e.g.`/sdcard/video/jpeg/event0`).

* Step 6: Check data exporting

    Type the command of `test_oryx_data_export -f /sdcard/export/`. It will create with related files such as "export_audio_0.acc", "export_audio_1.opus", "export_video_0.h265" in the path of /storage/sda/export/.

### Use config files
If users are bored about config sets or something failure during above steps, the SDK also provides config files already configured above examples. Users could directly use them in the path of `SDK/oryx/config_set/config_samples/` by the name of examples.tar.xz.

Users should extract the config files by the command of `tar -Jxf examples.tar.xz`. Then, cover config files in the path of `/etc/oryx/` with  folds of `video` and `stream`. At last, run the service with `apps_launcher`.

### CGI
Oryx Midlleware support users interactively manipulate options through Web service.

Now we begin with easy guide for users to access related services.

Type `10.0.0.2/oryx` into the address of web-browser with both username and password of `admin`.
Until then, it will access the Web as shown in below:

![image_cgi_full](img/cgi_full.png)

Now, users can make customised configurations with specific demands. As a guide, we will show some details with examples shown in below:

* Start/Stop encode

* Make record

#### Start/Stop record

Choose the item of Video>>Encode, the web will go to web-site as shown in below:

![image_encode_path](img/cgi_encode.png)

Users can start/stop encode by triggering the button, also some parameters could be setted through the list.

#### Event record

Choose the item of Media>>Recording, the web will go to the location as shown in below:

![image_record_path](img/cgh_record.png)

**Step 1:** Choose the format of file

**Step 2:** Trigger the button of **Start**

Trigger button of stop when it finishes. Then, you can find video file on specifed directory.

Until now, users may customise video features.

#### Add overlay

Users could add overlays with customised design. The CGI allow users to add variety types of overlay, such as **String**, **Time**
, **Picture** and **Animation**. In this section we will give an example of how to add overlay of type of **String**.

**Step 1:** Area Manipulate

The overlay added based on mechanism of **Area** and **Data Block**. The system firstly divide the screen into different areas, users
could set areas' color as background color. Then users could set data blocks in the areas. The overlay is eventualy added in data block.
Firstly, user should get information of Overlay Parameters by the list button of **Get**, Check the areas already in screen, and make sure
the specified parameters. The related parameters are shown in below:

![image_cgi_paramters_path](img/CGI_overlay_parameters.png)

Based on the limitation of area number, we don't add area in this section. We add overlay in default area.


**Step 2:** Data Block Manipulate

Users colud simply set data block to add overlay, type the list button of Add, then set the parameters shown in below:

![image_cgi_block_path](img/CGI_overlay_block.png)

It's should be reminded that the size of data block should not exceed the area.

Based on parameters set above, users could get the overlay shown in below:

![image_cgi_show_overlay](img/CGI_overlay_show.png)
# Features on Oryx

## Set Feature of LBR

LBR means low bitrate, it allow users to use low bitrate video optimization algorithm developed by Ambarella.

When users want to enable the feature of LBR. Please type the command of `test_video_service_cfg_air_api -s 0 --feature-bitrate lbr`

```shell
-s : Specify stream 0
--feature-bitrate: Specify the type of bitrate control
```
To enable the configuration, type the reset command of `test_video_service_cfg_air_api --apply`

Using the commands above also has the same effect to edit the config file of `/etc/oryx/video/features.acs` as shown in below:

![image_feature_path](img/feature-lbr.png)


## Set Feature of EIS

## Set Feature of HDR

## IR Filter Set

Currently, it is supported IR filter control in related boards. User could use the commend of **test_image_service_air_api**
to set IR filter off and on.

To turn on, type the command **test_image_service_air_api --ir-led-mode 1**, if user want to turn on in night, should type
**test_image_service_air_api --ir-led-mode 1 --day-night-mode 1**.

To turn off, type the command **test_image_service_air_api --ir-led-mode 0**, if user want to turn off in day, should type
**test_image_service_air_api --ir-led-mode 0 --day-night-mode 0**.

The IR filter is actually controlled by GPIO output, so user could also set it by editting file.

The GPIO's number is defined in:

ambarella/boards/s3lm_antman/devices.h (take s3lm_antman for example, if you use s2lm_ironman, just replace the board's name)

Use antman as example:

```
/* IR CUT */
#define GPIO_ID_IR_CUT_CTRL 106

```

```shell
# echo 106 > /sys/class/gpio/export
# echo out > /sys/class/gpio/gpio106/direction
# echo 1 > /sys/class/gpio/gpio106/value
# echo 0 > /sys/class/gpio/gpio106/value
```

If the user finds the IR CUT switcher is on the wrong state(removed on day mode, covered on night mode), user needs to revert
the polarity of IR CUT switcher define in:

**ambarella/packages/img_mw/arch_s3l/include/mw_ir_led.h**

```C
typedef enum {
  IR_CUT_NIGHT = 0,
  IR_CUT_DAY = 1,
} ir_cut_state;

```

## Firmare Upgrade
The SDK allows users to upgrade firmware with specified commands.

Type the command of `test_system_service_air_api`, then the program will run to guide you to execute related procedures.

As an example, we show the details of upgrading firmware in bleow:

Step 1. Type `test_system_service_air_api`

It will run the program that guide users to set configuration items as shown in in below:

![image_upgrade](img/system-1.png)

Step 2. Choose the item of **upgrade settings** by typing number **6**, we go into the stage shown in below:

![image_ugrade](img/system-2.png)

Step 3. Choose the item of **set upgrade mode** by typing **m**, in this example, we choose mode 1 that is only for upgrading.

Step 4. Set the path of dst file by typing **p**, in this case, we have already mounted host share forld to development plantform. The full path of firmware is `/mnt/firmware/AmSDK.bin`.

![image_upgrade](img/system-3.png)

Step 5. Start to upgrade by typing **s**, then the system will begin to upgrade and reboot.

## DSP Mode switch and native video playback

We allow you to playback video files with board, display on connected video output port(maybe HDMI, CVBS, or Digital (LCD)).

Please also be noted that DSP would need do a mode switch between Encode mode and Decode mode. Typically would be ENCODE mode <--> IDLE mode <--> DECODE mode.

Type the commands of `test_video_service_dyn_air_api --pause` to pause all encoding related things. DSP also back to IDLE mode.

Type the commands of `test_video_service_dyn_air_api --resume` to resume all encoding related things. DSP also resume to ENCODE mode.

When DSP is in IDLE mode, it's ready to playback video files(.mp4)

Type the commands of `test_playback_service_air_api -p [filename] -V720p --hdmi` to play file, [filename] is the video file, '-V720p' specify the VOUT mode, '--hdmi' specify the VOUT device type. You may see all available VOUT mode and VOUT device types.

During playback, there're several runtime commands:

|Press Key | Playback function |
|:-------------|:-------|
|' ' + ENTER|Pause/Resume|
|'s' + ENTER|Step play|
|'f%d' + ENTER| %d x speed fast forward|
|'b%d' + ENTER| %d x speed fast backward|
|'F%d' + ENTER| %d x speed fast forward, from begin of file|
|'B%d' + ENTER| %d x speed fast backward, from end of file|
|'g%d' + ENTER| Seek to %d ms|
|'q' + ENTER| Quit playback|
|CTRL + 'c'| Quit playback|


Typical Playback command flow:


`test_video_service_dyn_air_api --pause`

`test_playback_service_air_api -p [filename] -V720p --hdmi`

runtime playback command, quit playback

`test_video_service_dyn_air_api --resume`


## External USB camera as EFM input

We allow you to connect an external USB camera, configure it as EFM source (secondary stream), then we have a simple dual camera solution.

USB camera may have two pixel format: YUYV(YUV422) or MJPEG, we support both of them, but please note that due to the USB transfer speed, YUYV USB camera may not achieve full FPS(24). MJPEG USB camera may achieve its full FPS(24). MJPEG camera would cost more CPU cycle due to SW MJPEG decoding.


To configure USB camera as EFM input in Oryx:

enable EFM in mode's limit configure file(for example, /etc/oryx/video/adv_hdr_mode_resource_limit.acs), change to 'enc_from_mem_possible    = true', as shown in below:

![image_efm_path](img/efm-limit.png)

Change EFM's buffer size to USB camera's resolution (640x480): change '/etc/oryx/video/source_buffer.acs' change efm buffer's size to 640x480, as shown in below:

![image_efm_path](img/efm-source.png)

Change secondary stream's source to EFM: change '/etc/oryx/video/stream_fmt.acs', change Stream B's 'source = 6, enable = true', as shown in below:

![image_efm_path](img/efm-stream.png)


Type the commands of `test_efm_src_service_air_api --feed-usbcam -B` to start USB camera's feeding to EFM (stream B). '-A' means stream A, '-B' means stream B.

To specify prefer format of USB camera, use '--capyuv' to select YUYV as preferred format, use '--capjpeg' to choose MJPEG as preferred format.

To specify prefer resolution of USB camera, use '-s %dx%d' to specify preferred resolution (width x height).

To specify prefer FPS of USB camera, use '-f %d' to specify preferred FPS.

For example, to specify USB camera's resolution to 640x480, 15 FPS, YUYV format, the command line is 'test_efm_src_service_air_api --feed-usbcam -B -s 640x480 -f 15 --capyuv'

Type the commands of `test_efm_src_service_air_api --end-feed` to end USB camera's feeding to EFM.

#Appendix
##<span id="set_machine">Set up your building machine</span>

### PC sets

Firstly, you should make sure you install the related softwares. The related softwares are needed configuration below:

|EVK Software | Denote |
|-------------|-------:|
|AmbaUSB      |for burning firmware vis USB|
|PuTTY        |as a terminal tool for console access|
|VLC media player| for video playback of RTSP stream|

#### Setting Up the Network

1. Change the IP Address of the PC network adaptor to the board to 10.0.0.1. Note that the defaut IP address
of the board is 10.0.0.2. The example of IP configuration On PC shows in below:

    ![image_network_path](img/network.png)

2. Use the following command to determine if the network is working.

    ```ping 10.0.0.2```

#### Connecting to a Console Window

The console window enables the users to view the system boot sequence messages and run the
demonstration applications from the Linux shell.

##### Console Window: Serial

Connect a serial cable between the serial port on the EVK and the serial port (for example, COM1) on the host
PC.
Open a terminal emulator (such as Minicom on Linux or PuTTY / HyperTerminal on Windows) on the host
PC. Ambarella recommends using PuTTY on Windows. PuTTY can be found in the EVK tools.
Configure the terminal emulator to connect to the serial port using the following data:

|Item     |Set        |
|---------|----------:|
|Speed    |115200     |
|Data Bits|8          |
|Parity   |None       |
|Stop Bits|1          |
|Flow control|None    |

**Using PuTTY as example(for Windows)**:

* From PuTTY Configuration dialog, choose Category > Connection > Serial and for Serial line to
Connect to the correct port (For example, COM1). Use the settings given below, for Speed,
Data Bits, Parity, Stop Bits, and Flow Control.

    ![putty_1_path](img/putty-1.png)

* Using the PuTTY Configuration dialog, choose Category > Session for connection type. Then,
choose Serial and set Serial Line to COM1, Speed to 115200, and save the current session (For
example, Amba Serial), so it can be reused.

    ![putty_2_path](img/putty-2.png)

* Click Open and the following console appears:

    ![putty_3_path](img/putty-3.png)

* Power on the EVK board, the console reads the boot sequence messages. After Linux boots up,
login as root.

    ![putty_4_path](img/putty-4.png)

**Using the Minicom as example(for ubuntu):**

1. Type **sudo minicom -s** in shell, choose the **Serial port setup** to config as blew:

    ![image_minicom](img/minicom.png)

2. Save and exit from current set.

3. Type **sudo minicom** to lauch the default Serial port.

##### Console Window: Telnet

Connect an Ethernet cable between the EVK and the host PC.
Open a terminal emulator on the host PC.Configure the terminal emulator to the Telnet connection type using:

* host Name (or IP address): 10.0.0.2 (default)
* port:                       23

Using PuTTY as an example:

1. From PuTTY Configuration window, choose Session. In the right panel, select Telnet as
Connection type. For Host Name (or IP address) enter 10.0.0.2, for Port enter 23. Save the
session and click Open.

    ![putty_telnet_path](img/putty_telnet.png)

##### Play videos with VLC

To view the H.264 streams via VLC, configure the VLC as show in below.
Launch VLC and choose Tools > Preferences.

1. Select **All** in Show setting at the bottom left of the Preferences dialog.
2. Click **Input/Codecs** to unfold the option.
3. Single click **Demuxer**. Do not expand the folder.
4. On the right panel, select **H264 video demuxer** for the Demux module.
5. Click **Save** to quit.

    ![VLC_path](img/VLC.png)


##<span id="">Install Toolchain and AmbaUSB</span>

The related software needed will be provided by Amba Ltd, such as **AmbaUSB** and **Linaro Toolchain**.
The Linaro toochain is comprised of robust, comercial-grade tools which are optimized for Cortex-A processors. Linaro tools, software and testing procedures are include in this latest release of the processor SDK.

The version used in the latest SDK is as follows:

**Linaro-GCC :5.3    2016.02**

when downloading the Toolchain package, choose the IA64 GNU/Linux TAR file:
For example, linaro-multilib-2015.02-gcc4.9-ix64.tar.xz
Install the Toolchain
(for ubuntu linux, the commnads are as follows)

```
build $ cd tools/ToolChain

build sudo chmod +x ubuntuToolChain

build sudo ./ubuntuToolChain
```


##<span id="">Prepare a Ambarella EVK board</span>

Before specified operations, users should connect cables from EVK boards to host PC firstly. As an example, the connection of Antman is shown below:

![image_ahead](img/ahead_ant.JPG)

![image_back](img/back_ant.JPG)

The middleware of Oryx can help customers quickly customize it for products.

For comprehensive showing the operations can be configed, list cmds and options below:

## List of services and commands

|Command|service|
|:------|:--|
|apps_launcher|the maneger of service|
|test_image_service_air_api|image service|
|test_media_service_air_api|media service|
|test_audio_service_air_api|audio service|
|test_playback_service_air_api|playback service|
|test_system_service_air_api|system service|
|test_video_service_cfg_air_api|video service|
|test_video_service_dyn_air_api|video service|
|test_sip_service_air_api|sip service|
|test_efm_src_service_air_api|efm source service|

# Oryx Middleware Framework
S2L/S2E/S3L serails camera middleware framework has a unique name called "Oryx".
Oryx stands for strong, tough, rubust, agile, and teamwork. So, this middleware
is also designed to make a high quality Camera reference design and help
customers to quickly customize it for products.
## Software Architecture
### System Block Diagram
The system block diagram is divided into 6 layers:

![image_oryx](img/Oryx_arc_final.png)

Oryx is in the layer of fifth, it allows users to customise products by up-layer
such as Home Kit, CGI and other Users apps.
### Work Flow in Oryx
The main work flow and function details in oryx are shown in below:

![image_oryx_fun](img/oryx_fun.png)
### Files and Directories in Oryx
All Oryx components are under `$(ambarella)/oryx/`, they are organized into
different folders by function.
|File|Abstract|
|:---|:-------|
|[analytics]| placeholder for intelligent algorithm related components.|
|[audio]| audio capture, audio codecs, audio device and etc components.|
|[cloud_storage]| placeholder for cloud storage.|
|[configure]| configure module, by Lua script. Oryx components use Lua script to parse config file.|
|[discovery]| placeholder for camera auto discovery in a network.|
|[event]| system event like button press, audio alert and etc.|
|[image_quality]| image quality control including AE/AWB/AF and etc.|
|[include]| common header files of Oryx.|
|[ipc]| inter process communication used by different services of Oryx.|
|[network]| network configuration, use network manager to do the job.|
|[services]| the application level, processes that use Oryx middleware to do a task.|
|[stream]| video and audio playback/recording/network streaming.|
|[utility]| common utility tools.|
|[video]| all DSP related video format configuration, control, and video related features.|
|[watchdog]| watchdog module.|
