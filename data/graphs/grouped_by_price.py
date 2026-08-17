import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

columns = [
    'first_name', 'second_name', 'goals_scored', 'assists', 'total_points',
    'minutes', 'goals_conceded', 'creativity', 'influence', 'threat', 'bonus',
    'bps', 'ict_index', 'clean_sheets', 'red_cards', 'yellow_cards',
    'selected_by_percent', 'now_cost', 'element_type', 'value_per_m'
]

df = pd.read_csv('../2025', header=None, names=columns)

df['now_cost'] = pd.to_numeric(df['now_cost'], errors='coerce')
df['minutes'] = pd.to_numeric(df['minutes'], errors='coerce')
df = df.dropna(subset=['now_cost', 'minutes', 'element_type'])

df['price_rank'] = df.groupby('element_type')['now_cost'].rank(
    method='first', ascending=False
)

rank_map = (
    df.groupby(['element_type', 'price_rank'])['minutes'].mean().reset_index()
)

fig, axes = plt.subplots(2, 2, figsize=(14, 10), sharey=True)
axes = axes.flatten()

positions = ['GK', 'DEF', 'MID', 'FWD']
colors = ['#1f77b4', '#2ca02c', '#ff7f0e', '#d62728']

for i, pos in enumerate(positions):
    subset = rank_map[rank_map['element_type'] == pos]
    ax = axes[i]

    corr = np.corrcoef(subset['price_rank'], subset['minutes'])[0, 1]

    sns.scatterplot(
        data=subset,
        x='price_rank',
        y='minutes',
        color=colors[i],
        alpha=0.7,
        s=40,
        ax=ax,
        label=f'Rank Avg (r = {corr:.2f})',
    )

    sns.regplot(
        data=subset,
        x='price_rank',
        y='minutes',
        scatter=False,
        lowess=True,
        color='black',
        line_kws={'linestyle': '--', 'linewidth': 1.8},
        ax=ax,
        label='Non-linear Trend (LOWESS)',
    )

    ax.set_title(f'Position: {pos}', fontsize=12, fontweight='bold')
    ax.set_xlabel('Price Rank (1 = Most Expensive)', fontsize=10)
    ax.set_ylabel('Average Minutes Played', fontsize=10)
    ax.grid(True, linestyle=':', alpha=0.6)
    ax.legend(loc='upper right')

plt.suptitle(
    'Price Rank vs. Minutes Played by Position', fontsize=15, fontweight='bold'
)
plt.tight_layout()
plt.show()

