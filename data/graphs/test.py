import pandas as pd
from scipy.stats import spearmanr

PLAYERS_RAW_PATH = "../2025" 

POS_MAP = {1: "GK", 2: "DEF", 3: "MID", 4: "FWD"}
RANK_METHODS = ["first", "dense", "average"]


def load_and_prepare(path: str) -> pd.DataFrame:
    df = pd.read_csv(path)

    for col in ["now_cost", "cost_change_start", "minutes", "team", "starts"]:
        df[col] = pd.to_numeric(df[col], errors="coerce")

    df["start_cost"] = df["now_cost"] - df["cost_change_start"]
    df["position_name"] = df["element_type"].map(POS_MAP)

    df = df.dropna(subset=["start_cost", "minutes", "team", "position_name"])
    return df


def add_ranks(df: pd.DataFrame) -> pd.DataFrame:
    for method in RANK_METHODS:
        df[f"rank_{method}"] = df.groupby(["team", "position_name"])["start_cost"].rank(
            method=method, ascending=False
        )
    return df


def report_correlations(df: pd.DataFrame) -> None:
    positions = ["GK", "DEF", "MID", "FWD"]

    for scope_label, scope_df in [
        ("ALL registered players", df),
        ("players with >=1 start", df[df["starts"] >= 1]),
    ]:
        print(f"\n=== {scope_label} ===")
        header = f"{'pos':<5}{'n':>5}" + "".join(f"{m + ' rho':>14}" for m in RANK_METHODS)
        print(header)
        for pos in positions:
            sub = scope_df[scope_df["position_name"] == pos]
            row = f"{pos:<5}{len(sub):>5}"
            for method in RANK_METHODS:
                rho, _p = spearmanr(sub[f"rank_{method}"], sub["minutes"])
                row += f"{rho:>14.3f}"
            print(row)


if __name__ == "__main__":
    data = load_and_prepare(PLAYERS_RAW_PATH)
    data = add_ranks(data)
    report_correlations(data)
    data.to_csv("players_with_ranks.csv", index=False)
