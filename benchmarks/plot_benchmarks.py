import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
import sys


def plot_stacked_timing(df, figures_dir, target_sum):
    """
    Plot stacked timing breakdown for rows where p + q == target_sum, s == q, and depth == 0.
    """
    df_filtered = df[(df['p'] + df['q'] == target_sum) & (df['s'] == df['q']) & (df['depth'] == 0)]
    if df_filtered.empty:
        print(f"No matching rows found for p + q = {target_sum}, s == q, depth == 0 (stacked timing plot). Skipping plot.")
        return

    initial = df_filtered['t_initial_sort']
    pairwise = df_filtered['t_comm_pairwise']
    elbow = df_filtered['t_elbow_sort']
    other = df_filtered['t_total'] - (initial + pairwise + elbow)

    unique_p = sorted(df_filtered['p'].unique())
    x_mapping = {p: i for i, p in enumerate(unique_p)}
    x = df_filtered['p'].map(x_mapping)

    plt.figure(figsize=(6, 4))
    plt.bar(x, initial, label='Initial Sort', color='#4CAF50')
    plt.bar(x, pairwise, bottom=initial, label='Pairwise Comm', color='#2196F3')
    plt.bar(x, elbow, bottom=initial + pairwise, label='Elbow Sort', color='#FFC107')
    plt.bar(x, other, bottom=initial + pairwise + elbow, label='Other', color='#9E9E9E')

    ax = plt.gca()
    positions = list(range(len(unique_p)))
    labels = [str(2**p) for p in unique_p]
    ax.set_xticks(positions)
    ax.set_xticklabels(labels)
    ax.set_xlabel('Number of Processes (2^p)')

    plt.ylabel('Execution Time (seconds)')
    plt.title(f'Timing Breakdown (Array Size = $2^{{{target_sum}}}$, s = q)')
    plt.legend()
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.tight_layout()

    os.makedirs(figures_dir, exist_ok=True)
    save_path = os.path.join(figures_dir, 'stacked_timing.png')
    plt.savefig(save_path)
    plt.close()
    print(f"Stacked timing plot saved at {save_path}")


def export_timing_percentages(df, data_dir, target_sum):
    """
    Export timing percentages for rows where p + q == target_sum, s == q, and depth == 0.
    """
    df_filtered = df[(df['p'] + df['q'] == target_sum) & (df['s'] == df['q']) & (df['depth'] == 0)]
    if df_filtered.empty:
        print(f"No matching rows found for p + q = {target_sum}, s == q, depth == 0 (export). Skipping export.")
        return

    initial = df_filtered['t_initial_sort']
    pairwise = df_filtered['t_comm_pairwise']
    elbow = df_filtered['t_elbow_sort']
    other = df_filtered['t_total'] - (initial + pairwise + elbow)

    df_percent = df_filtered.copy()
    df_percent['initial_pct'] = (initial / df_filtered['t_total']) * 100
    df_percent['pairwise_pct'] = (pairwise / df_filtered['t_total']) * 100
    df_percent['elbow_pct'] = (elbow / df_filtered['t_total']) * 100
    df_percent['other_pct'] = (other / df_filtered['t_total']) * 100
    df_percent['total_time'] = df_filtered['t_total']

    export_columns = ['p', 'q', 's', 'depth', 'total_time', 'initial_pct', 'pairwise_pct', 'elbow_pct', 'other_pct']
    export_df = df_percent[export_columns]

    os.makedirs(data_dir, exist_ok=True)
    export_path = os.path.join(data_dir, 'timing_percentages.dat')
    export_df.to_csv(export_path, index=False, float_format='%.2f')
    print(f"Filtered timing percentages exported at {export_path}")


def plot_total_time_vs_elements(df, figures_dir):
    """
    Plot total execution time vs number of elements (2^(p+q)) for different p values.
    Only for rows where q == s and depth == 0.
    """
    df_filtered = df[(df['q'] == df['s']) & (df['depth'] == 0)]
    if df_filtered.empty:
        print("No matching rows found where q == s, depth == 0 (total time vs elements plot). Skipping plot.")
        return

    df_filtered = df_filtered.copy()
    df_filtered['elements'] = 2 ** (df_filtered['p'] + df_filtered['q'])

    plt.figure(figsize=(6, 4))
    unique_p = sorted(df_filtered['p'].unique())

    for p_val in unique_p:
        df_p = df_filtered[df_filtered['p'] == p_val].sort_values('elements')
        plt.plot(df_p['elements'], df_p['t_total'], marker='o', label=f'p={p_val} (proc={2**p_val})')

    plt.xscale('log', base=2)
    plt.xlabel('Number of Elements (2^(p+q))')
    plt.ylabel('Total Execution Time (seconds)')
    plt.title('Total Execution Time vs Number of Elements (filtered q == s, depth == 0)')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(title='p values')
    plt.tight_layout()

    os.makedirs(figures_dir, exist_ok=True)
    save_path = os.path.join(figures_dir, 'total_time_vs_elements.png')
    plt.savefig(save_path)
    plt.close()
    print(f"Total time vs elements plot saved at {save_path}")


def plot_pairwise_comm_vs_procs(df, figures_dir, target_sum):
    """
    Plot pairwise communication time vs number of processes (2^p) for different communication buffer splits (2^(q-s)).
    Only for rows where p + q == target_sum and depth == 0.
    """
    df_filtered = df[(df['p'] + df['q'] == target_sum) & (df['depth'] == 0)]
    if df_filtered.empty:
        print(f"No matching rows found for p + q = {target_sum}, depth == 0 (pairwise comm plot). Skipping plot.")
        return

    df_filtered = df_filtered.copy()
    df_filtered['splits'] = 2 ** (df_filtered['q'] - df_filtered['s'])

    plt.figure(figsize=(6, 4))

    unique_p = sorted(df_filtered['p'].unique())
    x_mapping = {p: i for i, p in enumerate(unique_p)}
    x_labels = [str(2**p) for p in unique_p]

    unique_splits = sorted(df_filtered['splits'].unique())

    for split_val in unique_splits:
        df_split = df_filtered[df_filtered['splits'] == split_val]
        df_split_sorted = df_split.sort_values('p')
        x = df_split_sorted['p'].map(x_mapping)
        plt.plot(x, df_split_sorted['t_comm_pairwise'], marker='o', label=f'splits = {int(split_val)}')

    ax = plt.gca()
    ax.set_xticks(list(range(len(unique_p))))
    ax.set_xticklabels(x_labels)
    ax.set_xlabel('Number of Processes (2^p)')
    ax.set_ylabel('Pairwise Communication Time (seconds)')
    ax.set_title(f'Pairwise Communication Time vs Number of Processes\n(p + q = {target_sum}, depth = 0)')
    plt.legend(title='Comm. Buffer Splits')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()

    os.makedirs(figures_dir, exist_ok=True)
    save_path = os.path.join(figures_dir, 'pairwise_comm_vs_procs_splits_sum.png')
    plt.savefig(save_path)
    plt.close()
    print(f"Pairwise communication time plot saved at {save_path}")


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python plot_bitonic_timing.py <log_file_path> <figures_save_dir> <data_save_dir>")
        sys.exit(1)

    log_file = sys.argv[1]
    figures_dir = sys.argv[2]
    data_dir = sys.argv[3]

    # Add 'depth' column after 's'
    columns = ['p', 'q', 's', 'depth', 't_initial_sort', 't_comm_pairwise', 't_elbow_sort', 't_total']
    df = pd.read_csv(log_file, sep=r'\s+', names=columns, engine='python', skiprows=2)

    export_timing_percentages(df, data_dir, 27)
    plot_stacked_timing(df, figures_dir, 27)
    plot_total_time_vs_elements(df, figures_dir)
    plot_pairwise_comm_vs_procs(df, figures_dir, 27)
