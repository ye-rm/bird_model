#include "sqlite3_n.h"
#include "audio.h"
#include "location.h"
void setup() {
  Serial.begin(115200);
  // if (!init_db()) {
  //   Serial.println("Database initialization failed!");
  //   return;
  // }

  // // 插入示例记录
  // int result = insert_record(
  //   "Corvus corax",
  //   -122.123456,
  //   37.123456,
  //   "2023-10-05 14:30:00",
  //   0.95);

  // if (result == SQLITE_DONE) {
  //   Serial.println("Insert successful");
  // }
  init_audio();
  set_working_status();
}

void loop() {
  // process_loop();
  check_read_buffer();
}
