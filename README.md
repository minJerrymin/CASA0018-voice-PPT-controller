# TinyML Voice Controller for PowerPoint

## Project Overview

This project is a CASA0018 embedded AI prototype that uses an **Arduino Nano 33 BLE Sense** and **Edge Impulse** to control PowerPoint with voice commands.

The Arduino uses its onboard microphone to listen continuously, runs a TinyML keyword spotting model locally, and sends simple serial commands to a Python script on the computer. The Python script then uses `pyautogui` to control PowerPoint.

The system recognises three commands:

| Voice command | Serial command | PowerPoint action |
|---|---|---|
| `next page` | `CMD:NEXT` | Next slide |
| `go back` | `CMD:BACK` | Previous slide |
| `exit` | `CMD:ESC` | Exit slideshow |

The aim of the project is not to build a full speech assistant, but to explore how a low-cost microcontroller can perform **offline keyword spotting** for hands-free presentation control.

---

## System Diagram

```mermaid
flowchart LR
    A[Voice command] --> B[Arduino microphone]
    B --> C[Edge Impulse TinyML model]
    C --> D[Serial command]
    D --> E[Python script]
    E --> F[PowerPoint control]
```

---

## Main Components

- **Arduino Nano 33 BLE Sense Rev2**  
  Captures audio and runs the TinyML model locally.

- **Edge Impulse**  
  Used to collect data, train the keyword spotting model, and export the Arduino library.

- **Python script**  
  Reads serial commands from Arduino and maps them to keyboard actions.

- **PowerPoint**  
  Responds to the simulated keyboard inputs.

---

## Model Design

The model was trained in Edge Impulse using five classes:

- `next page`
- `go back`
- `exit`
- `noise`
- `unknown`

The `noise` and `unknown` classes were included so the model would not treat every sound as a command.

The final model uses:

- Audio input at **16 kHz**
- **MFCC** feature extraction
- A small **1D CNN classifier**
- Quantised **int8 Arduino deployment**
- 1-second audio window
- 250 ms window increase for better live recognition


---

## Results

Two quantised int8 models were tested.

| Metric | Initial model | Final model |
|---|---:|---:|
| Test accuracy | 90.0% | 90.0% |
| AUC | 0.99 | 0.99 |
| Weighted precision | 0.92 | 0.93 |
| Weighted recall | 0.92 | 0.93 |
| Weighted F1 score | 0.92 | 0.93 |

Class-level results for the final model:

| Class | Test accuracy |
|---|---:|
| `exit` | 96.7% |
| `go back` | 100.0% |
| `next page` | 90.0% |
| `noise` | 83.3% |
| `unknown` | 80.0% |

The final system worked successfully in the full pipeline:

```text
voice command → Arduino inference → serial command → Python → PowerPoint
```

---

## Demo Use

1. Upload the Arduino code to the Nano 33 BLE Sense Rev2.
2. Close Arduino Serial Monitor.
3. Run the Python script:

```bash
python ppt_voice_controller.py
```

4. Open PowerPoint in slideshow mode.
5. Say one of the supported commands:
   - `next page`
   - `go back`
   - `exit`


---

## Edge Impulse Project

Add Edge Impulse project link here.

