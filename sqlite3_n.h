#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <SPI.h>
#include <FS.h>
#include "SD.h"

#define DB_PATH "/sd/bird.db"  // 确保路径与SD卡挂载点匹配

sqlite3 *db = nullptr;

int sck = 12;
int miso = 14;
int mosi = 13;
int cs = 11;

const char *data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName) {
  int i;
  Serial.printf("%s: ", (const char *)data);
  for (i = 0; i < argc; i++) {
    Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  Serial.printf("\n");
  return 0;
}

// 初始化数据库连接
int openDb(const char *filename, sqlite3 **db) {
  int rc = sqlite3_open(filename, db);
  if (rc != SQLITE_OK) {
    Serial.printf("Database opening failed: %s\n", sqlite3_errmsg(*db));
    sqlite3_close(*db);
    *db = nullptr;
  } else {
    Serial.printf("Database connection established\n");
  }
  return rc;
}

char *zErrMsg = 0;
int db_exec(const char *sql) {
  Serial.println(sql);
  long start = micros();
  int rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    Serial.printf("Operation done successfully\n");
  }
  Serial.print(F("Time taken:"));
  Serial.println(micros() - start);
  return rc;
}

// 数据库初始化流程
bool init_db() {
  // 挂载存储设备
  SPI.begin(sck, miso, mosi, cs);
  if (!SD.begin(cs)) {
    Serial.println("SD card mount failed");
    return false;
  }

  // 初始化SQLite引擎
  if (sqlite3_initialize() != SQLITE_OK) {
    Serial.println("SQLite initialization failed");
    return false;
  }

  // 打开/创建数据库
  if (openDb(DB_PATH, &db) != SQLITE_OK) {
    return false;
  }

  // 创建表结构
  const char *createTableSQL =
    "CREATE TABLE IF NOT EXISTS birdrecord ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "scientific_name TEXT NOT NULL,"
    "longitude REAL NOT NULL CHECK(longitude BETWEEN -180 AND 180),"
    "latitude REAL NOT NULL CHECK(latitude BETWEEN -90 AND 90),"
    "utc_time DATETIME NOT NULL,"
    "confidence REAL NOT NULL CHECK(confidence BETWEEN 0 AND 1)"
    ");";

  int rc = db_exec(createTableSQL);
  if (rc != SQLITE_OK) {
    Serial.println("Table creation failed");
    sqlite3_close(db);
    db = nullptr;
    return false;
  }

  return true;
}

// 安全插入函数
int insert_record(const char *scientific_name, double longitude, double latitude,
                  const char *utc_time, float confidence) {
  // 参数有效性验证
  if (!db) {
    Serial.println("Database not initialized");
    return SQLITE_ERROR;
  }

  // 构建参数化SQL语句
  const char *sql =
    "INSERT INTO birdrecord "
    "(scientific_name, longitude, latitude, utc_time, confidence) "
    "VALUES (?, ?, ?, ?, ?);";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    Serial.printf("Prepare failed: %s\n", sqlite3_errmsg(db));
    return rc;
  }

  // 绑定参数
  sqlite3_bind_text(stmt, 1, scientific_name, -1, SQLITE_STATIC);
  sqlite3_bind_double(stmt, 2, longitude);
  sqlite3_bind_double(stmt, 3, latitude);
  sqlite3_bind_text(stmt, 4, utc_time, -1, SQLITE_STATIC);
  sqlite3_bind_double(stmt, 5, confidence);

  // 执行插入
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("Execution failed: %s\n", sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
  return rc;
}