# -*- coding: utf-8 -*-
# @Time    : 
# @Author  : bingbing
# @File    : train2.py
from ultralytics import YOLO
import torch
import torch.multiprocessing as mp

# 多进程兼容，容器/Windows打包必备
mp.freeze_support()

# 1. 自动分配训练设备
device = 0 if torch.cuda.is_available() else 'cpu'
print(f"训练设备：{device}")

# 2. 加载预训练权重（根据显存切换n/s/m）
# 6G显存：yolov8n.pt | 12G显存：yolov8s.pt | 24G+：yolov8m.pt
model = YOLO('yolov8n.pt')

# 3. 修复参数后的训练配置
results = model.train(
    # 基础路径配置
    data='/home/ma-user/work/data.yaml',
    plots=False,  # 关闭绘图，彻底解决中文标签编码报错
    project='tongue_coating_train',
    name='tongue_coating_model_v2',
    # 训练轮次&保存策略
    epochs=100,
    save=True,
    save_period=10,
    patience=20,
    pretrained=True,
    # 硬件与输入尺寸
    imgsz=640,
    batch=16,  # 显存不足改为8/4
    device=device,
    workers=2,
    mosaic=1.0,
    # 学习率优化（解决mAP震荡）
    lr0=0.0008,
    optimizer='AdamW',
    cos_lr=True,
    warmup_epochs=3,
    # 损失权重平衡，提升mAP50-95
    box=7.5,
    cls=0.5,
    dfl=1.5,
    # 数据增强
    hsv_h=0.015,
    hsv_s=0.7,
    hsv_v=0.4,
    degrees=10.0,
    perspective=0.001,
    flipud=0.2,
    fliplr=0.5,
    # NMS、置信阈值
    conf=0.25,
    iou=0.7,
    rect=False,
    # 修复：替换multi_scale=True为scale=0.5 开启多尺度训练
    scale=0.5
)

# 训练完成打印最优权重路径
print(f"训练完成，最优模型保存路径：{results.save_dir}/weights/best.pt")