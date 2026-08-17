import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

df = pd.read_csv('../2025')

df['now_cost'] = pd.to_numeric(df['now_cost'], errors='coerce')
df['minutes'] = pd.to_numeric(df['minutes'], errors='coerce')
df['team'] = pd.to_numeric(df['team'], errors='coerce')

pos_mapping = {1: 'GK', 2: 'DEF', 3: 'MID', 4: 'FWD'}
df['position_name'] = df['element_type'].map(pos_mapping)

df = df.dropna(subset=['now_cost', 'minutes', 'team', 'position_name'])

df['team_rank'] = df.groupby(['team', 'position_name'])['now_cost'].rank(method='first', ascending=False)

fig, axes = plt.subplots(2, 2, figsize=(14, 10), sharey=True)
axes = axes.flatten()

positions = ['GK', 'DEF', 'MID', 'FWD']
colors = ['#1f77b4', '#2ca02c', '#ff7f0e', '#d62728']

for i, pos in enumerate(positions):
    subset = df[df['position_name'] == pos]
    ax = axes[i]
    
    sns.scatterplot(
        data=subset, x='team_rank', y='minutes', color=colors[i], 
        alpha=0.6, s=50, ax=ax, label='Player'
    )
    
    sns.regplot(
        data=subset, x='team_rank', y='minutes', scatter=False, 
        lowess=True, color='black', line_kws={'linestyle': '--', 'linewidth': 2}, ax=ax
    )
    
    ax.set_title(f'{pos} - Intra-Squad Hierarchy', fontweight='bold')
    ax.set_xlabel('Team Rank (1 = Most Expensive on Team)')
    ax.set_ylabel('Total Minutes')
    
    max_rank = subset['team_rank'].max()
    if pd.notna(max_rank):
        ax.set_xticks(range(1, int(max_rank) + 1))
        
    ax.grid(True, linestyle=':', alpha=0.5)

plt.suptitle('Predicting Minutes by Intra-Squad Price Rank', fontsize=16, fontweight='bold')
plt.tight_layout()
plt.show()

