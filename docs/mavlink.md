# MAVLink references for the C12 bridge

## Read first

- [Using C MAVLink Libraries](https://mavlink.io/en/mavgen_c/)  
  How the native generated C headers work, including packing, sending, and parsing MAVLink messages.

- [Heartbeat / Connection Protocol](https://mavlink.io/en/services/heartbeat.html)  
  Required for component discovery. Begin with a 1 Hz onboard-computer
  heartbeat; camera and gimbal heartbeats follow when those components are
  implemented.

- [MAVLink System and Component ID Assignment](https://mavlink.io/en/services/mavlink_id_assignment.html)  
  Explains system IDs and component IDs. The Pi onboard computer, camera, and
  gimbal use distinct component IDs under the Pixhawk vehicle's system ID.

- `ONBOARD_COMPUTER_STATUS` in the generated MAVLink headers
  Reports measured companion-computer health. Populate only fields the Pi can
  measure reliably and use the message-defined unknown values for the rest.

- [Command Protocol](https://mavlink.io/en/services/command.html)  
  Covers `COMMAND_LONG`, `COMMAND_INT`, and `COMMAND_ACK`. Both the camera and gimbal bridge need to correctly acknowledge supported commands.

## C12 camera component

- [Camera Protocol v2](https://mavlink.io/en/services/camera.html)  
  The primary specification for advertising the C12 as a MAVLink camera, reporting its capabilities, RTSP streams, video-recording state, capture events, zoom, and camera configuration.

- [Camera Definition Files](https://mavlink.io/en/services/camera_def.html)  
  Optional later work. A camera-definition XML file can let a ground station generate a configuration interface for C12-specific settings such as thermal palette, zoom, or image mode.

- [File Transfer Protocol](https://mavlink.io/en/services/ftp.html)  
  Optional later work if you decide to expose saved C12 images or recordings over MAVLink FTP. Do not make this part of the first milestone.

## C12 gimbal component

- [Gimbal Protocol v2](https://mavlink.io/en/services/gimbal_v2.html)  
  The primary specification for the C12 gimbal bridge. Focus first on discovery, `GIMBAL_DEVICE_INFORMATION`, `GIMBAL_DEVICE_SET_ATTITUDE`, and `GIMBAL_DEVICE_ATTITUDE_STATUS`.

- [Gimbal Manager protocol concepts](https://mavlink.io/en/services/gimbal_v2.html#concepts)  
  Important architecture reading: ArduPilot should normally be the Gimbal Manager, while the Pi bridge is the MAVLink Gimbal Device that translates MAVLink setpoints into C12 UDP commands.

- [ArduPilot gimbal mission commands](https://ardupilot.org/copter/docs/common-mavlink-mission-command-messages-mav_cmd.html)  
  Reference for the mission-side commands that may eventually control the C12, including `MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW`.

## Transport and reliability

- [MAVLink 2](https://mavlink.io/en/guide/mavlink_2.html)  
  MAVLink 2 features, including larger message IDs, extension fields, packet signing, and compatibility behavior.

- [Packet Serialization](https://mavlink.io/en/guide/serialization.html)  
  Explains the on-wire frame format, sequence numbers, payloads, checksums, and message IDs. Read this for understanding; use the generated C library rather than manually building MAVLink packets.

- [Routing](https://mavlink.io/en/guide/routing.html)  
  Relevant once the Pi, Pixhawk, SiK telemetry, Mission Planner, and companion network are all present. It explains how addressed and broadcast MAVLink messages travel through the network.

- [Message Signing](https://mavlink.io/en/guide/message_signing.html)  
  A future security item. Do not enable signing during initial bench integration, but understand it before using network-accessible MAVLink links in operational deployments.

## Later search-and-rescue features

- [Landing Target Protocol](https://mavlink.io/en/services/landing_target.html)  
  Potentially relevant if you later use camera detections to report a target in an image or relative frame.

- [Time Synchronization Protocol](https://mavlink.io/en/services/timesync.html)  
  Useful when correlating C12 video frames, hotspot detections, vehicle position, and gimbal attitude.

- [Traffic Management / ADS-B](https://mavlink.io/en/services/traffic_management.html)  
  Relevant to the planned ADS-B receive-and-display capability, but unrelated to the first C12 bridge milestone.
