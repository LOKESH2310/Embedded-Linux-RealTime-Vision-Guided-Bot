# -*- coding: utf-8 -*-
# Code to plot histograms of different methods of pin toggling
# Not original work generated with LLM

import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

# Set Seaborn theme
sns.set_theme(style="darkgrid")

# File names and corresponding descriptive labels
files = {
    "service_runs1_.csv": "Camera Service",
    "service_runs2_.csv": "Red Laser Service",
    "service_runs3_.csv": "Motor Decision Service",
    "service_runs4_.csv": "Motor Motion Service"
}

# Load and combine data
all_data = []
for file, label in files.items():
    df = pd.read_csv(file, header=None, names=["run_time"])
    df["run_label"] = label
    all_data.append(df)

combined_df = pd.concat(all_data, ignore_index=True)

# Create subplots (2x2 layout)
fig, axes = plt.subplots(2, 2, figsize=(14, 10))
axes = axes.flatten()

# Plot each histogram in its own subplot
for i, (file, label) in enumerate(files.items()):
    ax = axes[i]
    subset = combined_df[combined_df["run_label"] == label]

    counts, bins, patches = ax.hist(
        subset["run_time"],
        bins=30,
        color=sns.color_palette()[i],
        edgecolor="black",
        alpha=0.7,
        label=label
    )

    # Add count labels on top of each bar
    for count, bin_left in zip(counts, bins[:-1]):
        if count > 0:
            ax.text(bin_left, count + 0.5, f'{int(count)}', ha='left', va='bottom', fontsize=8, rotation=90)

    # Mean and Std deviation lines
    mean = subset["run_time"].mean()
    std = subset["run_time"].std()

    ax.axvline(mean, color=sns.color_palette()[i], linestyle="--", linewidth=1.5, label=f"Mean = {mean:.2f}")
    ax.axvline(mean + std, color=sns.color_palette()[i], linestyle=":", linewidth=1, label=" Std Dev")
    ax.axvline(mean - std, color=sns.color_palette()[i], linestyle=":", linewidth=1)

    ax.set_title(f"Distribution for {label}")
    ax.set_xlabel("Run Time")
    ax.set_ylabel("Frequency")
    ax.legend()

# Adjust layout to prevent overlap
plt.tight_layout()
plt.show()

# Print summary statistics
print("\nSummary Statistics (including variance):")
summary = combined_df.groupby("run_label")["run_time"].agg(['count', 'mean', 'std', 'var', 'min', 'max'])
print(summary)
