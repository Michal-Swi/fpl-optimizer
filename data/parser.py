import json
import pandas as pd

with open('data', 'r') as f:
    raw_data = json.load(f)

players_list = raw_data.get('elements', raw_data) if isinstance(raw_data, dict) else raw_data

df = pd.DataFrame(players_list)

columns_to_keep = [
    'id', 
    'first_name',
    'second_name',
    'team',           
    'element_type',  
    'now_cost',       
    'expected_goals_per_90', 
    'expected_assists_per_90',
    'expected_goals_conceded_per_90',
    'starts_per_90',
    'ep_next',        
    'minutes',
    'total_points'
]

df_clean = df[columns_to_keep]

cols_to_float = ['expected_goals_per_90', 'expected_assists_per_90', 'expected_goals_conceded_per_90', 'ep_next']
for col in cols_to_float:
    df_clean[col] = pd.to_numeric(df_clean[col], errors='coerce')

columns_to_export = [
    'id', 'first_name', 'second_name', 'team', 'element_type', 'now_cost',
    'expected_goals_per_90', 'expected_assists_per_90',
    'expected_goals_conceded_per_90', 'starts_per_90',
    'ep_next', 'total_points', 'minutes'
]

df_clean[columns_to_export].to_csv('parsed', sep=';', index=False)

with open('parsed', 'w', encoding='utf-8') as f:
    for row in df_clean[columns_to_export].itertuples(index=False):
        line = ";".join(str(val) for val in row)
        f.write(line + "\n")

