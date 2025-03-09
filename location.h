#include <SoftwareSerial.h>

// 引脚定义
const int readPin = 21;   // 模块TX接Arduino RX
const int writePin = 20;  // 模块RX接Arduino TX
const int statusPin = 19;
const int wakePin = 47;

EspSoftwareSerial::UART gpsSerial;  // 创建软串口实例

bool set_working_status() {
  // 设置引脚模式

  pinMode(readPin, INPUT);
  pinMode(writePin, OUTPUT);
  pinMode(statusPin, INPUT);
  pinMode(wakePin, OUTPUT);

  // 启动模块
  digitalWrite(wakePin, HIGH);

  // 初始化串口通信
  gpsSerial.begin(9600, SWSERIAL_8N1, readPin, writePin, false);  // 根据模块实际波特率调整

  return true;
}

bool check_status_wave() {
  // 检测两次上升沿的时间间隔
  unsigned long firstRising = pulseIn(statusPin, HIGH, 1500000);  // 等待1.5秒
  if (firstRising == 0) return false;

  unsigned long secondRising = pulseIn(statusPin, HIGH, 1500000);
  if (secondRising == 0) return false;

  // 计算总周期（高电平时间 + 低电平时间）
  unsigned long highDuration = pulseIn(statusPin, HIGH, 1500000);
  unsigned long lowDuration = pulseIn(statusPin, LOW, 1500000);
  if (highDuration == 0 || lowDuration == 0) return false;

  unsigned long period = highDuration + lowDuration;
  return (period > 900000 && period < 1100000);  // 允许±10%误差
}

void send_command(const char* command) {
  gpsSerial.println(command);  // 发送命令并换行
  delay(10);                   // 保证命令完整发送
}

void check_read_buffer() {
  if (gpsSerial.available() > 0) {
    // TODO: 这里添加后续数据处理逻辑
    String rawData = gpsSerial.readStringUntil('\n');
    Serial.print("[TODO] Received data: ");
    Serial.println(rawData);
  }else{
    // Serial.println("No data received");
  }
}