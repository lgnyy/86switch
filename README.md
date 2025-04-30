# 86switch
详细介绍参考 [Go DeepWiki](https://deepwiki.com/lgnyy/86switch)

## 编译
运行"ESP-IDF 5.3 CMD"
```sh
git clone https://github.com/lgnyy/86switch.git
cd 86switch
idf.py set-target esp32s3
idf.py build
```

## 功能说明

### 1. 智能开关控制
- 支持通过 Wi-Fi 进行远程控制
- 支持本地物理按键控制

### 2. 语音识别
- 内置 INMP441 麦克风
- 支持语音指令控制开关
- 支持小米物联网，可以向 Wi-Fi 音箱发送控制命令

### 3. 显示界面
- 使用 LVGL9 库进行图形界面开发
- 提供实时状态显示和控制界面

### 4. 硬件配置
- 基于 ESP32-S3 芯片
- 支持多种传感器和外设扩展

