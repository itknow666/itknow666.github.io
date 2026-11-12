"""截图中的简化 LIF：先更新，再比较阈值，发放后立即清零。"""


def simulate(beta, input_increment, threshold, steps):
    # 三个检查：这些是本练习的参数范围。
    if not 0 <= beta <= 1:
        raise ValueError("beta 必须在 0 到 1 之间")
    if threshold <= 0:
        raise ValueError("threshold 必须大于 0")
    if type(steps) is not int or steps <= 0:
        raise ValueError("steps 必须是正整数")

    # 每次调用都从 0 开始，并建立自己的结果列表。
    voltage = 0.0
    spike_times = []

    for t in range(1, steps + 1):
        before_reset = beta * voltage + input_increment

        if before_reset >= threshold:
            voltage = 0.0
            spike_times.append(t)
        else:
            voltage = before_reset

    return spike_times


# 第一部分：和截图一样，显示基准实验的每一步。
beta = 0.8
input_increment = 0.3
threshold = 1.0
voltage = 0.0
spike_times = []

print("时间步  重置前电压  是否发放  重置后电压")
for t in range(1, 21):
    before_reset = beta * voltage + input_increment

    if before_reset >= threshold:
        spike = 1
        voltage = 0.0
        spike_times.append(t)
    else:
        spike = 0
        voltage = before_reset

    print(t, round(before_reset, 4), spike, round(voltage, 4))

print("发生脉冲的时间步：", spike_times)

# 第二部分：函数让我们不用复制主循环就能做对照。
print("\n参数对照（每次从 0 开始）：")
print("基准：", simulate(0.8, 0.3, 1.0, 20))
print("只改 beta=1.0：", simulate(1.0, 0.3, 1.0, 20))
print("只改 beta=0.5：", simulate(0.5, 0.3, 1.0, 20))
print("只改输入=0.5：", simulate(0.8, 0.5, 1.0, 20))

# 第三部分：两个互不连接的神经元，接收相同输入。
print("\n同一输入，两个不同阈值：")
print("A，阈值 1.0：", simulate(0.8, 0.3, 1.0, 20))
print("B，阈值 0.6：", simulate(0.8, 0.3, 0.6, 20))
