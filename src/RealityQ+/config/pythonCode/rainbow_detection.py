import math
import os
import time
import warnings
import tkinter as tk
from tkinter import filedialog, simpledialog
import cv2
import matplotlib.pyplot as plt
import numpy as np
from PIL import Image
import  re
from pathlib import Path
import pandas as pd
import logging
import csv
from collections import defaultdict

logger = logging.getLogger()
logger.setLevel(logging.INFO)
fmt = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s")
warnings.filterwarnings("ignore", category=Image.DecompressionBombWarning)
plt.rcParams["font.sans-serif"] = ["SimHei", "Microsoft YaHei", "Arial Unicode MS"]
plt.rcParams["axes.unicode_minus"] = False
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
# External-use fixed mask path. Update this path directly after copying the script.
MASK_PATH = r"D:\project\test_png\mask_union.png"

SCORE_PERCENTILE = 99.9
MORPH_KERNEL_SIZE = 3
MIN_COMPONENT_AREA = 40
LIGHT_SOURCE_THRESHOLD = 150
ROI_PADDING = 100
ROI_MIN_NONZERO_PIXELS = 80
ROI_MIN_LARGEST_COMPONENT_FRACTION = 0.60
CENTER=[5632,4600]
PIXELSIZE=3.2e-3
FOCALLENGTH=13.32
IMGSIZE=[11264,9200 ]
DEBUG=False
IsSaveNpyFile=True
def resolve_mask_path():
    if os.path.isfile(MASK_PATH):
        return MASK_PATH
    raise FileNotFoundError(f"固定 mask 路径不存在，请修改脚本顶部的 MASK_PATH: {MASK_PATH}")


def generate_directional_images(image):
    image_float = np.float32(image)
    dft = cv2.dft(image_float, flags=cv2.DFT_COMPLEX_OUTPUT)
    dft_shift = np.fft.fftshift(dft)
    rows, cols = image.shape
    crow, ccol = rows // 2, cols // 2

    yy, xx = np.indices((rows, cols), dtype=np.float32)
    dy = yy - crow
    dx = xx - ccol
    dist = np.sqrt(dx * dx + dy * dy)
    point_angle = np.arctan2(dy, dx)

    radial_mask = (dist >= 20) & (dist <= 120)
    directions = [0, 45, 90, 135]
    directional_images = []

    for theta_deg in directions:
        theta = np.radians(theta_deg)
        angle_diff = np.abs(point_angle - theta)
        angle_diff = np.minimum(angle_diff, 2 * np.pi - angle_diff)
        angular_mask = angle_diff < np.radians(30)
        mask_2d = radial_mask & angular_mask

        mask = np.zeros((rows, cols, 2), np.float32)
        mask[mask_2d] = 1.0

        fshift = dft_shift * mask
        f_ishift = np.fft.ifftshift(fshift)
        img_back = cv2.idft(f_ishift)
        img_back = cv2.magnitude(img_back[:, :, 0], img_back[:, :, 1])
        img_back = cv2.normalize(img_back, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)
        directional_images.append(img_back)

    return directional_images
def calculate_light_source_position(theta_deg, phi_deg, image_shape):
    rows, cols = image_shape
    center_y, center_x = rows // 2, cols // 2

    theta_rad = np.radians(theta_deg)
    phi_rad = np.radians(phi_deg)

    max_radius = min(rows, cols) // 2
    radius = max_radius * np.sin(theta_rad)

    x = center_x + radius * np.cos(phi_rad)
    y = center_y - radius * np.sin(phi_rad)

    x = np.clip(x, 0, cols - 1)
    y = np.clip(y, 0, rows - 1)
    return int(x), int(y)
def load_fixed_mask(mask_path, image_shape):
    mask = cv2.imread(mask_path, cv2.IMREAD_GRAYSCALE)
    if mask is None:
        raise FileNotFoundError(f"无法读取固定 mask: {mask_path}")

    rows, cols = image_shape[:2]
    if mask.shape != (rows, cols):
        mask = cv2.resize(mask, (cols, rows), interpolation=cv2.INTER_NEAREST)

    return mask > 0
def refine_candidate_mask(binary_mask):
    mask_uint8 = binary_mask.astype(np.uint8) * 255
    kernel = cv2.getStructuringElement(
        cv2.MORPH_ELLIPSE, (MORPH_KERNEL_SIZE, MORPH_KERNEL_SIZE)
    )
    refined = cv2.morphologyEx(mask_uint8, cv2.MORPH_CLOSE, kernel)
    refined = cv2.morphologyEx(refined, cv2.MORPH_OPEN, kernel)

    num_labels, labels, stats, _ = cv2.connectedComponentsWithStats(refined)
    cleaned = np.zeros_like(refined)
    for label_idx in range(1, num_labels):
        area = stats[label_idx, cv2.CC_STAT_AREA]
        if area >= MIN_COMPONENT_AREA:
            cleaned[labels == label_idx] = 255
    return cleaned > 0
def calculate_binary_component_metrics(binary_mask):
    mask_uint8 = binary_mask.astype(np.uint8)
    # plt.imshow(mask_uint8)
    # plt.show()
    num_labels, labels, stats, _ = cv2.connectedComponentsWithStats(mask_uint8)
    component_areas = [
        int(stats[label_idx, cv2.CC_STAT_AREA]) for label_idx in range(1, num_labels)
    ]
    component_areas.sort(reverse=True)
    total_pixels = int(np.sum(component_areas))
    largest_component = int(component_areas[0]) if component_areas else 0
    largest_fraction = (
        float(largest_component / total_pixels) if total_pixels > 0 else 0.0
    )
    return {
        "component_count": len(component_areas),
        "nonzero_pixels": total_pixels,
        "largest_component_pixels": largest_component,
        "largest_component_fraction": largest_fraction,
    }

def merge_overlapping_rois(rois):
    merged = True
    while merged:
        merged = False
        for i in range(len(rois)):
            for j in range(i + 1, len(rois)):
                roi1 = rois[i]
                roi2 = rois[j]
                overlap = not (
                    roi1["right"] < roi2["left"]
                    or roi1["left"] > roi2["right"]
                    or roi1["bottom"] < roi2["top"]
                    or roi1["top"] > roi2["bottom"]
                )
                if not overlap:
                    continue

                new_rois = []
                for k, roi in enumerate(rois):
                    if k != i and k != j:
                        new_rois.append(roi)
                new_rois.append(
                    {
                        "left": min(roi1["left"], roi2["left"]),
                        "top": min(roi1["top"], roi2["top"]),
                        "right": max(roi1["right"], roi2["right"]),
                        "bottom": max(roi1["bottom"], roi2["bottom"]),
                        "regions": roi1["regions"] + roi2["regions"],
                    }
                )
                rois = new_rois
                merged = True
                break
            if merged:
                break
    return rois
def build_score_map(directional_images):
    stack = np.stack(directional_images, axis=0).astype(np.float32)
    min_map = np.min(stack, axis=0)
    max_map = np.max(stack, axis=0)
    mean_map = np.mean(stack, axis=0)
    ratio_map = min_map / np.maximum(max_map, 1e-6)
    score_map = mean_map * ratio_map
    return score_map, ratio_map, mean_map

def build_score_candidate_mask(score_map, fixed_mask):
    valid_values = score_map[fixed_mask]
    if valid_values.size == 0:
        raise ValueError("固定 mask 内没有可用像素，无法计算分位数阈值。")

    score_threshold = float(np.percentile(valid_values, SCORE_PERCENTILE))
    raw_candidate = np.logical_and(score_map >= score_threshold, fixed_mask)
    refined_candidate = refine_candidate_mask(raw_candidate)
    return score_threshold, raw_candidate, refined_candidate
def normalize_to_u8(image):
    return cv2.normalize(image, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)


def rainbowDetection1(img, theta_deg, phi_deg,waveLen):
    raw_image = np.array(img)
    mask_path=MASK_PATH
    fixed_mask = load_fixed_mask(mask_path, raw_image.shape)
    masked_image = np.where(fixed_mask, raw_image, 0)
    image_8bit = normalize_to_u8(masked_image)
    light_x, light_y = calculate_light_source_position(theta_deg, phi_deg, raw_image.shape)
    directional_images = generate_directional_images(raw_image)
    directional_images = [np.where(fixed_mask, item, 0).astype(np.uint8) for item in directional_images]
    score_map, ratio_map, mean_map = build_score_map(directional_images)
    score_threshold, raw_candidate, refined_candidate = build_score_candidate_mask(score_map, fixed_mask)
    raw_candidate_u8 = raw_candidate.astype(np.uint8) * 255
    refined_candidate_u8 = refined_candidate.astype(np.uint8) * 255
    num_labels, labels, stats, _ = cv2.connectedComponentsWithStats(refined_candidate_u8, 8)
    all_regions = []
    for label_idx in range(1, num_labels):
        area = stats[label_idx, cv2.CC_STAT_AREA]
        if area < MIN_COMPONENT_AREA:
            continue
        left = stats[label_idx, cv2.CC_STAT_LEFT]
        top = stats[label_idx, cv2.CC_STAT_TOP]
        width = stats[label_idx, cv2.CC_STAT_WIDTH]
        height = stats[label_idx, cv2.CC_STAT_HEIGHT]
        region = {
            "label": len(all_regions) + 1,
            "left": left,
            "top": top,
            "width": width,
            "height": height,
            "area": area,
        }
        all_regions.append(region)

    all_regions_marked = cv2.cvtColor(image_8bit, cv2.COLOR_GRAY2BGR)
    rois = []
    for region in all_regions:
        left = region["left"]
        top = region["top"]
        width = region["width"]
        height = region["height"]
        rois.append(
            {
                "left": max(0, left - ROI_PADDING),
                "top": max(0, top - ROI_PADDING),
                "right": min(image_8bit.shape[1], left + width + ROI_PADDING),
                "bottom": min(image_8bit.shape[0], top + height + ROI_PADDING),
                "regions": [region["label"]],
            }
        )
    rois = merge_overlapping_rois(rois)

    selected_roi_indices = []
    roi_analysis_results = []
    for idx, roi in enumerate(rois):
        roi_score = score_map[roi["top"] : roi["bottom"], roi["left"] : roi["right"]]
        roi_ratio = ratio_map[roi["top"] : roi["bottom"], roi["left"] : roi["right"]]
        roi_mean = mean_map[roi["top"] : roi["bottom"], roi["left"] : roi["right"]]
        roi_center_x = (roi["left"] + roi["right"]) // 2
        roi_center_y = (roi["top"] + roi["bottom"]) // 2
        light_distance = math.sqrt((roi_center_x - light_x) ** 2 + (roi_center_y - light_y) ** 2)
        is_light_source = light_distance < LIGHT_SOURCE_THRESHOLD
        metrics = {
            "score_mean": float(np.mean(roi_score)) if roi_score.size else 0.0,
            "score_max": float(np.max(roi_score)) if roi_score.size else 0.0,
            "ratio_mean": float(np.mean(roi_ratio)) if roi_ratio.size else 0.0,
            "mean_dir_mean": float(np.mean(roi_mean)) if roi_mean.size else 0.0,
            "is_light_source": is_light_source,
            "light_distance": float(light_distance),
            "stage7_status": "Light Source" if is_light_source else "Candidate",
            "stage8_status": "Skipped",
        }
        roi_analysis_results.append(metrics)
        if not is_light_source:
            selected_roi_indices.append(idx)
    merged_rois_marked = cv2.cvtColor(image_8bit, cv2.COLOR_GRAY2BGR)

    all_roi_denoised_filtered = []
    fallback_roi_denoised_filtered = []
    refined_roi_indices = []
    for idx in selected_roi_indices:
        roi = rois[idx]
        roi_left = roi["left"]
        roi_top = roi["top"]
        roi_right = roi["right"]
        roi_bottom = roi["bottom"]
        roi_full = image_8bit[roi_top:roi_bottom, roi_left:roi_right].copy()
        roi_mask = fixed_mask[roi_top:roi_bottom, roi_left:roi_right]
        roi_full[~roi_mask] = 0

        bg_left = max(0, roi_left - 50)
        bg_top = max(0, roi_top - 50)
        bg_right = min(image_8bit.shape[1], roi_right + 50)
        bg_bottom = min(image_8bit.shape[0], roi_bottom + 50)
        bg_region = image_8bit[bg_top:bg_bottom, bg_left:bg_right]
        bg_mask_region = np.ones(bg_region.shape, dtype=np.bool_)
        local_roi_left = roi_left - bg_left
        local_roi_top = roi_top - bg_top
        local_roi_right = local_roi_left + (roi_right - roi_left)
        local_roi_bottom = local_roi_top + (roi_bottom - roi_top)
        bg_mask_region[local_roi_top:local_roi_bottom, local_roi_left:local_roi_right] = False
        bg_mask_region &= fixed_mask[bg_top:bg_bottom, bg_left:bg_right]

        if np.sum(bg_mask_region) > 0:
            bg_mean = np.mean(bg_region[bg_mask_region])
            bg_std = np.std(bg_region[bg_mask_region])
        else:
            bg_mean = np.min(roi_full)
            bg_std = 10

        bottom_threshold = bg_mean + 2 * bg_std
        background = cv2.GaussianBlur(roi_full, (151, 151), 0)
        roi_denoised = cv2.subtract(roi_full, background)
        roi_denoised = cv2.normalize(roi_denoised, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)

        roi_denoised_filtered = np.where(roi_denoised > bottom_threshold, roi_denoised, 0)
        roi_denoised_filtered = np.where(roi_mask, roi_denoised_filtered, 0).astype(np.uint8)

        structure_metrics = calculate_binary_component_metrics(roi_denoised_filtered > 0)
        keep_roi = (
            structure_metrics["nonzero_pixels"] >= ROI_MIN_NONZERO_PIXELS
            and structure_metrics["largest_component_fraction"] >= ROI_MIN_LARGEST_COMPONENT_FRACTION)
        roi_analysis_results[idx]["stage8_status"] = "Keep" if keep_roi else "Reject"

        roi_output = {
            "left": roi_left,
            "top": roi_top,
            "right": roi_right,
            "bottom": roi_bottom,
            "data": roi_denoised_filtered,
        }
        fallback_roi_denoised_filtered.append(roi_output)
        if keep_roi:
            refined_roi_indices.append(idx)
            all_roi_denoised_filtered.append(roi_output)

    if selected_roi_indices and not refined_roi_indices:
        # print("  结构过滤将所有候选都筛空，回退到结构过滤前的候选结果。")
        refined_roi_indices = list(selected_roi_indices)
        all_roi_denoised_filtered = fallback_roi_denoised_filtered
        for idx in refined_roi_indices:
            roi_analysis_results[idx]["stage8_status"] = "Fallback Keep"

    full_denoised = np.zeros_like(image_8bit)
    for roi_info in all_roi_denoised_filtered:
        roi_left = roi_info["left"]
        roi_top = roi_info["top"]
        roi_data = roi_info["data"]
        roi_h, roi_w = roi_data.shape
        full_denoised[roi_top : roi_top + roi_h, roi_left : roi_left + roi_w] = roi_data

    [ys,xs]=np.where(full_denoised > 0)
    rainbowFlag=False
    if len(xs)>0:
        rainbowFlag=True
    rainbow_mask = full_denoised > 0
    for idx in refined_roi_indices:
        roi = rois[idx]
    return rainbowFlag,xs.astype(np.int32),ys.astype(np.int32)
def rainbowDetection(img_path, theta_deg, phi_deg,waveLen, output_dir, mask_path,isSave=True):
    print("=" * 80)
    print("彩虹纹检测 - V4 手工 Mask ScoreMap 独立版")
    print(f"图像: {img_path}")
    print(f"theta={theta_deg}°, phi={phi_deg}°")
    print("score = mean(direction) * (min(direction) / max(direction))")
    print(f"score percentile = {SCORE_PERCENTILE}")
    print(f"固定Mask: {mask_path}")
    print(f"morph={MORPH_KERNEL_SIZE}x{MORPH_KERNEL_SIZE}, min_area={MIN_COMPONENT_AREA}")
    print(
        f"roi_nonzero>={ROI_MIN_NONZERO_PIXELS}, "
        f"largest_frac>={ROI_MIN_LARGEST_COMPONENT_FRACTION}")
    print(f"输出目录: {output_dir}")
    print("=" * 80)
    os.makedirs(output_dir, exist_ok=True)
    logpath=os.path.join(output_dir, "rianBowDtection.log")
    file_handler = logging.FileHandler(logpath)
    file_handler.setFormatter(fmt)
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(fmt)
    logger.addHandler(file_handler)
    logger.addHandler(console_handler)
    logger.info("rainbow detection start")
    logger.info(output_dir)
    total_start = time.time()
    step_times = {}

    print("\n步骤1: 读取图像...")
    logger.info("步骤1: 读取图像...")
    step_start = time.time()
    raw_image = np.array(Image.open(img_path))
    fixed_mask = load_fixed_mask(mask_path, raw_image.shape)
    masked_image = np.where(fixed_mask, raw_image, 0)
    image_8bit = normalize_to_u8(masked_image)
    if DEBUG == True:
        cv2.imwrite(os.path.join(output_dir, "00_fixed_mask.png"), fixed_mask.astype(np.uint8) * 255)
        cv2.imwrite(os.path.join(output_dir, "01_original_image.png"), image_8bit)
        masked_preview = cv2.cvtColor(image_8bit, cv2.COLOR_GRAY2BGR)
        masked_preview[~fixed_mask] = (0, 0, 0)
        cv2.imwrite(os.path.join(output_dir, "00_masked_input_preview.png"), masked_preview)

    print(f"  图像尺寸: {raw_image.shape}")
    step_times["读取图像"] = time.time() - step_start

    print("\n步骤2: 计算光源理论位置...")
    logger.info("步骤2: 计算光源理论位置...")

    step_start = time.time()
    light_x, light_y = calculate_light_source_position(theta_deg, phi_deg, raw_image.shape)
    if DEBUG == True:
        image_with_light_mark = cv2.cvtColor(image_8bit, cv2.COLOR_GRAY2BGR)
        cv2.circle(image_with_light_mark, (light_x, light_y), 20, (0, 0, 255), 3)
        cv2.putText( image_with_light_mark,  "Light Source",  (light_x + 30, light_y),  cv2.FONT_HERSHEY_SIMPLEX,0.8,  (0, 0, 255),2, )
        cv2.imwrite(os.path.join(output_dir, "01_original_with_light_mark.png"), image_with_light_mark)
    print(f"  光源理论位置: (x={light_x}, y={light_y})")
    logger.info(f"  光源理论位置: (x={light_x}, y={light_y})")
    step_times["计算光源位置"] = time.time() - step_start

    print("\n步骤3: 生成方向图和评分图...")
    logger.info("步骤3: 生成方向图和评分图...")

    step_start = time.time()
    directional_images = generate_directional_images(raw_image)
    directional_images = [np.where(fixed_mask, item, 0).astype(np.uint8) for item in directional_images]
    if DEBUG == True:
        fig, axes = plt.subplots(1, 4, figsize=(16, 4))
        directions = [0, 45, 90, 135]
        for idx, direction_deg in enumerate(directions):
            img_back = directional_images[idx]
            cv2.imwrite(os.path.join(output_dir, f"02_dir_{direction_deg}deg.png"), img_back)
            axes[idx].imshow(img_back, cmap="gray")
            axes[idx].set_title(f"{direction_deg}°")
            axes[idx].axis("off")
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, "02_all_directions.png"), dpi=100, bbox_inches="tight")
        plt.close(fig)

    score_map, ratio_map, mean_map = build_score_map(directional_images)
    if DEBUG == True:
        cv2.imwrite(os.path.join(output_dir, "02_score_map.png"), normalize_to_u8(np.where(fixed_mask, score_map, 0)))
        cv2.imwrite(os.path.join(output_dir, "02_ratio_map.png"), normalize_to_u8(np.where(fixed_mask, ratio_map, 0)))
        cv2.imwrite(os.path.join(output_dir, "02_mean_map.png"), normalize_to_u8(np.where(fixed_mask, mean_map, 0)))

    step_times["生成方向图和评分图"] = time.time() - step_start

    print("\n步骤4: 基于评分图分位数阈值生成候选区域...")
    logger.info("步骤4: 基于评分图分位数阈值生成候选区域...")

    step_start = time.time()
    score_threshold, raw_candidate, refined_candidate = build_score_candidate_mask(score_map, fixed_mask)
    raw_candidate_u8 = raw_candidate.astype(np.uint8) * 255
    refined_candidate_u8 = refined_candidate.astype(np.uint8) * 255
    if DEBUG == True:
        cv2.imwrite(os.path.join(output_dir, "03_candidate_raw.png"), raw_candidate_u8)
        cv2.imwrite(os.path.join(output_dir, "03_candidate_selected.png"), refined_candidate_u8)
    num_labels, labels, stats, _ = cv2.connectedComponentsWithStats(refined_candidate_u8, 8)
    print(f"  score_threshold={score_threshold:.2f}")
    print(f"  检测到 {num_labels - 1} 个初始区域")
    logger.info(f"  score_threshold={score_threshold:.2f}")
    logger.info(f"  检测到 {num_labels - 1} 个初始区域")

    step_times["评分图候选区域"] = time.time() - step_start

    print("\n步骤5: 过滤过小连通域...")
    logger.info("步骤5: 过滤过小连通域...")

    step_start = time.time()
    all_regions = []
    for label_idx in range(1, num_labels):
        area = stats[label_idx, cv2.CC_STAT_AREA]
        if area < MIN_COMPONENT_AREA:
            continue
        left = stats[label_idx, cv2.CC_STAT_LEFT]
        top = stats[label_idx, cv2.CC_STAT_TOP]
        width = stats[label_idx, cv2.CC_STAT_WIDTH]
        height = stats[label_idx, cv2.CC_STAT_HEIGHT]
        region = {
            "label": len(all_regions) + 1,
            "left": left,
            "top": top,
            "width": width,
            "height": height,
            "area": area,
        }
        all_regions.append(region)
        print(f"  区域{region['label']}: ({left}, {top})-({left + width}, {top + height}), 面积={area}")
        logger.info(f"  区域{region['label']}: ({left}, {top})-({left + width}, {top + height}), 面积={area}")

    all_regions_marked = cv2.cvtColor(image_8bit, cv2.COLOR_GRAY2BGR)
    if DEBUG == True:
        for region in all_regions:
            left = region["left"]
            top = region["top"]
            width = region["width"]
            height = region["height"]
            cv2.rectangle(all_regions_marked, (left, top), (left + width, top + height), (0, 255, 0), 3)
            cv2.putText(all_regions_marked,f"{region['label']}",(left, top - 5),cv2.FONT_HERSHEY_SIMPLEX,0.6,(0, 255, 0),2,)
        cv2.imwrite(os.path.join(output_dir, "04_all_regions_marked.png"), all_regions_marked)

    step_times["过滤连通域"] = time.time() - step_start

    print("\n步骤6: 扩展并合并 ROI...")
    logger.info("步骤6: 扩展并合并 ROI...")

    step_start = time.time()
    rois = []
    for region in all_regions:
        left = region["left"]
        top = region["top"]
        width = region["width"]
        height = region["height"]
        rois.append(
            {
                "left": max(0, left - ROI_PADDING),
                "top": max(0, top - ROI_PADDING),
                "right": min(image_8bit.shape[1], left + width + ROI_PADDING),
                "bottom": min(image_8bit.shape[0], top + height + ROI_PADDING),
                "regions": [region["label"]],
            }
        )
    rois = merge_overlapping_rois(rois)
    print(f"  合并后得到 {len(rois)} 个ROI")
    logger.info(f"  合并后得到 {len(rois)} 个ROI")

    for idx, roi in enumerate(rois, start=1):
        print(
            f"  ROI{idx}: ({roi['left']}, {roi['top']})-({roi['right']}, {roi['bottom']}), "
            f"包含区域: {roi['regions']}"
        )
        logger.info(
            f"  ROI{idx}: ({roi['left']}, {roi['top']})-({roi['right']}, {roi['bottom']}), "
            f"包含区域: {roi['regions']}"
        )
    step_times["合并ROI"] = time.time() - step_start

    print("\n步骤7: 用评分图候选 + 光源位置筛选 ROI...")
    logger.info("步骤7: 用评分图候选 + 光源位置筛选 ROI...")

    step_start = time.time()
    selected_roi_indices = []
    roi_analysis_results = []
    for idx, roi in enumerate(rois):
        roi_score = score_map[roi["top"] : roi["bottom"], roi["left"] : roi["right"]]
        roi_ratio = ratio_map[roi["top"] : roi["bottom"], roi["left"] : roi["right"]]
        roi_mean = mean_map[roi["top"] : roi["bottom"], roi["left"] : roi["right"]]
        roi_center_x = (roi["left"] + roi["right"]) // 2
        roi_center_y = (roi["top"] + roi["bottom"]) // 2
        light_distance = math.sqrt((roi_center_x - light_x) ** 2 + (roi_center_y - light_y) ** 2)
        is_light_source = light_distance < LIGHT_SOURCE_THRESHOLD
        metrics = {
            "score_mean": float(np.mean(roi_score)) if roi_score.size else 0.0,
            "score_max": float(np.max(roi_score)) if roi_score.size else 0.0,
            "ratio_mean": float(np.mean(roi_ratio)) if roi_ratio.size else 0.0,
            "mean_dir_mean": float(np.mean(roi_mean)) if roi_mean.size else 0.0,
            "is_light_source": is_light_source,
            "light_distance": float(light_distance),
            "stage7_status": "Light Source" if is_light_source else "Candidate",
            "stage8_status": "Skipped",
        }
        roi_analysis_results.append(metrics)
        if not is_light_source:
            selected_roi_indices.append(idx)
        print(
            f"  ROI{idx + 1}: score_mean={metrics['score_mean']:.2f}, "
            f"score_max={metrics['score_max']:.2f}, "
            f"ratio_mean={metrics['ratio_mean']:.2f}, "
            f"light_dist={metrics['light_distance']:.1f} -> {metrics['stage7_status']}"
        )
        logger.info(
            f"  ROI{idx + 1}: score_mean={metrics['score_mean']:.2f}, "
            f"score_max={metrics['score_max']:.2f}, "
            f"ratio_mean={metrics['ratio_mean']:.2f}, "
            f"light_dist={metrics['light_distance']:.1f} -> {metrics['stage7_status']}"
        )
    merged_rois_marked = cv2.cvtColor(image_8bit, cv2.COLOR_GRAY2BGR)
    if DEBUG == True:
        cv2.circle(merged_rois_marked, (light_x, light_y), 20, (0, 0, 255), 3)
        for idx, roi in enumerate(rois):
            color = (0, 255, 0) if idx in selected_roi_indices else (0, 0, 255)
            cv2.rectangle(merged_rois_marked, (roi["left"], roi["top"]), (roi["right"], roi["bottom"]), color, 3)
            cv2.putText(merged_rois_marked,f"{idx + 1}",(roi["left"], roi["top"] - 5),cv2.FONT_HERSHEY_SIMPLEX,0.6,  color,2, )
        cv2.imwrite(os.path.join(output_dir, "05_merged_rois_marked.png"), merged_rois_marked)
    step_times["筛选ROI"] = time.time() - step_start

    print("\n步骤8: 对筛选后的 ROI 执行去背景和结构过滤...")
    logger.info("步骤8: 对筛选后的 ROI 执行去背景和结构过滤...")

    step_start = time.time()
    all_roi_denoised_filtered = []
    fallback_roi_denoised_filtered = []
    refined_roi_indices = []
    for idx in selected_roi_indices:
        roi = rois[idx]
        roi_left = roi["left"]
        roi_top = roi["top"]
        roi_right = roi["right"]
        roi_bottom = roi["bottom"]
        roi_full = image_8bit[roi_top:roi_bottom, roi_left:roi_right].copy()
        roi_mask = fixed_mask[roi_top:roi_bottom, roi_left:roi_right]
        roi_full[~roi_mask] = 0

        bg_left = max(0, roi_left - 50)
        bg_top = max(0, roi_top - 50)
        bg_right = min(image_8bit.shape[1], roi_right + 50)
        bg_bottom = min(image_8bit.shape[0], roi_bottom + 50)
        bg_region = image_8bit[bg_top:bg_bottom, bg_left:bg_right]
        bg_mask_region = np.ones(bg_region.shape, dtype=np.bool_)
        local_roi_left = roi_left - bg_left
        local_roi_top = roi_top - bg_top
        local_roi_right = local_roi_left + (roi_right - roi_left)
        local_roi_bottom = local_roi_top + (roi_bottom - roi_top)
        bg_mask_region[local_roi_top:local_roi_bottom, local_roi_left:local_roi_right] = False
        bg_mask_region &= fixed_mask[bg_top:bg_bottom, bg_left:bg_right]

        if np.sum(bg_mask_region) > 0:
            bg_mean = np.mean(bg_region[bg_mask_region])
            bg_std = np.std(bg_region[bg_mask_region])
        else:
            bg_mean = np.min(roi_full)
            bg_std = 10

        bottom_threshold = bg_mean + 2 * bg_std
        background = cv2.GaussianBlur(roi_full, (151, 151), 0)
        roi_denoised = cv2.subtract(roi_full, background)
        roi_denoised = cv2.normalize(roi_denoised, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)

        roi_denoised_filtered = np.where(roi_denoised > bottom_threshold, roi_denoised, 0)
        roi_denoised_filtered = np.where(roi_mask, roi_denoised_filtered, 0).astype(np.uint8)

        structure_metrics = calculate_binary_component_metrics(roi_denoised_filtered > 0)
        keep_roi = (
            structure_metrics["nonzero_pixels"] >= ROI_MIN_NONZERO_PIXELS
            and structure_metrics["largest_component_fraction"] >= ROI_MIN_LARGEST_COMPONENT_FRACTION)
        roi_analysis_results[idx]["stage8_status"] = "Keep" if keep_roi else "Reject"
        print(
            f"  ROI{idx + 1} 结构过滤: nz={structure_metrics['nonzero_pixels']}, "
            f"components={structure_metrics['component_count']}, "
            f"largest_frac={structure_metrics['largest_component_fraction']:.2f} -> "
            f"{'Keep' if keep_roi else 'Reject'}"
        )

        roi_output = {
            "left": roi_left,
            "top": roi_top,
            "right": roi_right,
            "bottom": roi_bottom,
            "data": roi_denoised_filtered,
        }
        fallback_roi_denoised_filtered.append(roi_output)
        if keep_roi:
            refined_roi_indices.append(idx)
            all_roi_denoised_filtered.append(roi_output)

    if selected_roi_indices and not refined_roi_indices:
        print("  结构过滤将所有候选都筛空，回退到结构过滤前的候选结果。")
        logger.info("  结构过滤将所有候选都筛空，回退到结构过滤前的候选结果。")

        refined_roi_indices = list(selected_roi_indices)
        all_roi_denoised_filtered = fallback_roi_denoised_filtered
        for idx in refined_roi_indices:
            roi_analysis_results[idx]["stage8_status"] = "Fallback Keep"
    step_times["步骤8过滤"] = time.time() - step_start

    print("\n步骤8.5: 生成 ROI 分析图...")
    logger.info("步骤8.5: 生成 ROI 分析图...")

    step_start = time.time()
    fig = plt.figure(figsize=(24, 10))
    gs = fig.add_gridspec(1, 2, width_ratios=[3.2, 1.4])
    ax_img = fig.add_subplot(gs[0])
    ax_img.imshow(merged_rois_marked)
    ax_img.set_title("ROI标记 (绿色=通过评分图筛选, 红色=光源附近)")
    ax_img.axis("off")

    right_gs = gs[1].subgridspec(2, 1, height_ratios=[0.42, 0.58], hspace=0.15)
    ax_info = fig.add_subplot(right_gs[0])
    ax_table = fig.add_subplot(right_gs[1])
    ax_info.axis("off")
    ax_table.axis("off")

    table_data = []
    for idx, metrics in enumerate(roi_analysis_results):
        table_data.append(
            [
                f"ROI {idx + 1}",
                f"{metrics['score_mean']:.2f}",
                f"{metrics['score_max']:.2f}",
                f"{metrics['ratio_mean']:.2f}",
                metrics["stage7_status"],
                metrics["stage8_status"],
            ]
        )

    if table_data:
        table = ax_table.table(
            cellText=table_data,
            colLabels=["ROI", "ScoreMean", "ScoreMax", "RatioMean", "步骤7", "步骤8"],
            loc="center",
            cellLoc="center",
            colWidths=[0.16, 0.18, 0.18, 0.16, 0.16, 0.16],
        )
        table.auto_set_font_size(False)
        table.set_fontsize(9)
        table.scale(1.08, 1.8)
    else:
        ax_table.text(
            0.5,
            0.48,
            "未检测到任何 ROI",
            ha="center",
            va="center",
            transform=ax_table.transAxes,
            fontsize=13,
            color="red",
        )

    ax_info.text(0.5, 0.86, "步骤7判定", ha="center", va="center", transform=ax_info.transAxes, fontsize=13, fontweight="bold")
    ax_info.text(0.5, 0.68, "评分图候选 ROI 全部保留", ha="center", va="center", transform=ax_info.transAxes, fontsize=11, color="green")
    ax_info.text(0.5, 0.54, f"仅排除光源附近 < {LIGHT_SOURCE_THRESHOLD}px", ha="center", va="center", transform=ax_info.transAxes, fontsize=10, color="red")
    ax_info.text(0.5, 0.36, "评分图", ha="center", va="center", transform=ax_info.transAxes, fontsize=13, fontweight="bold")
    ax_info.text(0.5, 0.24, "score = mean * (min / max)", ha="center", va="center", transform=ax_info.transAxes, fontsize=10, color="blue")
    ax_info.text(0.5, 0.12, f"threshold = mask内 P{SCORE_PERCENTILE}", ha="center", va="center", transform=ax_info.transAxes, fontsize=10, color="blue")
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, "06_roi_analysis_with_table.png"), dpi=100, bbox_inches="tight")
    plt.close(fig)
    step_times["生成分析图"] = time.time() - step_start

    print("\n步骤9: 生成完整检测图...")
    logger.info("n步骤9: 生成完整检测图...")

    step_start = time.time()
    full_denoised = np.zeros_like(image_8bit)
    for roi_info in all_roi_denoised_filtered:
        roi_left = roi_info["left"]
        roi_top = roi_info["top"]
        roi_data = roi_info["data"]
        roi_h, roi_w = roi_data.shape
        full_denoised[roi_top : roi_top + roi_h, roi_left : roi_left + roi_w] = roi_data
    # cv2.imwrite(os.path.join(output_dir, "07_full_denoised.png"), full_denoised)
    step_times["生成完整检测图"] = time.time() - step_start

    [ys,xs]=np.where(full_denoised > 0)
    rainbowFlag=False
    if len(xs)>0:
        rainbowFlag=True

    saveRainbowInfo(img_path,output_dir,full_denoised,raw_image)
    step_times["输出rainbow到csv"] = time.time() - step_start
    print("\n步骤10: 输出叠加结果...")
    logger.info("步骤10: 输出叠加结果...")

    step_start = time.time()
    rainbow_mask = full_denoised > 0
    fig, ax = plt.subplots(figsize=(12, 10))
    ax.imshow(image_8bit, cmap="gray")
    overlay = ax.imshow(full_denoised, cmap="jet", alpha=0.8)
    overlay.set_data(np.where(rainbow_mask, full_denoised, np.nan))

    for idx in refined_roi_indices:
        roi = rois[idx]
        print(roi)
        rect = plt.Rectangle(
            (roi["left"], roi["top"]),
            roi["right"] - roi["left"],
            roi["bottom"] - roi["top"],
            linewidth=3,
            edgecolor="g",
            facecolor="none",
        )
        ax.add_patch(rect)

    ax.axis("off")
    ax.set_title(
        "Rainbow Detection (ScoreMap Manual Mask Standalone)\n"
        f"theta={theta_deg}°, phi={phi_deg}°"
    )
    # plt.colorbar(overlay, ax=ax, label="Brightness Level")
    plt.savefig(os.path.join(output_dir, "08_final_result_overlay.png"), dpi=100, bbox_inches="tight")
    plt.close(fig)
    step_times["输出叠加结果"] = time.time() - step_start

    total_elapsed = time.time() - total_start
    print("\n各步骤耗时:")
    for key, value in step_times.items():
        print(f"  {key}: {value:.2f}秒")
    print(f"\n总耗时: {total_elapsed:.2f}秒")
    logger.info("rainbow 检测结束")
    return rainbowFlag,xs.astype(np.int32),ys.astype(np.int32)

def getImgInfoByImgPath(img_path):
    folder_name = Path(img_path).parts[-4]
    match = re.search(r'_(\d+)_(\d+)_(\d+)_(\d+)$', folder_name)
    parts = folder_name.split('_')
    if len(parts)==6 or len(parts)==5:
        waveLen=int(parts[2])
        phi_deg=int(parts[3])
        theta_deg=int(parts[4])
    p = Path(img_path)
    img_filename=p.stem
    output_dir=p.parent.parent
    output_dir = os.path.join(
        output_dir,
        f"result_final_{img_filename}_timed_v4_manual_mask_scoremap_standalone",
    )
    return theta_deg, phi_deg,waveLen, output_dir
def saveRainbowInfo(imgpath,outputdir,full_denoised,raw_image):
    p = Path(imgpath)
    foldername=p.parts[-4]
    csvname=foldername+'.csv'
    csv_path= os.path.join(outputdir, csvname)
    npyname=foldername+'.npy'
    npy_path=os.path.join(outputdir,npyname)
    h, w = full_denoised.shape[:2]
    bin=np.round(np.sqrt(IMGSIZE[0]*IMGSIZE[1]/(h*w)))
    cen=CENTER/bin
    pixel=PIXELSIZE*bin
    y, x = np.where(full_denoised > 0)
    imgfilter=np.zeros_like(raw_image)
    imgfilter[x,y]=raw_image[x,y]
    ys, xs = np.indices((h, w))
    xs = xs.ravel()
    ys = ys.ravel()
    values = imgfilter.ravel()
    count = np.count_nonzero(values > 0)
    fovx=np.arctan((xs-cen[0])*pixel/FOCALLENGTH)*180.0/np.pi
    fovy=np.arctan((ys-cen[1])*pixel/FOCALLENGTH)*180.0/np.pi
    df = pd.DataFrame({
        'pixel_x': xs,
        'pixel_y': ys,
        'FOV_X':fovx,
        'FOV_Y':fovy,
        'gray': values
    })
    data = {
        "columns": df.columns.tolist(),
        "data": df.to_numpy()
    }
    if IsSaveNpyFile==True:
        np.save(npy_path, data)

def collectRainbowData(path):
    root=Path(path)
    save_dir = root / "resultSummary"
    save_dir.mkdir(exist_ok=True)
    groups = defaultdict(list)
    for file in root.rglob("*.npy"):
        name = file.stem
        parts = name.split('_')

        match = re.search(
            r'_(\d+)_(\d+)_(\d+)_(\d+)$',
            name)
        # if match:
        #     # print(match.group())
        #     wavelength = match.group(1)
        #     key1 = match.group(2)
        #     key2 = match.group(3)
        #     key = f"{key1}_{key2}"
        #     groups[key].append(
        #         (wavelength, file))
        if len(parts)==6 or len (parts)==5:
            # print(match.group())
            wavelength = parts[2]
            key1 = parts[3]
            key2 =parts[4]
            key = f"{key1}_{key2}"
            groups[key].append(
                (wavelength, file))

    for key, files in groups.items():
        print(f"\n处理组: {key}")
        merged_df = None
        for wavelength, file in files:
            print(file)
            df = np.load(file,allow_pickle=True).item()
            data=df["data"]
            col=df["columns"]
            if data.shape[1] < 5:
                print("列数不足")
                continue
            base_cols = pd.DataFrame(data[:, :4], columns=["pixel_x", "pixel_y", "FOV_X", "FOV_Y"])
            value_col = pd.DataFrame(data[:, -1], columns=["W_" + str(wavelength)])
            current_df = pd.concat([base_cols, value_col], axis=1)
            current_df = pd.concat(
                [base_cols, value_col],
                axis=1
            )
            if merged_df is None:
                merged_df = current_df
            else:
                merged_df = pd.merge(
                    merged_df,
                    current_df,
                    on=list(base_cols.columns),
                    how="outer"
                )
        if merged_df is not None and IsSaveNpyFile==True:
            save_path = save_dir / f"{key}.npy"
            data = {
                "columns": merged_df.columns.tolist(),
                "data": merged_df.to_numpy()
            }
            np.save(save_path, data)
    return "sucessfully"


def process_single_image(img_path):
    theta_deg, phi_deg,waveLen, output_dir=getImgInfoByImgPath(img_path)
    rainbowFlag,xs,ys=rainbowDetection(img_path, theta_deg, phi_deg,waveLen,output_dir, MASK_PATH)
    print("\n处理完成。")
    print(f"结果目录: {output_dir}")
    return rainbowFlag,xs,ys

def process_batch_image(root_path):
    logpath=os.path.join(root_path, "process_batch_image.log")
    file_handler = logging.FileHandler(logpath)
    file_handler.setFormatter(fmt)
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(fmt)
    logger.addHandler(file_handler)
    logger.addHandler(console_handler)
    logger.info("process_batch_image detection start")
    folder_names = [f.name for f in Path(root_path).iterdir() if f.is_dir()]

    data=[]
    for folder in folder_names:
        # count = folder.count('_')
        parts = folder.split('_')
        count=len(parts)
        if count!=5 and count!=6:
            logger.error("image foldre name is not right: "+folder)
            continue
        sub=Path(folder+'\IQ\Raw\GrayImage_Raw.tif')
        fullpath = os.path.join(root_path, sub)
        path = Path(fullpath)
        if path.exists():
            rainbowFlag, xs, ys = process_single_image(fullpath)
            if rainbowFlag ==True:
                theta_deg, phi_deg,waveLen, output_dir=getImgInfoByImgPath(fullpath)
                data.append([theta_deg,phi_deg,waveLen,np.mean(xs),np.mean(ys),len(xs)])
                logger.info(folder+" has rainbow, and rianbow detection successfully")
            else:
                logger.info(folder+" rainbow ddetection error")
        else:
            logger.error(fullpath +" is not exist")

    df = pd.DataFrame(data, columns=["theta", "phi","waveLength","centerx","centery","rainbowPtsNum"])
    dataOutname=os.path.join(root_path, "rianbowInfo.csv")
    df.to_csv(dataOutname, index=False)
    collectRainbowData(root_path)
    logger.info("process_batch_image detection end")
    return True
if __name__ == "__main__":
    path = r"I:\IMGS\rainbow\20260327_MetricsTest\20260327_MetricsTest\SL1_20260327T210301_410_195_9_1\IQ\Raw\GrayImage_Raw.tif"
    path=r'I:\IMGS\rainbow\Rainbow\test\SL1_20260415T111317_410_135_82_500\IQ\Raw\GrayImage_Raw.tif'
    # flag,xs,ys=process_single_image(path)
    #img=cv2.imread(path,-1)
    # flag,xs1,ys1=rainbowDetection1(img,86,134,410,MASK_PATH)
    root_path=r'E:\SAT_GRR\20260522_MetricsTest\test'
    start=time.time()
    flag=process_batch_image(root_path)
    end=time.time()
    #collectRainbowData(root_path)
    print("运行时间:", end - start, "秒")


