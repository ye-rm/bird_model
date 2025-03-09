#include <arduinoFFT.h>

// 配置参数修改
#define SAMPLES 256            // 增加样本数以提高频率分辨率
#define SAMPLING_RATE 16000.0  // 提升采样率至16kHz
#define FRAME_LENGTH 25        // 每帧25ms
#define HOP_LENGTH 10          // 帧移10ms
#define N_MELS 40              // 梅尔带数量

float vReal[SAMPLES];  // 使用float类型数组
float vImag[SAMPLES] = { 0 };
ArduinoFFT<float> FFT(vReal, vImag, SAMPLES, SAMPLING_RATE);
float mel_bank[N_MELS][SAMPLES / 2 + 1];  // 预计算的梅尔滤波器组
const int micPin = 4;
const float pre_emphasis = 0.97;  // 预加重系数
unsigned long samplingPeriod;
// 初始化Mel滤波器组
void init_mel_filterbank() {
  float f_min = 0.0;
  float f_max = SAMPLING_RATE / 2.0;
  float mel_min = 1125.0 * log(1 + f_min / 700.0);
  float mel_max = 1125.0 * log(1 + f_max / 700.0);

  float mel_points[N_MELS + 2];
  for (int i = 0; i < N_MELS + 2; i++) {
    mel_points[i] = mel_min + (mel_max - mel_min) * i / (N_MELS + 1);
  }

  for (int m = 0; m < N_MELS; m++) {
    float left = 700 * (exp(mel_points[m] / 1125.0) - 1);
    float center = 700 * (exp(mel_points[m + 1] / 1125.0) - 1);
    float right = 700 * (exp(mel_points[m + 2] / 1125.0) - 1);

    for (int k = 0; k < SAMPLES / 2 + 1; k++) {
      float freq = (k * SAMPLING_RATE) / SAMPLES;
      if (freq < left || freq > right) {
        mel_bank[m][k] = 0.0;
      } else {
        mel_bank[m][k] = (freq <= center) ? (freq - left) / (center - left) : (right - freq) / (right - center);
      }
    }
  }
}

void init_audio() {
  Serial.begin(115200);

  // 配置ESP32 ADC
  // analogReadResolution(12);  // 使用12位分辨率
  // analogSetPinAttenuation(micPin, ADC_11db);

  // 计算采样周期
  samplingPeriod = (unsigned long)(1000000.0 / SAMPLING_RATE);

  // 初始化Mel滤波器
  init_mel_filterbank();
}

// 预加重处理
void pre_emphasis_filter(float* buffer, int size) {
  static float prev = 0.0;
  for (int i = 0; i < size; i++) {
    float current = buffer[i];
    buffer[i] = current - pre_emphasis * prev;
    prev = current;
  }
}

// 生成Mel频谱
void compute_mel_spectrogram() {
  static float power_spectrum[SAMPLES / 2 + 1];

  // 计算功率谱
  for (int k = 0; k < SAMPLES / 2 + 1; k++) {
    power_spectrum[k] = vReal[k] * vReal[k];
  }

  // 应用Mel滤波器组
  float mel_spectrum[N_MELS] = { 0 };
  for (int m = 0; m < N_MELS; m++) {
    for (int k = 0; k < SAMPLES / 2 + 1; k++) {
      mel_spectrum[m] += power_spectrum[k] * mel_bank[m][k];
    }
    mel_spectrum[m] = log(mel_spectrum[m] + 1e-6);  // 对数压缩
  }

  // 输出供AI模型使用
  Serial.print("AI_INPUT:");
  for (int m = 0; m < N_MELS; m++) {
    Serial.print(mel_spectrum[m], 2);
    Serial.print(",");
  }
  Serial.println();
}

void process_loop() {
  // 采集原始音频数据
  unsigned long startTime = micros();
  for (int i = 0; i < SAMPLES; i++) {
    vReal[i] = (float)analogRead(micPin) - 2048.0;  // 12位ADC中心值偏移
    vImag[i] = 0.0;
    while (micros() - startTime < i * samplingPeriod)
      ;
  }

  // 预处理流程
  pre_emphasis_filter(vReal, SAMPLES);  // 预加重
  FFT.dcRemoval(vReal, SAMPLES);        // 直流偏移消除
  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  FFT.compute(FFTDirection::Forward);
  FFT.complexToMagnitude();

  // 生成Mel频谱
  compute_mel_spectrogram();

  // 控制处理速率
  delay(FRAME_LENGTH - HOP_LENGTH);  // 根据帧移等待
}