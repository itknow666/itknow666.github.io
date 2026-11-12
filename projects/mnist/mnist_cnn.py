"""CPU MNIST demo: train, save figures, and independently verify a checkpoint."""
import argparse
import csv
import json
import platform
import random
import time
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import torch
from torch import nn
from torch.utils.data import DataLoader, Subset
import torchvision
from torchvision import datasets, transforms

ROOT = Path(__file__).resolve().parent


class SmallCNN(nn.Module):
    def __init__(self):
        super().__init__()
        self.layers = nn.Sequential(
            nn.Conv2d(1, 16, 3, padding=1), nn.ReLU(), nn.MaxPool2d(2),
            nn.Conv2d(16, 32, 3, padding=1), nn.ReLU(), nn.MaxPool2d(2),
            nn.Flatten(), nn.Linear(32 * 7 * 7, 64), nn.ReLU(), nn.Linear(64, 10),
        )

    def forward(self, x):
        return self.layers(x)


def seed_all(seed):
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    torch.use_deterministic_algorithms(True)


def load_data(train):
    # [0,255] -> [0,1] -> normalized float tensor of shape [1,28,28].
    transform = transforms.Compose([
        transforms.ToTensor(), transforms.Normalize((0.1307,), (0.3081,))
    ])
    return datasets.MNIST(ROOT / "data", train=train, download=True, transform=transform)


@torch.inference_mode()
def evaluate(model, loader):
    model.eval()
    loss_sum, correct, count = 0.0, 0, 0
    for images, labels in loader:
        logits = model(images)
        loss_sum += nn.functional.cross_entropy(logits, labels, reduction="sum").item()
        correct += (logits.argmax(1) == labels).sum().item()
        count += labels.numel()
    return {"loss": loss_sum / count, "accuracy": correct / count, "samples": count}


@torch.inference_mode()
def save_predictions(model, dataset, out):
    model.eval()
    images = torch.stack([dataset[i][0] for i in range(24)])
    labels = torch.tensor([dataset[i][1] for i in range(24)])
    probabilities = model(images).softmax(1)
    confidence, predictions = probabilities.max(1)
    fig, axes = plt.subplots(4, 6, figsize=(12, 8))
    rows = []
    for i, ax in enumerate(axes.flat):
        target, pred = labels[i].item(), predictions[i].item()
        ax.imshow(images[i, 0] * 0.3081 + 0.1307, cmap="gray", vmin=0, vmax=1)
        ax.set_title(f"true={target} pred={pred}\np={confidence[i]:.2f}",
                     color="green" if target == pred else "red", fontsize=10)
        ax.axis("off")
        rows.append({"test_index": i, "target": target, "prediction": pred,
                     "confidence": confidence[i].item()})
    fig.tight_layout()
    fig.savefig(out / "predictions.png", dpi=150)
    plt.close(fig)
    (out / "predictions.json").write_text(json.dumps(rows, indent=2), encoding="utf-8")


def train(args):
    seed_all(args.seed)
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    dataset = load_data(True)
    if args.train_samples + args.val_samples > len(dataset):
        raise ValueError("train_samples + val_samples exceeds MNIST training split")
    indices = torch.randperm(len(dataset)).tolist()
    train_indices = indices[:args.train_samples]
    val_indices = indices[args.train_samples:args.train_samples + args.val_samples]
    train_loader = DataLoader(Subset(dataset, train_indices), batch_size=args.batch_size,
                              shuffle=True, num_workers=0)
    val_loader = DataLoader(Subset(dataset, val_indices), batch_size=256, num_workers=0)
    model = SmallCNN().to("cpu")
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr)
    criterion = nn.CrossEntropyLoss()
    history = []
    started = time.perf_counter()
    for epoch in range(1, args.epochs + 1):
        model.train()
        total_loss, correct, count = 0.0, 0, 0
        for images, labels in train_loader:
            optimizer.zero_grad(set_to_none=True)
            logits = model(images)
            loss = criterion(logits, labels)
            loss.backward()
            optimizer.step()
            total_loss += loss.item() * labels.numel()
            correct += (logits.argmax(1) == labels).sum().item()
            count += labels.numel()
        val = evaluate(model, val_loader)
        row = {"epoch": epoch, "train_loss": total_loss / count,
               "train_accuracy": correct / count, "val_loss": val["loss"],
               "val_accuracy": val["accuracy"]}
        history.append(row)
        print(json.dumps(row), flush=True)
    elapsed = time.perf_counter() - started
    # Test split is evaluated only after training; it never updates the model.
    test_data = load_data(False)
    test = evaluate(model, DataLoader(test_data, batch_size=256))
    config = {k: str(v) if isinstance(v, Path) else v for k, v in vars(args).items()}
    metrics = {"device": "cpu", "seed": args.seed, "config": config,
               "training_seconds": elapsed, "parameters": sum(p.numel() for p in model.parameters()),
               "test": test, "history": history,
               "environment": {"python": platform.python_version(), "torch": torch.__version__,
                               "torchvision": torchvision.__version__, "platform": platform.platform()}}
    torch.save({"state_dict": model.state_dict(), "config": config,
                "train_indices": train_indices, "val_indices": val_indices}, out / "model.pt")
    (out / "metrics.json").write_text(json.dumps(metrics, indent=2), encoding="utf-8")
    with (out / "history.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(history[0]))
        writer.writeheader()
        writer.writerows(history)
    fig, axes = plt.subplots(1, 2, figsize=(10, 4))
    for prefix in ("train", "val"):
        axes[0].plot(range(1, args.epochs + 1), [r[f"{prefix}_loss"] for r in history], "o-", label=prefix)
        axes[1].plot(range(1, args.epochs + 1), [r[f"{prefix}_accuracy"] for r in history], "o-", label=prefix)
    for ax, ylabel in zip(axes, ("Cross entropy loss", "Accuracy")):
        ax.set(xlabel="Epoch", ylabel=ylabel)
        ax.legend()
        ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out / "training_curves.png", dpi=150)
    plt.close(fig)
    save_predictions(model, test_data, out)
    print(f"Test: {test['accuracy']:.2%} ({test['samples']} images); saved to {out}")


def verify(args):
    out = args.output.resolve()
    checkpoint = torch.load(out / "model.pt", map_location="cpu", weights_only=True)
    model = SmallCNN()
    model.load_state_dict(checkpoint["state_dict"], strict=True)
    train_indices, val_indices = checkpoint["train_indices"], checkpoint["val_indices"]
    assert set(train_indices).isdisjoint(val_indices), "Training/validation overlap"
    model.eval()
    with torch.inference_mode():
        logits = model(torch.zeros(2, 1, 28, 28))
        assert logits.shape == (2, 10) and torch.isfinite(logits).all()
    actual = evaluate(model, DataLoader(load_data(False), batch_size=256))
    saved = json.loads((out / "metrics.json").read_text(encoding="utf-8"))
    assert actual["samples"] == 10000
    assert abs(actual["accuracy"] - saved["test"]["accuracy"]) < 1e-12
    assert abs(actual["loss"] - saved["test"]["loss"]) < 1e-6
    assert actual["accuracy"] >= args.min_accuracy, "Accuracy below requested threshold"
    for name in ("training_curves.png", "predictions.png"):
        image = plt.imread(out / name)
        assert image.size > 0 and np.isfinite(image).all()
    predictions = json.loads((out / "predictions.json").read_text(encoding="utf-8"))
    data = load_data(False)
    with torch.inference_mode():
        for row in predictions:
            x, target = data[row["test_index"]]
            assert target == row["target"]
            assert model(x.unsqueeze(0)).argmax(1).item() == row["prediction"]
    report = {"passed": True, "checks": ["strict checkpoint load", "disjoint training/validation",
              "finite output and shape", "full test metric reproduction", "accuracy threshold",
              "readable plots", "saved predictions reproduced"], "test": actual,
              "min_accuracy": args.min_accuracy}
    (out / "verification.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["train", "verify"])
    parser.add_argument("--output", type=Path, default=ROOT / "mnist_results")
    parser.add_argument("--epochs", type=int, default=3)
    parser.add_argument("--train-samples", type=int, default=5000)
    parser.add_argument("--val-samples", type=int, default=1000)
    parser.add_argument("--batch-size", type=int, default=64)
    parser.add_argument("--lr", type=float, default=0.001)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--threads", type=int, default=2)
    parser.add_argument("--min-accuracy", type=float, default=0.85)
    args = parser.parse_args()
    for name in ("epochs", "train_samples", "val_samples", "batch_size", "threads"):
        if getattr(args, name) <= 0:
            parser.error(f"--{name.replace('_', '-')} must be positive")
    if args.lr <= 0 or not 0 <= args.min_accuracy <= 1:
        parser.error("lr must be positive and min-accuracy must lie in [0,1]")
    torch.set_num_threads(args.threads)
    (train if args.command == "train" else verify)(args)


if __name__ == "__main__":
    main()
