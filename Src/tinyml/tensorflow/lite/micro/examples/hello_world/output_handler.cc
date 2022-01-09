/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/lite/micro/examples/hello_world/output_handler.h"
#include "fw.h"
#include "DispInterface.h"
#include "fw_macro.h"

using namespace APP;

static uint32_t testCnt = 0;

void HandleOutput(tflite::ErrorReporter* error_reporter, float x_value,
                  float y_value) {
  // Log the current X and Y values
  TF_LITE_REPORT_ERROR(error_reporter, "x_value: %f, y_value: %f\n",
                       static_cast<double>(x_value),
                       static_cast<double>(y_value));

  const uint16_t kSize = 10;
  if (x_value < 0.01) {
      testCnt++;
  }
  uint16_t x_pos = (x_value / (2*3.1416)) * (320 - kSize);
  uint16_t y_pos = ((y_value + 1.0) / 2) * (240 - kSize);
  // x and y are transposed.
  Evt *evt = new DispDrawRectReq(ILI9341, CONSOLE, 0, y_pos, x_pos, kSize, kSize,
                               (testCnt)%2 ? COLOR24_YELLOW:COLOR24_WHITE);
  Fw::Post(evt);
}
