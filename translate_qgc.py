# -*- coding: utf-8 -*-
"""QGC Full Chinese Translation - Compositional approach."""
import xml.etree.ElementTree as ET
import re, sys, copy

# Comprehensive phrase dictionary - longer phrases first for greedy matching
# Format: (english_phrase, chinese_translation)
PHRASES = sorted([
    # === Multi-word technical phrases ===
    ("angle controller P gain", "角度控制器P增益"),
    ("rate controller P gain", "速率控制器P增益"),
    ("rate controller I gain", "速率控制器I增益"),
    ("rate controller D gain", "速率控制器D增益"),
    ("rate controller", "速率控制器"),
    ("angle controller", "角度控制器"),
    ("flight controller", "飞控"),
    ("flight mode", "飞行模式"),
    ("flight plan", "飞行计划"),
    ("remote control", "遥控器"),
    ("ground station", "地面站"),
    ("ground speed", "地速"),
    ("air speed", "空速"),
    ("battery cell", "电池单元"),
    ("battery voltage", "电池电压"),
    ("battery current", "电池电流"),
    ("battery level", "电池电量"),
    ("power module", "电源模块"),
    ("voltage divider", "分压器"),
    ("voltage multiplier", "电压倍增器"),
    ("current sensor", "电流传感器"),
    ("amps per volt", "安培/伏特"),
    ("serial port", "串口"),
    ("baud rate", "波特率"),
    ("data bits", "数据位"),
    ("stop bits", "停止位"),
    ("flow control", "流控"),
    ("video stream", "视频流"),
    ("video source", "视频源"),
    ("file path", "文件路径"),
    ("file name", "文件名"),
    ("file size", "文件大小"),
    ("disk size", "磁盘大小"),
    ("memory size", "内存大小"),
    ("save path", "保存路径"),
    ("log file", "日志文件"),
    ("log files", "日志文件"),
    ("color scheme", "配色方案"),
    ("dark mode", "深色模式"),
    ("font size", "字体大小"),
    ("ui scale", "界面缩放"),
    ("debug level", "调试级别"),
    ("clear settings", "清除设置"),
    ("factory reset", "恢复出厂设置"),
], key=lambda x: -len(x[0]))

# Single word translations
WORDS = {
