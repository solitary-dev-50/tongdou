# TongDou v0.1 Early Tester – Features & Usage

## 1. What this release is

TongDou v0.1 is an early test release of the complete TongDou system: the TongDou device, its formal application firmware, and TongDou Server working together.

This release is intended for practical testing of voice conversation, reminders, proactive desk reminders, information tools, music, and TongDou's screen, light, and short physical reactions. It is not a finished consumer product, and some features depend on the test environment or external online services.

Features marked **Requires TongDou Server** need the device to be connected to Wi-Fi and to a running TongDou Server. The device still provides limited local feedback when the Server is unavailable, but it cannot run natural conversation or generate personality speech offline.

## 2. Voice interaction

### Wake TongDou by touching the Logo

Single-tap the TongDou Logo, then speak after TongDou enters listening mode. A single tap starts a new voice turn through the normal conversation system.

- A double tap may trigger a brief character reaction depending on the current state. It does not start a voice conversation.
- Long-press during an active conversation to end it.
- If a reminder is currently speaking, TongDou may temporarily ignore a new touch so that the reminder is not interrupted incorrectly.

**Requires TongDou Server.**

### Wake TongDou by voice

After TongDou has connected to the Server and played its ready sound, say:

> Ni hao, TongDou

This is the Chinese phrase **“你好，铜豆”**. Wake-word detection happens locally on the device, but the greeting and following conversation require TongDou Server.

The v0.1 local wake model recognises this Chinese wake phrase only. Changing the setup language does not change the wake phrase.

### Natural conversation

After waking TongDou, speak normally. TongDou sends speech to the Server for speech recognition, generates a reply using its current personality, converts the reply to speech, and streams the audio back to the device speaker.

After a reply, you can normally continue speaking without repeating the wake phrase. To finish, say that you want to end the conversation or long-press the Logo. If nobody speaks for the Server's idle period, TongDou ends the conversation and returns to its ready state.

TongDou's replies are intended to be short and useful, with a mildly cheeky but non-hostile personality. Recognition quality and response time can vary with room noise, microphone distance, network conditions, and the configured Server services.

### Speaker volume

You can ask TongDou for its current speaker volume or ask it to change the volume, for example:

- “What is your current volume?”
- “Set the volume to 40 percent.”
- “Turn the volume down a little.”
- “Mute the speaker.”

**Requires TongDou Server.**

## 3. Reminders

Ordinary reminders are created through natural voice. TongDou Server understands the request and saves the resulting reminder on the device.

### Create a reminder

Try requests such as:

- “Remind me in ten minutes to check the oven.”
- “Remind me at 3 PM to call Alex.”
- “My meeting is at 4 PM. Remind me ten minutes beforehand to prepare my notes.”

For an event with advance notice, TongDou keeps the advance reminder and the event-time reminder as related parts of the same event.

Only treat the reminder as created after TongDou confirms that the operation succeeded. **Creating reminders by voice requires TongDou Server.**

### List reminders

Ask, for example:

- “What reminders do I have?”
- “List my reminders.”

TongDou reads the reminders that are actually stored on the device rather than guessing from conversation history. **Requires TongDou Server.**

### Delete a reminder

Ask TongDou to list reminders first if the target is unclear, then say which event to cancel. Deleting one part of a related event cancels all unfinished parts of that event.

**Requires TongDou Server.**

### Snooze a reminder

When a reminder finishes speaking, respond promptly with a request such as:

- “Remind me again in five minutes.”
- “Snooze that for ten minutes.”

TongDou opens a short response window after the announcement, so you do not need to say the wake phrase again. Snoozing adds a new delayed reminder without deleting a separate event-time reminder that is still pending.

**Requires TongDou Server.**

### When a reminder becomes due

- TongDou shows the reminder text on the OLED.
- When the Server is available, TongDou speaks the reminder through its normal personality and text-to-speech path.
- If the Server is unavailable, TongDou uses the current language's local offline notice followed by notification tones. The reminder does not crash or repeatedly flood the device.

Pending reminder records and their display images survive a device reboot. The maximum is 12 pending entries: a normal reminder uses 1 entry, an event with an advance reminder uses 2 entries, and snoozing adds another pending entry until it is delivered.

Ordinary reminders in v0.1 are one-time or event-based reminders. Daily, weekly, and other repeating schedules are not included in this release.

## 4. Proactive reminders

TongDou v0.1 includes two optional desk-presence reminders:

- **Activity-break reminder:** suggests getting up or moving after a configured period at the desk.
- **Hydration reminder:** suggests drinking water after a separate configured period at the desk.

These reminders use LD2412 desk presence as a practical proxy. TongDou does not detect posture, identify who is present, know whether you are sitting, or know whether you are thirsty or have already had a drink.

### Default settings

- Activity-break reminder: enabled, every **60 minutes**.
- Hydration reminder: enabled, every **45 minutes**.

### Change the settings

Open TongDou's local Web Config page and find **Proactive reminders**.

1. Enable or disable **Remind me to move**.
2. Set its interval from 1 to 1,440 minutes.
3. Enable or disable **Remind me to drink water**.
4. Set its interval from 1 to 1,440 minutes.
5. Select **Save proactive reminders**.

The enabled switches and intervals are saved on the device and survive a reboot. Elapsed desk time is not saved; after a reboot, timing starts again.

### How desk timing works

- Timing starts when the radar reports presence at the desk.
- The two reminder types keep independent timers.
- A brief loss of presence does not immediately clear the timers.
- Continuous absence for approximately five minutes resets both desk timers and clears their pending state.
- If both types become due together, TongDou delivers one and keeps the other pending. Different reminder types are separated by at least five minutes to avoid back-to-back announcements.
- Repeated reminders of the same type follow that type's configured interval; the five-minute separation is only for switching between the two types.

### Online and offline delivery

When TongDou Server is available, each proactive event enters the same personality and text-to-speech path used by the current TongDou voice system. The Server generates a short activity or hydration message rather than relying on one hard-coded sentence.

When the Server is unavailable, TongDou uses the corresponding OLED expression and light response, then plays the local offline notice and notification tones. It does not save a backlog of old proactive messages to play all at once when the Server returns.

## 5. Information & tools

The current TongDou Server exposes the following tools to normal voice conversation. Ask in natural language; there is no separate tool menu.

### Weather

Ask for current conditions or a forecast for a named city, for example, “What is the weather in London tomorrow?” Weather data comes from the Server's configured weather provider.

### News

Ask for current news or a news item from an available source. You can then ask for more detail about the most recently returned item. Available sources are controlled by the Server configuration.

### Web search

Ask TongDou to search the web when you need current online information. Search results depend on the Server's configured search provider and may be incomplete or temporarily unavailable.

### Date and lunar-calendar queries

TongDou can answer current date questions and provide lunar-calendar information, including queries for a specified date. The Server also includes date-related data such as lunar dates and solar terms.

### Music playback

Music capability is included, but the Early Tester package does not include music files by default. Testers may add their own legally usable audio files to the Server music library. TongDou can then play a named track or select from the available local library; it is not an unlimited online music catalogue.

All features in this section **require TongDou Server**. Weather, news, and web search also require their external data providers to be reachable.

## 6. Expressions and physical reactions

TongDou uses its OLED, a single RGB status light, and selected short body movements as feedback.

- During voice interaction, the OLED and light distinguish listening, thinking, speaking, and idle states.
- Ordinary reminders can show their text on the OLED.
- Activity reminders use a concerned or serious expression with a gentle breathing light.
- Hydration reminders use a simple alert expression and a soft light.
- Selected events may trigger a brief physical reaction.

Movements are intentionally short and are not used for every spoken reply. TongDou v0.1 does not roam autonomously, and the motion system is not used to infer whether the user is sitting.

## 7. Language / first setup

### First setup

1. Start TongDou Server and use the address shown by the TongDou Server startup script. In the supplied Docker setup, enter `http://<server-PC-LAN-IP>:8102`, replacing `<server-PC-LAN-IP>` with the LAN IP address of the computer running the Server. Do not use port `8100`, port `8103`, or the legacy port `8002` example still shown in the Web Config input field.
2. Power on TongDou. If it has no saved Wi-Fi connection, it creates a Wi-Fi network named **TongDou-Setup**.
3. Connect a phone or computer to **TongDou-Setup**.
4. If the setup page does not open automatically, open `http://192.168.4.1/`.
5. Choose or enter the Wi-Fi network, enter its password, and enter the exact TongDou Server address provided for the test.
6. Save and connect. Wait for TongDou to connect to Wi-Fi and then to TongDou Server.

After setup, the same Web Config page is available at TongDou's local network address. It provides network status, language selection, proactive reminder settings, and hardware diagnostics.

### Included setup languages

The current firmware accepts these setup locales:

- English (United Kingdom)
- English (Canada)
- French (France)
- French (Canada)
- Dutch (Netherlands)
- Russian
- Chinese (Simplified)

The factory firmware includes the English (United Kingdom) (`en-GB`) local prompt pack. TongDou Server provides local prompt packs for all seven selectable locales. When you change the language, the device downloads and saves the selected language pack; it does not store all seven packs in flash at the same time. After a language pack has downloaded successfully, its local prompts remain available if TongDou Server is temporarily unavailable.

The selected locale is saved on the device and is used for the local setup page, local prompts, activation-code speech, and reminder locale requests.

The local wake phrase remains **“Ni hao, TongDou”** in every setup locale. Natural conversation language and voice depend on the speech recognition, language model, and text-to-speech configuration assigned by TongDou Server. Not every complete voice path has been separately verified in all seven locales.

## 8. Advanced / experimental

TongDou uses MCP, an external tool integration protocol, internally so that TongDou Server can call real device functions such as reminder and volume operations. Early Testers do not need to configure MCP for normal use.

TongDou Server supports optional external tool extensions, but none are configured in the standard Early Tester setup.

## 9. Known limitations

- **Desk presence detection is environment-dependent.** Different desk layouts, reflective objects, nearby movement, and room structure can cause false presence reports or delay the decision that the user has left. This can affect both activity-break and hydration reminder timing. **This does not necessarily indicate a defective radar module.** Early Testers are not expected to perform environmental calibration.
- The radar reports presence only. It does not recognise posture, sitting time directly, identity, thirst, drinking, or completed activity.
- Natural conversation, online personality speech, voice-created reminders, information tools, and music require TongDou Server. Internet-based tools additionally depend on external services.
- The local wake phrase is currently Chinese only: “Ni hao, TongDou” (你好，铜豆).
- Selecting a setup locale does not by itself guarantee that the Server's complete speech recognition and generated voice chain is configured for that language.
- Some less-used setup labels may still fall back to English in non-English locales.
- Ordinary reminders do not yet support daily, weekly, weekday, or other repeating schedules.
- The maximum is 12 pending entries: a normal reminder uses 1 entry, an event with an advance reminder uses 2 entries, and snoozing adds another pending entry until it is delivered.
- Proactive reminder settings survive reboot, but accumulated desk time does not.
- Music capability is included, but the Early Tester package does not include music files by default. Testers may add their own legally usable audio files to TongDou Server's local music library.
- Search, weather, and news answers can be delayed, incomplete, or unavailable when their third-party source is unavailable.
- External MCP services are supported by the Server code but are not configured in the standard Early Tester environment.

## 10. What feedback is useful

When reporting an issue, please include:

1. **What you were doing** — for example, touching the Logo, saying the wake phrase, creating a reminder, waiting for a proactive reminder, or asking for weather.
2. **What TongDou did** — include the spoken response, OLED expression or text, light behaviour, movement, tones, or silence.
3. **What you expected** to happen.
4. **The approximate local time** of the event. This allows device and Server logs to be matched.
5. **The desk environment if radar-related** — TongDou's position and direction, nearby people or movement, large reflective or moving objects, and roughly how long you had been away.

Also helpful when relevant:

- The exact words you said and the selected setup language.
- Whether TongDou Server was running and whether other online voice requests worked at the time.
- Whether the problem happened once or can be repeated.
- A short video, photo of the desk layout, or the relevant serial and Server log excerpt.
- For reminder problems, the requested time, the actual trigger time, and whether the device was rebooted in between.

For radar-related reports, describe the environment and observed behaviour first. Do not recalibrate the radar unless the test coordinator specifically asks you to do so.
