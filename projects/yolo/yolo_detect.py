"""Ultralytics official pretrained YOLO example: single-image CPU detection."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import platform
import shutil

from PIL import Image
import torch
import ultralytics
from ultralytics import YOLO
from ultralytics.utils import ASSETS

ROOT = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, help="Your local image; default: official bus.jpg")
    parser.add_argument("--output", type=Path, default=ROOT / "yolo_results")
    parser.add_argument("--conf", type=float, default=0.25, help="Minimum confidence, 0 to 1")
    parser.add_argument("--imgsz", type=int, default=640)
    args = parser.parse_args()
    if not 0 <= args.conf <= 1 or args.imgsz <= 0 or args.imgsz % 32:
        parser.error("conf must be in [0,1]; imgsz must be a positive multiple of 32")
    if args.source is None:
        # The pip package ships the official bus example, so no image URL is needed.
        source = ROOT / "yolo_inputs" / "bus.jpg"
        source.parent.mkdir(parents=True, exist_ok=True)
        if not source.exists():
            shutil.copy2(ASSETS / "bus.jpg", source)
    else:
        source = args.source.resolve()
    if not source.is_file():
        parser.error(f"Image does not exist: {source}")
    # Restrict this demo to one readable still image.
    with Image.open(source) as image:
        image.verify()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    torch.set_num_threads(2)
    weights = ROOT / "yolo_weights" / "yolo26n.pt"
    weights.parent.mkdir(parents=True, exist_ok=True)
    model = YOLO(str(weights))  # Downloads the official pretrained weights on first run.
    results = model.predict(source=str(source), device="cpu", imgsz=args.imgsz,
                            rect=False, conf=args.conf, save=False, verbose=True)
    result = results[0]  # One Results object for this one image.
    height, width = result.orig_shape
    detections = []
    for box in result.boxes:
        class_id = int(box.cls.item())
        score = float(box.conf.item())
        xyxy = [float(v) for v in box.xyxy[0].tolist()]
        # Coordinates refer to the original image, not the resized network input.
        x1, y1, x2, y2 = xyxy
        assert all(math.isfinite(v) for v in xyxy + [score])
        assert 0 <= score <= 1 and score + 1e-6 >= args.conf
        assert 0 <= x1 < x2 <= width and 0 <= y1 < y2 <= height
        detections.append({"class_id": class_id, "class_name": result.names[class_id],
                           "confidence": score, "xyxy": xyxy})
    annotated = out / "detected.jpg"
    result.save(filename=str(annotated), labels=True, conf=True, line_width=2)
    # Actually reopen the saved image and check that it retained the original size.
    with Image.open(annotated) as image:
        image.load()
        assert image.size == (width, height)
    report = {
        "source": str(source), "weights": str(weights), "device": "cpu",
        "image_size": {"width": width, "height": height},
        "network_input": [1, 3, args.imgsz, args.imgsz],
        "confidence_threshold": args.conf, "count": len(detections),
        "class_counts": dict(Counter(d["class_name"] for d in detections)),
        "detections": detections, "speed_ms": result.speed,
        "annotated_image": str(annotated),
        "verification": {"saved_image_readable": True, "original_size_preserved": True,
                         "box_coordinates_and_scores_valid": True},
        "environment": {"python": platform.python_version(), "torch": torch.__version__,
                        "ultralytics": ultralytics.__version__, "platform": platform.platform()},
    }
    (out / "detections.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"Detected: {report['class_counts']}")
    for detection in detections:
        print(f"  {detection['class_name']}: {detection['confidence']:.3f}, xyxy={detection['xyxy']}")
    print(f"Saved image: {annotated}\nSaved data: {out / 'detections.json'}")


if __name__ == "__main__":
    main()
