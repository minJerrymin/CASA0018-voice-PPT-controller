# CASA0018 Project Log: Offline TinyML Voice Interface for Hands-Free Presentation Control

## Project Title

**Offline TinyML Voice Interface for Hands-Free Presentation Control**

## Project Goal

This project implements a continuous TinyML voice controller for PowerPoint using the Arduino Nano 33 BLE Sense Rev2. The system uses the onboard microphone to capture voice commands, runs an Edge Impulse keyword spotting model locally on the Arduino, and sends serial commands to a Python script on the host computer. The Python script then uses `pyautogui` to control PowerPoint.

The final prototype is intended to demonstrate an end-to-end embedded AI pipeline:

`microphone input → on-device TinyML inference → serial command output → real computer control`

The system recognises three presentation-related voice commands:

- `next page`
- `go back`
- `exit`

It also includes two non-command classes:

- `noise`
- `unknown`

These additional classes are used to reduce false activations during continuous listening.

---

## Project Positioning

At first glance, a voice-controlled PowerPoint controller may seem like a simple replacement for a keyboard, mouse, or presentation clicker. However, the main purpose of this project is not to replace existing presentation tools in ordinary conditions. Instead, the project explores how a low-cost microcontroller can be used as an offline, local voice-command interface for hands-free and more accessible slide control.

The project is positioned as an embedded AI interaction prototype rather than a general-purpose speech recognition system. It does not attempt to understand natural language or transcribe speech. Instead, it uses keyword spotting to recognise a small set of predefined commands that are directly mapped to presentation actions. This narrower scope makes the system more realistic for a microcontroller-based TinyML project.

The key value of the project is local inference. Unlike cloud-based speech assistants, the audio does not need to be sent to an external server. The Arduino Nano 33 BLE Sense Rev2 captures audio through its onboard microphone, processes the signal locally, and runs the trained model directly on the board. This makes the system more privacy-preserving, independent of internet connectivity, and suitable for demonstrating machine learning deployed at the edge.

The intended use case is a hands-busy presentation scenario. In many demonstrations, the presenter may not always be able to conveniently use a keyboard, mouse, or handheld clicker. For example, a presenter might be holding a physical prototype, performing a lab demonstration, explaining a hardware build, or running a live coding session. In these cases, a simple voice command such as “next page” could allow the presenter to continue the demonstration without interrupting their flow.

The project also has an accessibility-oriented motivation. Some users may find conventional input devices inconvenient or difficult to use, especially if they have limited hand mobility or difficulty operating small handheld controllers. This project does not claim to solve accessibility challenges completely, but it explores the potential of a low-cost voice interface as an alternative interaction method for presentation control.

Therefore, the project should not be understood simply as “voice control for PowerPoint”. A more accurate description is:

**A low-cost offline TinyML voice interface that supports hands-free and more accessible presentation control through three predefined voice commands.**

This positioning gives the project a clearer embedded AI purpose. The focus is not only on whether the slides can be controlled, but also on how reliable local keyword spotting can be under realistic operating conditions, such as different speaker distances, background noise, and non-command speech.

---

## Research Question

**How reliably can a low-cost microcontroller perform offline keyword spotting for hands-free presentation control under different speaker distances and background noise conditions?**

This research question focuses the project on three important issues:

1. Whether a small embedded device can perform useful local voice recognition.
2. How continuous listening affects false positives and false negatives.
3. How environmental conditions influence real-time model performance.

---

## Hardware and Software

## Hardware

- Arduino Nano 33 BLE Sense Rev2
- Onboard digital microphone
- USB cable for power and serial communication
- Laptop or desktop computer
- PowerPoint

## Software

- Edge Impulse
- Arduino IDE
- Python
- `pyserial`
- `pyautogui`

---

## System Overview

The system is divided into two parts:

1. **Arduino-side inference**
   - The Arduino continuously listens through the onboard microphone.
   - Audio features are extracted using the Edge Impulse pipeline.
   - A TinyML keyword spotting model runs locally on the board.
   - If a command is recognised with sufficient confidence, the Arduino sends a serial command to the computer.

2. **Computer-side control**
   - A Python script reads serial messages from the Arduino.
   - The script maps each serial command to a keyboard action.
   - PowerPoint responds to those keyboard actions.

The data and control flow is:

`voice command → Arduino microphone → MFCC feature extraction → TinyML classification → serial command → Python keyboard control → PowerPoint action`

---

## Model Classes

The model uses five classes:

| Class | Meaning | Action |
|---|---|---|
| `next page` | User wants to move to the next slide | Send `CMD:NEXT` |
| `go back` | User wants to return to the previous slide | Send `CMD:BACK` |
| `exit` | User wants to exit presentation mode | Send `CMD:ESC` |
| `noise` | Environmental sound or non-speech noise | No action |
| `unknown` | Speech that is not one of the target commands | No action |

The `noise` and `unknown` classes are important because the system is designed for continuous listening. Without these classes, the model would be more likely to incorrectly map background sound or unrelated speech to one of the three command classes.

---

## Serial Protocol

The Arduino sends simple serial messages to the computer:

| Recognised command | Serial output |
|---|---|
| `next page` | `CMD:NEXT` |
| `go back` | `CMD:BACK` |
| `exit` | `CMD:ESC` |

The Python script maps these serial messages to PowerPoint actions:

| Serial command | Keyboard output | PowerPoint effect |
|---|---|---|
| `CMD:NEXT` | Right Arrow | Next slide |
| `CMD:BACK` | Left Arrow | Previous slide |
| `CMD:ESC` | Escape | Exit presentation mode |

---

## Data Collection Plan

The project requires labelled audio data for all five classes.

The three command classes should include repeated recordings of:

- “next page”
- “go back”
- “exit”

The two non-command classes should include:

- `noise`: keyboard sounds, mouse clicks, room noise, coughing, desk tapping, and other background sounds.
- `unknown`: spoken phrases that are not commands, such as ordinary presentation speech.

To improve robustness, data should be collected under varied conditions:

- Different distances from the microphone, such as 20 cm, 50 cm, and 1 m.
- Different speaking speeds.
- Different speaking volumes.
- Quiet and noisy environments.
- Ideally, more than one speaker.

This data collection strategy supports the project’s research question because it allows the model to be tested under conditions closer to real presentation environments.

---

## Model Development

The model is trained in Edge Impulse as a keyword spotting classifier.

The intended signal processing and learning pipeline is:

1. Microphone audio input.
2. Audio windowing.
3. MFCC feature extraction.
4. Neural network classification.
5. Deployment as an Arduino library.

Initial model testing achieved around 90% test accuracy. However, live classification revealed a major difference between offline testing and real-time use. The system was sensitive to the fixed 1-second inference window.

The main issue was that a spoken command could be split across adjacent windows. For example, the phrase “next page” might not always be fully captured within a single 1-second window. In some cases, the partial audio from “next page” was misclassified as “go back”. This showed that a high offline test accuracy did not automatically guarantee stable live control.

To improve live performance without collecting new data, the model was retrained with a faster window increase of 250 ms. This made the sliding window update more frequently and increased the chance that a full command phrase would be captured in at least one inference window.

---

## Deployment Logic

Arduino-side post-processing was added to improve real-time reliability.

The deployment logic includes:

- A confidence threshold before executing commands.
- A cooldown period to prevent one spoken command from triggering multiple slide actions.
- A delayed pending-back state for `go back`.
- Override logic where a later `next page` prediction can cancel a pending `go back`.
- A stricter threshold for `exit`, because a false exit is more disruptive than a false slide navigation.

This post-processing is an important part of the system. The project demonstrates that real-time TinyML applications require more than just a trained model. They also require application-level decision logic to manage uncertainty, timing, and the consequences of false positives.

---

## Current Status

The Arduino model is running successfully on the Arduino Nano 33 BLE Sense Rev2.

The system can output the following serial commands:

- `CMD:NEXT`
- `CMD:BACK`
- `CMD:ESC`

The Python script can read these commands and control PowerPoint through keyboard simulation.

Current command performance:

- `next page`: working well after post-processing.
- `go back`: working well after adding delayed pending-back logic.
- `exit`: working, but requires further threshold tuning because it can sometimes be confused with `go back`.

---

## Key Limitation

The main limitation is that live performance is affected by audio segmentation. Although offline validation and testing accuracy were high, real-time continuous listening introduced additional challenges.

The most important issue was fixed-window inference. A 1-second window may not always capture a complete spoken phrase. This can lead to partial-command misclassification, especially when phrases are short or acoustically similar.

This limitation shows that evaluating an embedded AI system only through test accuracy is not enough. For real-time voice interfaces, timing, window overlap, confidence thresholds, and temporal post-processing all strongly affect the user experience.

---

## Evaluation Plan

The project should be evaluated through both model-level and system-level tests.

## Model-level evaluation

- Accuracy on the Edge Impulse test set.
- Confusion matrix between the five classes.
- Special attention to confusion between:
  - `next page` and `go back`
  - `go back` and `exit`
  - command classes and `unknown`
  - command classes and `noise`

## System-level evaluation

The full prototype should be tested by speaking commands while PowerPoint is open.

Suggested test conditions:

| Test | Purpose |
|---|---|
| Quiet room | Measure baseline reliability |
| Different microphone distances | Test real presentation conditions |
| Background keyboard/mouse noise | Test noise robustness |
| Background speech | Test false activations from `unknown` speech |
| Repeated commands | Test cooldown and duplicate-trigger prevention |
| Different speakers | Test generalisation across users |

The most important system-level metrics are:

- Correct command trigger rate.
- False positive rate.
- False negative rate.
- Number of duplicate triggers.
- Whether errors are disruptive during actual slide control.

---

## Reflection Notes

This project shows that building an embedded AI system is not only about training a model with high test accuracy. The live deployment environment introduces timing, noise, and user interaction challenges that are not fully visible during offline testing.

The most valuable learning so far is that real-time keyword spotting depends heavily on how audio is segmented into inference windows. The model may classify individual windows correctly in isolation, but the user experience depends on whether the system captures a complete spoken command at the right time.

The addition of a 250 ms window increase and Arduino-side post-processing improved reliability because the system became more tolerant of phrase timing and short misclassifications.

Future improvements could include:

- Collecting more data from different speakers.
- Adding more varied `unknown` and `noise` samples.
- Testing more distances and noise conditions.
- Fine-tuning class-specific thresholds.
- Improving the `exit` command to reduce confusion with `go back`.
- Exploring a wake word or push-to-talk option for higher safety.
- Replacing the USB serial connection with BLE for a more wireless prototype.

---

## Summary

This project is an offline TinyML voice-command interface for hands-free presentation control. It uses the Arduino Nano 33 BLE Sense Rev2 to perform local keyword spotting and sends simple serial commands to a Python script that controls PowerPoint.

The project is motivated by hands-busy presentation scenarios and accessibility-oriented interaction. Its technical contribution is not simply that it can control slides, but that it demonstrates a complete embedded AI pipeline running locally on a microcontroller.

The project also highlights an important challenge in real-time TinyML systems: high offline accuracy does not guarantee reliable live behaviour. Real-time voice control requires careful data collection, windowing choices, threshold tuning, and temporal post-processing.
