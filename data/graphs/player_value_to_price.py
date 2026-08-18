import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from adjustText import adjust_text

df = pd.read_csv('../player_ev_data.csv')

df = df[df['ev'] > 0]

fig, axes = plt.subplots(2, 2, figsize=(20, 14))
axes = axes.flatten()

positions = ['GK', 'DEF', 'MID', 'FWD']
colors = {'GK': '#1f77b4', 'DEF': '#2ca02c', 'MID': '#ff7f0e', 'FWD': '#d62728'}

for i, pos in enumerate(positions):
    ax = axes[i]
    
    pos_df = df[df['element_type'] == pos].copy()
    
    if len(pos_df) < 2:
        ax.set_title(f'{pos} (Insufficient Data)')
        continue
        
    sns.scatterplot(
        data=pos_df, x='now_cost', y='ev', 
        color=colors[pos], alpha=0.7, s=75, edgecolor='w', ax=ax
    )
    
    x = pos_df['now_cost']
    y = pos_df['ev']
    coef = np.polyfit(x, y, 1)
    poly1d_fn = np.poly1d(coef)
    
    x_trend = np.linspace(x.min(), x.max(), 100)
    ax.plot(x_trend, poly1d_fn(x_trend), color='black', linestyle='--', linewidth=1.5)
    
    pos_df['expected_ev'] = poly1d_fn(pos_df['now_cost'])
    pos_df['residual'] = pos_df['ev'] - pos_df['expected_ev']
    
    top_ev = pos_df.nlargest(5, 'ev')
    top_value = pos_df.nlargest(7, 'residual')
    to_label = pd.concat([top_ev, top_value]).drop_duplicates(subset=['first_name', 'second_name'])
    
    texts = []
    
    for _, row in to_label.iterrows():
        label_name = f"{row['first_name'][0]}. {row['second_name']}"
        
        t = ax.text(
            row['now_cost'], row['ev'], label_name,
            fontsize=8.5,
            bbox=dict(boxstyle="round,pad=0.2", fc="white", ec="gray", alpha=0.85)
        )
        texts.append(t)
        
    adjust_text(
        texts, 
        x=pos_df['now_cost'].tolist(),
        y=pos_df['ev'].tolist(),
        ax=ax, 
        force_text=(1.0, 1.5),
        force_points=(0.5, 1.0),
        max_iter=3000,            
        arrowprops=dict(arrowstyle='-', color='gray', lw=1.0, alpha=0.8)
    )
        
    ax.set_title(f'{pos} Expected Value (EV) vs. Price', fontweight='bold', fontsize=16)
    ax.set_xlabel('Price (Raw FPL Cost)')
    ax.set_ylabel('Calculated EV')
    ax.grid(True, linestyle=':', alpha=0.6)

plt.tight_layout()
plt.show()

