# TongDou v0.1.2 Early Tester Guide

Thank you for helping test TongDou.

This guide is for people who have received a physical TongDou unit. It explains what you can try, how to complete the first setup, and what information is useful when something goes wrong.

TongDou v0.1 is an early test release, not a finished consumer product. Please expect rough edges, occasional setup problems, and visible changes between test builds.

## 1. What you received

TongDou is a small physical desktop companion built around an ESP32-S3 controller. Your test system has three parts:

1. **The TongDou device** — microphone, speaker, OLED screen, status light, motors, touch Logo, desk-presence radar, motion sensor, and battery hardware.
2. **TongDou application firmware** — runs on the device and controls voice transport, reminders, expressions, lights, movement, local setup, and hardware feedback.
3. **TongDou Server** — runs on a Windows computer through Docker Desktop and provides speech recognition, AI replies, speech generation, online tools, device binding, and management.

For normal AI use, TongDou and the computer running TongDou Server must be connected to the same local network. Docker Desktop and TongDou Server must remain running.

## 2. What you can test in the formal application firmware

> **Demo-firmware boundary:** First Summon / Nightmare and Confused Accountant are real TongDou scenes, but they belong to the separate TongDou Demo Firmware package. They are not features of the formal v0.1 application firmware. Do not flash Demo Firmware onto an application-firmware test unit unless the TongDou team specifically asks you to do so.

### 2.1 AI conversation

TongDou can listen to speech, send it to TongDou Server, generate an AI reply, and play the reply through its speaker.

After one reply, you can normally continue speaking without waking TongDou again. Conversation quality and response time depend on room noise, microphone distance, network conditions, and the speech and AI services configured on TongDou Server.

TongDou currently uses a half-duplex voice flow. In plain language, let TongDou finish speaking before you begin your next sentence.

### 2.2 Starting a conversation

You can start a conversation in either of these ways:

- **Single-tap the TongDou Logo**, then speak after TongDou enters listening mode.
- Say **“Ni hao, TongDou”**, the Chinese phrase **“你好，铜豆”**.

The wake phrase is detected on the device, but the greeting and conversation require TongDou Server.

The local wake model is currently Chinese-only. Changing the display language does not change the wake phrase.

During an active conversation:

- Long-press the Logo to end the conversation.
- A double tap triggers a short dance reaction when the current device state allows it; it does not start a voice conversation.

### 2.3 Weather

You can ask questions such as:

- “What is the weather in London today?”
- “Will it rain in Toronto tomorrow?”

Weather requires a working weather provider configured on TongDou Server. A server installation without weather credentials cannot answer live weather queries reliably.

### 2.4 News

You can ask for current news and request more detail about a returned item. Available sources depend on the TongDou Server configuration and may vary between test packages.

### 2.5 Web search

You can ask TongDou to search the web for current information. Web search requires a working search provider and its credentials on TongDou Server.

Search results can be delayed, incomplete, or temporarily unavailable when the external provider is unavailable.

### 2.6 Reminders

You can create and manage reminders by voice. Try:

- “Remind me in ten minutes to check the oven.”
- “Remind me at 3 PM to call Alex.”
- “My meeting is at 4 PM. Remind me ten minutes beforehand to prepare my notes.”
- “What reminders do I have?”
- “Cancel the reminder to call Alex.”

Only treat a reminder as saved after TongDou confirms that the operation succeeded.

After a reminder is announced, reply promptly with a request such as “Remind me again in five minutes” to test snooze. You do not normally need to repeat the wake phrase during the short reminder-response window.

When a reminder becomes due:

- The reminder text is shown on the OLED screen.
- When TongDou Server is available, TongDou speaks the reminder.
- When the server is unavailable, TongDou shows local feedback and plays a local notice with notification tones.

Pending reminders survive a normal reboot. This release supports one-time and event-based reminders, not daily or weekly repeating schedules.

### 2.7 Activity and hydration reminders

TongDou can use its desk-presence radar to provide two optional reminders:

- An activity-break reminder after a period of desk presence.
- A hydration reminder after a separate period of desk presence.

The default intervals are 60 minutes for activity and 45 minutes for hydration. You can enable, disable, or change them on TongDou's local configuration page.

The radar detects presence only. It does not know who is present, whether the person is sitting, whether they have exercised, or whether they have had a drink.

### 2.8 OLED expressions

The OLED screen, which is TongDou's small monochrome display, shows different states such as idle, listening, thinking, speaking, reminders, and selected character reactions.

Please note whether the expression matches what TongDou is doing. A frozen, missing, or obviously incorrect expression is useful feedback.

### 2.9 Light and movement feedback

TongDou uses one colour status light and short motor movements as feedback.

- Voice states use different screen and light feedback.
- Some reminders use a matching expression and gentle light effect.
- A double tap on the Logo can trigger a short dance.
- Selected events may trigger a nod, lean, shake, or short movement.

Movements are intentionally brief. TongDou does not roam around the room and does not move during every spoken reply.

Place TongDou on a stable, clear surface before testing movement. Keep fingers, cables, drinks, and fragile objects away from its moving area.

### 2.10 Other available functions

This version also includes:

- Voice control for reading and changing speaker volume.
- Current date and lunar-calendar queries through TongDou Server.
- Optional local music playback when legally usable music files have been added to the server library.
- A local configuration page for network status, language, proactive reminders, and hardware diagnostics.
- Seven selectable local interface languages: English (United Kingdom), English (Canada), French (France), French (Canada), Dutch, Russian, and Simplified Chinese.
- Local storage for network settings, language selection, reminder records, and proactive-reminder settings.
- A local fallback for due reminders when the AI server is temporarily unavailable.

Changing the local interface language does not automatically configure speech recognition or the spoken AI voice for that language. Those services must also support the chosen language on TongDou Server.

## 3. First-time setup

### 3.1 Before you begin

You need:

- A Windows computer with Docker Desktop installed.
- The complete TongDou Server Early Tester ZIP file.
- The TongDou firmware package if your unit has not already been prepared.
- A USB data cable if firmware installation is required.
- A local Wi-Fi network shared by TongDou and the server computer.
- Your own working speech-recognition, AI-model, and text-to-speech provider accounts and credentials.

In this guide:

- **Speech recognition** converts your voice into text.
- **AI model** generates TongDou's reply.
- **Text-to-speech** converts the reply back into spoken audio.

All three must work for a complete voice conversation.

### 3.2 Extract the server package

1. Fully extract `TongDou_Server_v0.1.2_Early_Tester.zip` to a normal folder.
2. Keep the folder structure unchanged.
3. Do not run files from inside the ZIP preview window.

The Windows one-click server launcher and its support files must stay together.

### 3.3 Start Docker Desktop

1. Open **Docker Desktop** from the Windows Start menu.
2. Wait until Docker Desktop reports that its engine is running.
3. If Docker Desktop asks to finish installation or restart Windows, complete that step before continuing.
4. Keep Docker Desktop open while using TongDou Server.

The TongDou one-click launcher starts the TongDou containers, but it does **not** start Docker Desktop itself. If Docker Desktop is closed or still starting, the launcher will stop and ask you to start it first.

### 3.4 Start TongDou Server with the one-click launcher

1. Open the extracted TongDou Server folder.
2. Double-click `START_TONGDOU_SERVER.bat`.
3. Keep the command window open.
4. On the first run, Docker Desktop downloads base images and builds the TongDou services locally. This may take several minutes.
5. Wait until the window shows `SUCCESS`.
6. The local dashboard should open automatically in your browser.
7. Write down the yellow **TongDou device Server address** printed by the launcher.

The default addresses are:

- Local dashboard on the server computer: `http://localhost:8102`
- Address entered on TongDou: `http://<server-computer-LAN-IP>:8102`

Always use the exact device address printed by the launcher. Do not enter `localhost` or `127.0.0.1` on TongDou, because those addresses would point back to the device itself.

If Windows Firewall asks, allow Docker or TongDou Server on **Private networks**. Do not disable the firewall.

The server package also includes:

- `STATUS_TONGDOU_SERVER.bat` — shows the current server status.
- `STOP_TONGDOU_SERVER.bat` — stops the server without intentionally deleting saved data.

Do not use `docker compose down -v`. The `-v` option deletes Docker data volumes and can remove dashboard accounts, devices, agents, and provider settings.

### 3.5 Configure TongDou Server

The first account created in a new local dashboard is the local administrator account.

After signing in:

1. Configure a working speech-recognition provider.
2. Configure a working AI-model provider.
3. Configure a working text-to-speech provider.
4. Create or select a TongDou agent.
5. Assign the three providers to that agent and save the configuration.

The Early Tester package does not include centrally funded speech or AI usage. Use only test credentials that you are authorised to use.

Weather and web search require separate provider credentials. If the TongDou team supplied dedicated Early Tester keys, follow the private key instructions supplied with your test package. Otherwise, those tools will remain unavailable until you configure your own supported providers.

Never include passwords, provider keys, access tokens, or the server's local secret in screenshots, videos, logs, or public feedback.

### 3.5.1 Optional: configure voiceprint recognition

Voiceprint recognition is optional. It is only needed when you want TongDou Server to identify different speakers. The full Docker package includes a local `voiceprint-api` service.

To find the token currently used by the running voiceprint container, open PowerShell in the TongDou Server folder and run:

```powershell
docker compose exec -T voiceprint-api printenv VOICEPRINT_API_TOKEN
```

Copy the complete printed value. Keep this token private. If the command says that the container does not exist, start TongDou Server first. The v0.1.2 launcher generates the token in the package-root `.env` on first start. Keep that file when updating.

In the dashboard, open **Parameter Management**, find `server.voice_print`, and set its value to:

```text
http://voiceprint-api:8005/voiceprint/health?key=<your-token>
```

Replace `<your-token>` with the complete token. Do not use `127.0.0.1` or the TongDou device IP in this value; the server reaches the voiceprint service through the Docker container name.

You can check the service from the server computer with:

```powershell
curl "http://127.0.0.1:8105/voiceprint/health?key=<your-token>"
```

The response should contain `"status":"healthy"`. Do not paste the token or this full URL into public bug reports.

### 3.6 Optional: install the formal application firmware

Most prepared Early Tester units should already have the correct application firmware. Only reinstall it when your test instructions ask you to do so.

1. Fully extract the supplied TongDou Application Firmware ZIP file.
2. Connect TongDou to the computer with a USB data cable.
3. Close any serial monitor or other program using TongDou's serial port.
4. Double-click `FLASH_TONGDOU.bat` and confirm the fresh installation.
5. If the script finds more than one serial device, select TongDou from the numbered list.
6. Wait for `SUCCESS` and the automatic reset before disconnecting the cable.

A fresh installation clears previously saved device settings. Complete Wi-Fi setup and device binding again afterwards. Do not use the separate Demo Firmware for this procedure.

### 3.7 Power on TongDou

Place TongDou on a stable surface and switch on its power.

During startup, the screen and light should show that the device is waking. If no network has been saved, TongDou enters setup mode and creates a Wi-Fi network named:

`TongDou-Setup`

### 3.8 Configure Wi-Fi and the server address

1. On a phone or computer, connect to the `TongDou-Setup` Wi-Fi network.
2. If the setup page does not open automatically, visit `http://192.168.4.1/` in a browser.
3. Select or enter your Wi-Fi network name.
4. Enter the Wi-Fi password.
5. Enter the exact **TongDou device Server address** printed by `START_TONGDOU_SERVER.bat`.
6. Save the settings and wait for TongDou to connect.

The default Early Tester device address looks like:

`http://192.168.x.x:8102`

The numbers will be different on your network. Use port `8102` unless your server package prints a customised port. Do not use ports `8100`, `8103`, or port `8002`.

TongDou and the server computer must be on the same local network. Guest Wi-Fi, client isolation, some corporate networks, and some VPN configurations can prevent devices from reaching each other even when both appear connected.

### 3.9 Bind the device

After TongDou reaches the server, it displays or speaks an activation code.

1. Open the TongDou Server dashboard at `http://localhost:8102` on the server computer.
2. Open the agent you prepared.
3. Choose the device-binding option.
4. Enter the activation code shown or spoken by TongDou.
5. Confirm the binding.
6. Wait for TongDou to finish activation and report that the voice service is ready.

The expected order is:

**Wi-Fi connection → TongDou Server connection → activation code → device binding → voice service ready**

If the device keeps showing a code, confirm that you bound the code to the correct agent and that the server computer's local network address has not changed.

### 3.10 First conversation test

1. Single-tap the TongDou Logo.
2. Wait for listening feedback.
3. Ask a short question such as “What day is it today?”
4. Confirm that TongDou listens, thinks, speaks, and returns to its ready state.
5. Ask a follow-up question without repeating the wake phrase.
6. Long-press the Logo to end the conversation.

After the Logo test works, try the voice wake phrase: **“Ni hao, TongDou.”**

## 4. Suggested test plan

You do not need to test everything in one session. Short, clearly described test sessions are more useful than a long list with no detail.

### 4.1 Basic voice path

- Test both Logo start and voice wake.
- Try speech from approximately 0.5 metre, 1 metre, and 2 metres away.
- Try a quiet room and normal background noise.
- Continue for at least five conversation turns.
- Note missing first words, long pauses, repeated replies, unexpected exits, or audio distortion.

### 4.2 Information tools

- Ask for weather in two named cities.
- Ask for current news and then ask for more detail.
- Ask a question that clearly requires a web search.
- Record whether TongDou says a tool is unavailable or invents an answer when a provider is not configured.

### 4.3 Reminder flow

- Create a reminder a few minutes ahead.
- List reminders and confirm the saved time and text.
- Delete one reminder.
- Create an event with an advance reminder.
- Snooze a reminder immediately after it is announced.
- Reboot TongDou before a reminder is due and confirm that it remains saved.
- If requested by the test brief, stop TongDou Server before a reminder is due and check the local fallback.

### 4.4 Screen, light, touch, and movement

- Check listening, thinking, speaking, and idle feedback.
- Single-tap, double-tap, and long-press the Logo.
- Watch for touch actions being missed or triggered twice.
- Check that short movements stop correctly and do not leave a motor running.
- Report unexpected scraping, repeated motor clicking, or a movement that could make TongDou fall.

### 4.5 Desk-presence reminders

- Put TongDou in its intended position on the desk.
- Note false presence when nobody is nearby.
- Note delayed away detection after leaving the desk.
- Record nearby movement, reflective objects, fans, pets, and room layout when reporting radar behaviour.

For a short test, you may temporarily reduce the activity or hydration interval on the local configuration page. Restore your preferred interval afterwards.

### 4.6 Restart and recovery

- Restart TongDou and confirm that network and reminder settings remain available.
- Stop and restart TongDou Server with the supplied scripts.
- Confirm that TongDou reconnects after the server becomes available again.
- If the server computer changes network, check the new address printed by the launcher and update TongDou's server setting if necessary.

## 5. How to report a problem

Use the private feedback channel supplied with your Early Tester invitation. If no private channel was provided, contact `hello@gettongdou.com`.

Please report one clear problem at a time and include:

1. **Test build** — firmware and server version, if known.
2. **What you were testing** — wake phrase, Logo, conversation, reminder, weather, radar, movement, and so on.
3. **Exact steps** — what you did in order.
4. **Actual result** — what TongDou displayed, said, played, or did.
5. **Expected result** — what you thought should happen.
6. **Approximate local time** — helps match device and server logs.
7. **Frequency** — once, sometimes, or every time.
8. **Environment** — room noise, distance, desk layout, network type, and nearby movement when relevant.
9. **Evidence** — a short video, photo, device log, or server log when useful.

You can inspect server logs in Docker Desktop. You can also open a command window in the extracted server folder and run:

```text
docker compose --env-file .env logs --tail 200
```

Remove passwords, provider keys, access tokens, private conversations, and personal information before sharing any log or screenshot.

### Useful feedback template

```text
Build/version:
Tested feature:
Steps:
Actual result:
Expected result:
Approximate local time:
How often it happens:
Room/network/desk conditions:
Photo, video, or log attached: Yes / No
```

## 6. What we especially want testers to watch

- Whether the wake phrase works reliably without frequent false wakes.
- Whether the first spoken words are captured or cut off.
- Whether multi-turn conversation starts and ends cleanly.
- Whether the OLED, light, sound, and movement agree with the current state.
- Whether reminder time, text, snooze, deletion, reboot recovery, and offline fallback are correct.
- Whether radar presence behaves consistently in a real desk environment.
- Whether the server setup and device-binding instructions are understandable without developer help.
- Whether errors explain the next useful action instead of leaving the tester stuck.
- Whether any motor, charging, heat, smell, battery, or power behaviour appears unsafe.

If the device becomes unusually hot, smells abnormal, shows battery swelling, repeatedly resets during charging, or makes a motor run continuously, switch it off, disconnect power, and contact the TongDou team. Do not continue stress testing a possibly damaged unit.

## 7. Known limitations

- This is an Early Tester build. Setup, reconnect behaviour, timing, wording, and visual feedback are still being refined.
- Natural conversation and online tools require a running TongDou Server and correctly configured external services.
- The local wake phrase is currently Chinese-only: “Ni hao, TongDou” / “你好，铜豆”.
- Voice interaction is currently half-duplex; speaking over TongDou's reply may not work reliably.
- Weather and web search require separate provider credentials. News availability depends on configured sources.
- Internet-based answers can be delayed or unavailable when external services fail.
- The selected local interface language does not guarantee that the complete speech-recognition and spoken-voice path supports that language.
- Ordinary reminders do not yet support daily, weekly, weekday, or other repeating schedules.
- The device stores a maximum of 12 pending reminder entries. An event with an advance notice uses two entries, and snoozing adds another pending entry until delivery.
- Radar presence is affected by room layout, reflective objects, nearby movement, fans, pets, and device position. A false presence report does not automatically mean the radar hardware is defective.
- Radar does not identify a person or detect posture, drinking, thirst, or completed exercise.
- Proactive-reminder settings survive reboot, but accumulated desk time starts again after reboot.
- Movements are short character reactions. Autonomous roaming is not included.
- Battery percentage and fully automatic low-battery character behaviour are not complete in this release.
- Personality styles exist in the system, but end-to-end user switching and persistence are not fully complete.
- Music files are not included by default. Testers must use their own legally usable files if music testing is requested.
- First Summon / Nightmare and Confused Accountant belong to the separate Demo Firmware package, not the formal v0.1 application firmware.
- Remote firmware update is still under staged validation. Follow only update instructions supplied for your exact test build.

## 8. Quick troubleshooting

### The server launcher says Docker was not found

Install Docker Desktop, then restart Windows if its installer requests it.

### The launcher says Docker Desktop is not running

Open Docker Desktop and wait until its engine reports that it is running. Then run `START_TONGDOU_SERVER.bat` again.

### The first server start appears slow

The first run builds the TongDou server and dashboard images. Keep the window open and check that the computer still has internet access. Later starts are normally faster.

### The dashboard does not open

Try `http://localhost:8102`, run `STATUS_TONGDOU_SERVER.bat`, and check Docker Desktop for stopped or unhealthy containers.

### TongDou cannot reach the server

- Confirm that TongDou and the server computer are on the same local network.
- Use the device address printed by the launcher, not `localhost`.
- Allow Docker and TongDou Server on Windows Private networks.
- Check whether a VPN, guest network, or Wi-Fi client isolation is blocking local connections.
- If the computer's local address changed, update the server address on TongDou.

### TongDou shows or speaks an activation code repeatedly

Bind the current code to the intended agent in the local dashboard. Confirm that the agent has working speech-recognition, AI-model, and text-to-speech providers.

### TongDou listens but does not give a useful spoken reply

Check all three voice services assigned to the agent. A complete conversation needs speech recognition, an AI model, and text-to-speech.

### Weather or web search does not work

Confirm that the required provider and credentials were configured. These credentials are not included automatically in every Early Tester package.

### Server ports are already in use

The package supports changing its three `TONGDOU_*_PORT` values in `.env`, but this is an advanced step. After changing ports, start the server again and enter the newly printed device address on TongDou.

## 9. Ending a test session

1. End any active conversation.
2. Switch off TongDou if you have finished testing the device.
3. Double-click `STOP_TONGDOU_SERVER.bat` if you want to stop the local server.
4. Exit Docker Desktop if you no longer need it.

The supplied stop script keeps normal server data. Back up the extracted server folder and Docker data before replacing the package with a major new test version.

Thank you for testing a real, unfinished TongDou and helping make it more reliable.

## v0.1.2 delivery notes

Use the matching v0.1.2 application and server packages. The server package now includes the current memory worker and the local voiceprint service. Model and embedding credentials must be supplied by the tester; memory is not ready merely because an image has been installed. See the included Features & Usage document for the memory boundary and follow-up tests.

IP / device-console queries were physically tested and confirmed by the project owner on 2026-09-07. The device address and the server dashboard address are different.

The first move from the v0.1.0 flash layout requires the full USB image and clears Wi-Fi, server address, reminders, language, volume, and motor direction/power settings. Reconfigure these after flashing. The hardware has no RESET button: power-cycle with its switch. If a board has an accessible BOOT input, hold it low while switching the power on to enter download mode, then release it. Otherwise request board-specific recovery instructions; do not short unidentified pads.

Remote update support is supplied for early testing. Production-level power-loss stress testing was not performed for this release and is not a release gate.
