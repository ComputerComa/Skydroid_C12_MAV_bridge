# Video Path Options

## Purpose

This document compares possible ways to deliver Skydroid C12 video from the
aircraft to Mission Planner. It records the current direction without treating
unverified range, latency, codec, or hardware claims as established facts.

MAVLink is not a video transport. Camera Protocol v2 may advertise stream
metadata and ground-side connection information, but visible and thermal video
packets always travel through a separate media path.

## Current preference

WFB-ng is the preferred primary transport because it can carry low-latency
RTP/UDP video directly to the ground laptop without an external display or USB
video-capture device. It can also carry MAVLink as a separate WFB-ng service.

LTE/Tailscale and 915 MHz remain useful independent backup telemetry paths.
Routine video should not depend on the cellular link.

This preference remains subject to bench testing with the real C12, Raspberry
Pi, selected Wi-Fi adapters, and Mission Planner.

## Option 1: Raspberry Pi composite video and analog VTX

### Path

```text
C12 RTSP
  -> Raspberry Pi decode, composition, and display
  -> Raspberry Pi composite video output
  -> analog 5.8 GHz VTX
  -> analog receiver with USB UVC capture
  -> Mission Planner laptop
```

### Advantages

- Inexpensive and widely available FPV hardware.
- Straightforward RF behavior and graceful visual degradation.
- A processed full-screen Pi display can include overlays and picture-in-picture.

### Costs and risks

- Pi 4 composite output is standard-definition interlaced NTSC/PAL, not 480p.
- Enabling composite output disables HDMI output on the Pi 4.
- The ground station still needs a receiver and USB capture device.
- Analog noise and limited resolution reduce thermal detail and text clarity.
- The UVC receiver adds capture latency and another driver/device dependency.
- High-power analog transmitters require careful power, cooling, antenna, and
  regulatory planning.
- Raspberry Pi TRRS cable wiring must match the Pi pinout.

This is a reasonable fallback experiment, but it does not meet the goal as
cleanly as a native digital feed inside Mission Planner.

Reference: [Raspberry Pi composite video configuration](https://www.raspberrypi.com/documentation/computers/config_txt.html#composite-video-mode).

## Option 2: WFB-ng digital RTP transport

### Unmodified stream path

```text
C12 RTSP over Ethernet
  -> GStreamer or FFmpeg depayload and RTP repacketization
  -> RTP/UDP to WFB-ng air input, normally port 5602
  -> WFB-ng raw Wi-Fi link
  -> WFB-ng ground output, normally port 5600
  -> Mission Planner GStreamer receiver
```

If the C12 codec and packetization are compatible, this path may avoid decoding
and re-encoding. It still performs network reception, parsing, and RTP
repacketization, so it is not literally free of CPU work.

### Processed stream path

```text
C12 RTSP
  -> decode
  -> image processing, overlays, or picture-in-picture
  -> H.264/H.265 encode
  -> RTP packetization
  -> WFB-ng
  -> Mission Planner
```

Modifying pixels forces a decode and re-encode cycle. Hardware encoder support,
pixel formats, bitrate, keyframe interval, and end-to-end latency must therefore
be measured before selecting an OpenCV or GStreamer processing design.

### Advantages

- Digital video enters Mission Planner without an external monitor or capture
  converter.
- WFB-ng maps RTP/UDP packets to radio packets and provides configurable forward
  error correction.
- Video and bidirectional MAVLink can use separate WFB-ng services on the same
  radio system.
- LTE data usage is avoided during normal operation.
- Independent visible and thermal feeds can use separate service and UDP-port
  assignments if link capacity permits.

### Costs and risks

- Supported monitor-mode Wi-Fi hardware and patched drivers are required.
- Airborne Wi-Fi hardware may require a dedicated 5 V supply, cooling, suitable
  antennas, and stable USB wiring.
- FEC mitigates packet loss but cannot guarantee artifact-free video.
- Two simultaneous streams increase bandwidth and RF-link requirements.
- Ground-side drivers and WFB-ng configuration must be maintained on the
  Mission Planner computer.

References:

- [WFB-ng project](https://github.com/svpcom/wfb-ng)
- [WFB-ng setup guide](https://github.com/svpcom/wfb-ng/wiki/Setup-HOWTO)
- [Mission Planner live video](https://ardupilot.org/planner/docs/live-video.html)

## Option 3: SIYI HM30 network bridge

### Path

```text
C12 RTSP or Raspberry Pi processed RTSP
  -> Ethernet
  -> HM30 air unit
  -> HM30 digital radio link
  -> HM30 ground unit Ethernet or Wi-Fi
  -> Mission Planner GStreamer receiver
```

### Advantages

- Purpose-built network radio with less Linux monitor-mode driver work.
- The ground laptop receives a normal IP network path.
- Mission Planner can consume a reachable RTSP stream.
- It may carry other IP traffic in addition to video.

### Costs and risks

- Higher acquisition cost and additional proprietary hardware.
- Advertised range and latency depend on RF conditions, antennas, bitrate, and
  installation quality.
- Processed overlays still require decode, processing, re-encode, and RTSP
  hosting on the Pi.
- Codec, addressing, routing, and Mission Planner interoperability still require
  bench verification.

Reference: [SIYI HM30 user manual](https://siyi.biz/siyi_file/HM30/HM30_User_Manual_en_v1.2.pdf).

## Camera Protocol v2 integration

The camera component should describe the stream that exists at the ground
station, not incorrectly imply that video travels in MAVLink. For WFB-ng, this
will likely mean one `VIDEO_STREAM_INFORMATION` instance per downlinked feed,
using `VIDEO_STREAM_TYPE_RTPUDP` and the configured ground-side UDP port.

A possible two-stream assignment is:

| Stream | Content | WFB-ng air input | Ground output | MAVLink flags |
| --- | --- | ---: | ---: | --- |
| 1 | Visible | 5602 | 5600 | Running |
| 2 | Thermal | To be assigned | To be assigned | Running, thermal |

The second service ports must be selected from the actual WFB-ng configuration;
they should not be guessed in the implementation.

Camera commands, status, and stream descriptions use MAVLink. C12 RTSP, relayed
RTP, and processed frames do not.

## Validation sequence

1. Establish WFB-ng with a synthetic H.264 RTP test stream.
2. Display the ground-side UDP stream in a standalone GStreamer pipeline.
3. Display the same stream in Mission Planner.
4. Relay one unmodified C12 stream without transcoding if the codec permits.
5. Measure latency, bitrate, packet loss, recovery, CPU load, and temperature.
6. Add the second C12 stream and repeat the measurements.
7. Publish verified stream metadata through Camera Protocol v2.
8. Add onboard image processing only after the direct media path is reliable.
9. Compare WFB-ng against HM30 or analog only if testing exposes a material
   reliability, latency, integration, or regulatory problem.

## Decision boundary

Do not purchase hardware based only on advertised range or example pipelines.
Record the selected adapters, drivers, antennas, frequencies, power system,
codecs, bitrates, and measured results when bench hardware becomes available.
