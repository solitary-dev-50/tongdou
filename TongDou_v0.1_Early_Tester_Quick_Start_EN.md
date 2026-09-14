# TongDou v0.1.2 Early Tester – Quick Start

## 1. Before you start

You will need:

- A Windows PC with Docker Desktop installed and running.
- The TongDou Server ZIP and Firmware ZIP, both fully extracted to normal folders. Do not run files from inside a ZIP.
- A USB data cable for TongDou.
- TongDou and the Server PC connected to the same local network.
- Your own working provider/API configuration for speech recognition (ASR), a language model (LLM), and text-to-speech (TTS).

Allow Docker Desktop and TongDou Server through the Windows firewall on private networks if Windows asks.

## 2. Start TongDou Server

1. Open the extracted TongDou Server folder.
2. Double-click `START_TONGDOU_SERVER.bat`.
3. The first build may take several minutes. Wait until the script reports `SUCCESS` and opens the local dashboard.
4. Note the LAN address shown by the startup script.

In the supplied Docker setup, enter this address on TongDou's device configuration page:

```text
http://<server-PC-LAN-IP>:8102
```

Replace `<server-PC-LAN-IP>` with the LAN IP address of the PC running TongDou Server. **Use the address shown by the TongDou Server startup script.** Port `8102` is the Early Tester device-configuration entry. Do not enter port `8002`, `8100`, or `8103`.

Keep TongDou Server and Docker Desktop running while testing online features.

## 3. Configure voice services

In the TongDou Server dashboard:

1. Create or sign in to your local dashboard account.
2. Configure a working ASR service, LLM service, and TTS service using your own providers and API credentials.
3. Create or select an agent, assign those three services to it, and save the configuration.

Provider-specific setup is not included here. TongDou needs all three services for the complete voice path: hearing speech, generating a reply, and speaking it aloud.

## 4. Flash TongDou

1. Open the fully extracted Firmware folder.
2. Connect TongDou to the PC with a USB data cable.
3. Close any serial monitor or other program using the device's serial port.
4. Double-click `FLASH_TONGDOU.bat` and confirm the fresh installation.
5. If more than one serial device is found, select TongDou from the numbered list.
6. Wait for `SUCCESS` and the automatic reset before disconnecting the cable.

No Python, PlatformIO, ESP-IDF, or compiler installation is required. A fresh installation clears previously saved device settings, so complete first setup again after flashing.

## 5. Connect TongDou to Wi-Fi and Server

If TongDou has no saved Wi-Fi configuration, it creates a setup network named **TongDou-Setup**.

1. Connect a phone or computer to **TongDou-Setup**.
2. If the page does not open automatically, visit [http://192.168.4.1/](http://192.168.4.1/).
3. Enter your Wi-Fi network name and password.
4. Enter the TongDou Server address in this format: `http://<server-PC-LAN-IP>:8102`.
5. Save the settings and wait for TongDou to connect to Wi-Fi and TongDou Server.
6. When TongDou displays or speaks an activation code, bind that code to the prepared agent in the Server dashboard.
7. Wait for activation to complete and for the voice backend to become ready.

The expected first-connection sequence is: **Wi-Fi → Server connection → activation → voice backend ready**.

## 6. First test

Wake TongDou in either of these ways:

- Single-tap the TongDou Logo, then speak after it enters listening mode.
- Say **“Ni hao, TongDou”** — the Chinese phrase “你好，铜豆”.

The local wake phrase remains this Chinese phrase regardless of the selected interface language. Try a short natural question first and confirm that TongDou listens, replies, plays speech through its speaker, and then returns to its ready state.

## 7. Useful things to try

- Have a short natural conversation.
- Create, list, delete, and snooze a reminder.
- Ask for the weather or current news.
- Ask TongDou to search the web.
- Change the speaker volume by voice.
- Observe the activity-break and hydration reminders while desk presence is detected.

For the complete confirmed feature list and usage details, see `TongDou_v0.1_Early_Tester_Features_and_Usage_EN.md`.

## 8. Known radar limitation

**Desk presence detection is environment-dependent.** Different desk layouts, reflective objects, nearby movement, and room structure can cause false presence or delayed away detection. This can affect the timing of activity-break and hydration reminders. **This does not necessarily indicate a defective radar module.** Early Testers are not expected to perform environmental calibration.

## 9. If something goes wrong

Check that Docker Desktop and TongDou Server are still running, that the startup script reported `SUCCESS`, that TongDou and the Server PC are on the same local network, and that ASR, LLM, and TTS are all configured for the selected agent.

When reporting a problem, please include:

- What you were doing.
- What TongDou did.
- What you expected.
- The approximate local time.
- A photo, short video, device log, or Server log when useful.
- The desk layout and nearby movement if the issue is radar-related.

## v0.1.2 delivery notes

Use the matching v0.1.2 application and server packages. The server package now includes the current memory worker and the local voiceprint service. Model and embedding credentials must be supplied by the tester; memory is not ready merely because an image has been installed. See the included Features & Usage document for the memory boundary and follow-up tests.

IP / device-console queries were physically tested and confirmed by the project owner on 2026-09-07. The device address and the server dashboard address are different.

The first move from the v0.1.0 flash layout requires the full USB image and clears Wi-Fi, server address, reminders, language, volume, and motor direction/power settings. Reconfigure these after flashing. The hardware has no RESET button: power-cycle with its switch. If a board has an accessible BOOT input, hold it low while switching the power on to enter download mode, then release it. Otherwise request board-specific recovery instructions; do not short unidentified pads.

Remote update support is supplied for early testing. Production-level power-loss stress testing was not performed for this release and is not a release gate.
