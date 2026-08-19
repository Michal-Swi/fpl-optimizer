import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

csv_files = {
    "Top 11 (Stars & Scrubs)": ("../11.csv", "blue", "o"),
    "Top 13 (Balanced)": ("../13.csv", "red", "^")
}

plt.figure(figsize=(12, 7))

for label, (filepath, color, marker) in csv_files.items():
    if os.path.exists(filepath):
        df = pd.read_csv(filepath)
        
        plt.scatter(df['cost'], df['ev'], label=label, color=color, alpha=0.7, s=100, edgecolors='black', marker=marker)
        
        for i, txt in enumerate(df['name']):
            name_parts = txt.split()
            short_name = f"{name_parts[0][0]}. {name_parts[-1]}" if len(name_parts) > 1 else txt
            
            plt.annotate(short_name, (df['cost'].iloc[i], df['ev'].iloc[i]),
                         textcoords="offset points", xytext=(5,5), ha='left', fontsize=8,
                         bbox=dict(boxstyle="round,pad=0.2", fc="white", alpha=0.7, lw=0.5))
            
        if len(df) > 1:
            m, b = np.polyfit(df['cost'], df['ev'], 1)
            x_vals = np.linspace(df['cost'].min(), df['cost'].max(), 100)
            plt.plot(x_vals, m * x_vals + b, color=color, linestyle='--', alpha=0.8)

plt.xlabel("Cost")
plt.ylabel("Expected Value (EV)")
plt.title("Starting 11: Cost vs. EV by Optimization Strategy")
plt.grid(True, linestyle=':', alpha=0.6)
plt.legend()
plt.tight_layout()

plt.savefig("optimization_scatter.png", dpi=300)
plt.show()

