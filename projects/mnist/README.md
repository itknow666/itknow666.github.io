# MNIST 手写数字识别

使用 PyTorch 小型 CNN，在 CPU 上进行训练和验证。2026-10-18 的记录中，随机选择 5,000 张训练、1,000 张验证，训练 3 轮，在 MNIST 10,000 张测试图像上取得 95.87%（9,587/10,000）；模型约 105,866 个参数。训练、数据集与测试集的区分及曲线见 `results/metrics.json` 和 `results/training_curves.png`。

`mnist_cnn.py` 为训练与验证入口，`results/model.pt` 为该次保存的模型权重。需先安装 PyTorch、torchvision 和 matplotlib；脚本首次运行可能下载公开 MNIST 数据。请阅读代码中的参数后运行，不要把这次小实验当作其他任务的性能。

该工程由学习者与 AI 辅助协作整理；公开记录保留原始度量，路径字段已移除。
