/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */



/**
 * Define the number of slices per model window. E.g. a model window of 1000 ms
 * with slices per model window set to 4. Results in a slice size of 250 ms.
 * For more info: https://docs.edgeimpulse.com/docs/continuous-audio-sampling
 */


/*
 ** NOTE: If you run into TFLite arena allocation issue.
 **
 ** This may be due to may dynamic memory fragmentation.
 ** Try defining "-DEI_CLASSIFIER_ALLOCATION_STATIC" in boards.local.txt (create
 ** if it doesn't exist) and copy this file to
 ** `<ARDUINO_CORE_INSTALL_PATH>/arduino/hardware/<mbed_core>/<core_version>/`.
 **
 ** See
 ** (https://support.arduino.cc/hc/en-us/articles/360012076960-Where-are-the-installed-cores-located-)
 ** to find where Arduino installs cores on your machine.
 **
 ** If the problem persists then there's not enough memory for this model and application.
 */

/* Includes ---------------------------------------------------------------- */

/**
 * Define the number of slices per model window. E.g. a model window of 1000 ms
 * with slices per model window set to 4. Results in a slice size of 250 ms.
 * For more info: https://docs.edgeimpulse.com/docs/continuous-audio-sampling
 */
#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 4

#include <PDM.h>
#include <string.h>
#include <CASA0018_TinyML_Voice_PPT_Controller_inferencing.h>



const float NEXT_THRESHOLD = 0.50f;
const float BACK_THRESHOLD = 0.75f;
const float EXIT_THRESHOLD = 0.85f;
const float NON_COMMAND_MARGIN = 0.15f;
const float COMMAND_MARGIN = 0.10f;
const unsigned long CMD_COOLDOWN_MS = 1500;

const unsigned long BACK_DECISION_DELAY_MS = 450;


const unsigned long BACK_PENDING_TIMEOUT_MS = 1000;

unsigned long last_cmd_time = 0;

bool pending_back = false;
unsigned long pending_back_time = 0;




typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;
static signed short sampleBuffer[2048];
static bool debug_nn = false;
static volatile bool record_ready = false;


static bool print_predictions = false;



float get_score(ei_impulse_result_t *result, const char *target_label) {
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        const char *label = result->classification[ix].label;

        if (strcmp(label, target_label) == 0) {
            return result->classification[ix].value;
        }
    }

    return 0.0f;
}


float max_float(float a, float b) {
    return (a > b) ? a : b;
}


void clear_pending_back() {
    pending_back = false;
    pending_back_time = 0;
}


void emit_command(const char *cmd) {
    Serial.println(cmd);
    last_cmd_time = millis();
    clear_pending_back();
}



// PPT command decision logic


void handle_ppt_command(ei_impulse_result_t *result) {
    float exit_score = get_score(result, "exit");
    float back_score = get_score(result, "go back");
    float next_score = get_score(result, "next page");
    float noise_score = get_score(result, "noise");
    float unknown_score = get_score(result, "unknown");

    float non_command_score = max_float(noise_score, unknown_score);
    unsigned long now = millis();

 
    if (last_cmd_time != 0 && (now - last_cmd_time < CMD_COOLDOWN_MS)) {
        return;
    }


    if (next_score > 0.30 || back_score > 0.30 || exit_score > 0.30) {
    Serial.print("CMD_CHECK next=");
    Serial.print(next_score, 2);
    Serial.print(" back=");
    Serial.print(back_score, 2);
    Serial.print(" exit=");
    Serial.print(exit_score, 2);
    Serial.print(" noncmd=");
    Serial.print(non_command_score, 2);
    Serial.print(" pending_back=");
    Serial.println(pending_back ? "YES" : "NO");
}
   
    if (exit_score >= EXIT_THRESHOLD &&
        (exit_score - non_command_score) >= NON_COMMAND_MARGIN &&
        (exit_score - max_float(next_score, back_score)) >= COMMAND_MARGIN) {

        emit_command("CMD:ESC");
        return;
    }

    
    if (next_score >= NEXT_THRESHOLD &&
        (next_score - non_command_score) >= NON_COMMAND_MARGIN &&
        (next_score - max_float(back_score, exit_score)) >= COMMAND_MARGIN) {

        emit_command("CMD:NEXT");
        return;
    }

   
    if (back_score >= BACK_THRESHOLD &&
        (back_score - non_command_score) >= NON_COMMAND_MARGIN &&
        (back_score - max_float(next_score, exit_score)) >= COMMAND_MARGIN) {

        if (!pending_back) {
            pending_back = true;
            pending_back_time = now;
        }

    
        if (now - pending_back_time >= BACK_DECISION_DELAY_MS) {
            emit_command("CMD:BACK");
        }

        return;
    }

   
    if (pending_back && (now - pending_back_time >= BACK_DECISION_DELAY_MS)) {
        emit_command("CMD:BACK");
        return;
    }


    if (pending_back && (now - pending_back_time > BACK_PENDING_TIMEOUT_MS)) {
        clear_pending_back();
    }
}



// Arduino setup and loop


void setup() {
    Serial.begin(115200);

    // Wait up to 5 seconds for Serial.
    // This avoids blocking forever when running without Serial Monitor.
    unsigned long start_time = millis();
    while (!Serial && (millis() - start_time < 5000)) {
        delay(10);
    }

    Serial.println("TinyML Voice PPT Controller");
    Serial.println("Starting continuous microphone inference...");

    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        Serial.println("ERR: Could not allocate audio buffer.");
        return;
    }

    Serial.println("Microphone initialized.");
}


void loop() {
    bool m = microphone_inference_record();

    if (!m) {
        Serial.println("ERR: Failed to record audio.");
        return;
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &microphone_audio_signal_get_data;

    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);

    if (r != EI_IMPULSE_OK) {
        Serial.print("ERR: Failed to run classifier: ");
        Serial.println((int)r);
        return;
    }

    if (print_predictions) {
        Serial.println("Predictions:");
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            Serial.print("  ");
            Serial.print(result.classification[ix].label);
            Serial.print(": ");
            Serial.println(result.classification[ix].value, 5);
        }
    }

    handle_ppt_command(&result);
}


// Microphone functions


static void pdm_data_ready_inference_callback(void) {
    int bytesAvailable = PDM.available();

    int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

    if ((inference.buf_ready == 0) && (record_ready == true)) {
        for (int i = 0; i < bytesRead >> 1; i++) {
            inference.buffer[inference.buf_count++] = sampleBuffer[i];

            if (inference.buf_count >= inference.n_samples) {
                inference.buf_count = 0;
                inference.buf_ready = 1;
                break;
            }
        }
    }
}


static bool microphone_inference_start(uint32_t n_samples) {
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));

    if (inference.buffer == NULL) {
        return false;
    }

    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    PDM.onReceive(&pdm_data_ready_inference_callback);


    PDM.setBufferSize(2048);

    delay(250);

    if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
        Serial.println("ERR: Failed to start PDM.");
        microphone_inference_end();
        return false;
    }

 
    PDM.setGain(127);

    record_ready = true;

    return true;
}


static bool microphone_inference_record(void) {
    bool ret = true;

    if (inference.buf_ready == 1) {
        Serial.println("ERR: Audio buffer overrun.");
        Serial.println("Try changing EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW from 4 to 2.");
        ret = false;
    }

    while (inference.buf_ready == 0) {
        delay(1);
    }

    inference.buf_ready = 0;

    return ret;
}


static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);
    return 0;
}


static void microphone_inference_end(void) {
    PDM.end();

    if (inference.buffer != NULL) {
        free(inference.buffer);
        inference.buffer = NULL;
    }
}


#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor. This model should be a microphone audio model."
#endif