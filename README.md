---
author: [Even]
date: [2024年12月31日]
update: [2024年01月07日]
title: [【ESP32-WiFi学习】任务目标]
tags: [ESP32,WiFi,ESP-IDF]
---

# 前言
从元件箱中翻出了一个ESP32,在电赛之后未再学过ESP相关的内容。当前ESP32已经可以接入LLM作为语音助手，充分利用其WiFi联网与蓝牙优势。因此想认真学习一下ESP的WiFi联网功能。

# 项目链接

github: https://github.com/even904/esp32.git
Document: https://even904.github.io/CloudDoc/

# 任务目标

作为任务完成的标志，我将尝试完成一个WiFi联网天气时钟，应当具有如下基本功能:
- 接入家用WiFi联网
- 屏幕显示
- 实时时钟
- 倒计时

其他功能后续视情况添加。

# 完成表
- [x] 接入家用WiFi联网
- [x] 屏幕显示
- [x] 实时时钟
- [ ] 倒计时（取消，因为没有交互外设，使用不便）

***

- [x] 网络天气
- [ ] 硬件一体化
- [ ] 外壳设计与封装

# 研究方法与内容
了解ESP32WiFi工作原理。WiFi作为一种通信方式，其标准由IEEE规定。ESP32这种低功耗IoT应用场景，以2.4GHz作为主要频率。ESP32设计支持的WiFi标准是IEEE 802.11 b/g/n。三个标准的主要区别在于调制方式(IR/DSSS/CCK/FHSS/OFDM)、编码方式(64-QAM)、信道带宽和理论速率。[这篇文章](https://www.shuzixingkong.net/article/1653)作了详细介绍。

WiFi作为网络基础服务，应当按照网络系统的模型来研究。
```mermaid
flowchart LR
    物理层-->数据链路层-->网络层-->传输层-->会话层-->表示层-->应用层
```
由于不可能对每一个标准以及层级都进行详细研究，因此需要选取其中最关键的部分。

在这个[项目](【ESP32-WiFi学习】任务目标.md)中，需要实现任务目标的同时构建更稳健的代码，以便在各种网络情况下程序的正常运行，这要求程序具有对WiFi通信的错误处理机制。硬件驱动层和应用层，乐鑫以及开源社区已经提供了专门的函数库。
要实现更健壮的通信机制，关键部分在于深入理解数据链路层及其协议，也就是理解其数据帧及管理帧。了解WiFi通信如何通过管理帧进行错误处理，也就是在遇到问题时会发送什么类型的帧内容，是构建更健壮的通信机制的基础。

在本项目中，应用层的主要工作是在客户端和服务器之间建立http/https连接，然后使用json传输和解析数据。

将配置信息保存在NVS中，在线重启或是硬件复位重启后可以调用配置信息执行天气获取等操作。

# 打赏☕
如果您觉得本项目对您有用，可以打赏一下！期待您的留言~

|WechatPay|Alipay|
|---|---|
|<img src="https://raw.githubusercontent.com/even904/Images/main/pic/WechatPay.png" alt="" height="199">|<img src="https://raw.githubusercontent.com/even904/Images/main/pic/Alipay.jpeg" alt="" height="200">|