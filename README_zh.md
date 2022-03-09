# device_soc_st

-   [简介](#section469617221261)
-   [目录](#section469617221262)
-   [约束](#section469617221263)
-   [使用说明](#section469617221264)
-   [相关仓](#section469617221265)

## 简介<a name="section469617221261"></a>

该仓库为意法半导体(STMicroelectronics)芯片相关代码仓库, 用于存放与SOC芯片紧密相关的代码，不同厂商使用同一款soc，开发不同的device/board时，可共用该仓库代码进行开发。

## 目录<a name="section469617221262"></a>

```
device/soc/st

├── common                                  # 公共驱动
│   └── hal                                 # hal适配目录
│   └── platform                            # 平台驱动适配
├── stm32mp157                              # 芯片SOC名称
├── stm32f4xx                               # 芯片SOC名称
```

## 约束<a name="section469617221263"></a>
- 支持STM32MP157芯片

- 支持STM32F407xx芯片

## 使用说明<a name="section469617221264"></a>

参考各开发板readme。
## 相关仓<a name="section469617221265"></a>

- [device_soc_st](https://gitee.com/openharmony-sig/device_soc_st)
