/*
 * @Description: Download packages from the Internet
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2024-06-08 16:18:12
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-06-19 18:13:42
 * @License: GPL 3.0
 */
#include <WiFi.h>
#include <HTTPClient.h>
#include <FS.h>

// 替换以下信息为你的Wi-Fi网络信息
const char *ssid = "xinyuandianzi";
const char *password = "AA15994823428";
// const char *ssid = "LilyGo-AABB";
// const char *password = "xinyuandianzi";

// 文件下载链接
// const char *fileDownloadUrl = "https://code.visualstudio.com/docs/?dv=win64user";//vscode
// const char *fileDownloadUrl = "https://dldir1.qq.com/qqfile/qq/PCQQ9.7.17/QQ9.7.17.29225.exe"; // 腾讯CDN加速
// const char *fileDownloadUrl = "https://cd001.www.duba.net/duba/install/packages/ever/kinsthomeui_150_15.exe"; // 金山毒霸
const char *fileDownloadUrl = "http://music.163.com/song/media/outer/url?id=26122999.mp3";
// const char *fileDownloadUrl = "http://vipspeedtest8.wuhan.net.cn:8080/download?size=1073741824";//1GB（电信）

static size_t CycleTime = 0;

void setup()
{
    Serial.begin(115200);

    // 连接到Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Successfully connected to WiFi");

    // 初始化HTTP客户端
    HTTPClient http;
    http.begin(fileDownloadUrl);
    // 获取重定向的URL
    const char *headerKeys[] = {"Location"};
    http.collectHeaders(headerKeys, 1);

    // 记录下载开始时间
    size_t startTime = millis();
    // 无用时间
    size_t uselessTime = 0;

    // 发起GET请求
    int httpCode = http.GET();

    while (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND)
    {
        String newUrl = http.header("Location");
        Serial.printf("Redirecting to: %s\n", newUrl.c_str());
        http.end(); // 关闭旧的HTTP连接

        // 使用新的URL重新发起GET请求
        http.begin(newUrl);
        httpCode = http.GET();
    }

    if (httpCode == HTTP_CODE_OK)
    {
        // 获取文件大小
        size_t fileSize = http.getSize();
        Serial.printf("Starting file download...\n");
        Serial.printf("file size: %f MB\n", fileSize / 1024.0 / 1024.0);

        // 读取HTTP响应
        WiFiClient *stream = http.getStreamPtr();

        size_t temp_count_s = 0;
        size_t temp_fileSize = fileSize;
        uint8_t *buf_1 = (uint8_t *)heap_caps_malloc(100 * 1024, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        // uint8_t buf_1[4096] = {0};
        CycleTime = millis() + 1000; // 开始计时
        while (http.connected() && (fileSize > 0 || fileSize == -1))
        {
            // 获取可用数据的大小
            size_t availableSize = stream->available();
            if (availableSize)
            {
                temp_fileSize -= stream->readBytes(buf_1, min(availableSize, (size_t)(100 * 1024)));

                if (millis() > CycleTime)
                {
                    size_t temp_time_1 = millis();
                    temp_count_s++;
                    Serial.printf("Download speed: %f KB/s\n", ((fileSize - temp_fileSize) / 1024.0) / temp_count_s);
                    Serial.printf("Remaining file size: %f MB\n\n", temp_fileSize / 1024.0 / 1024.0);

                    CycleTime = millis() + 1000;
                    size_t temp_time_2 = millis();

                    uselessTime = uselessTime + (temp_time_2 - temp_time_1);
                }
            }
            // delay(1);

            if (temp_fileSize == 0)
            {
                break;
            }
        }

        // 关闭HTTP客户端
        http.end();

        // 记录下载结束时间并计算总花费时间
        size_t endTime = millis();
        Serial.printf("Download completed!\n");
        Serial.printf("Total download time: %f s\n", (endTime - startTime - uselessTime) / 1000.0);
        Serial.printf("Average download speed: %f KB/s\n", (fileSize / 1024.0) / ((endTime - startTime - uselessTime) / 1000.0));
    }
    else
    {
        Serial.printf("Failed to download\n");
        Serial.printf("Error httpCode: %d \n", httpCode);
    }
}

void loop()
{
    // 这里不需要执行任何操作，因为所有的工作都在setup函数中完成
}