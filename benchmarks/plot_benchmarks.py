import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
import sys

def plot_stacked_timing(df, figures_dir, target_sum=30):
    """
    Plot stacked timing breakdown for rows where p + q == target_sum and s == q.
    """
    # Apply filtering inside the function
    df_filtered = df[(df['p'] + df['q'] == target_sum) & (df['s'] == df['q'])]
    if df_filtered.empty:
        print(f"No matching rows found for p + q = {target_sum} and s == q (stacked timing plot). Skipping plot.")
        return

    initial = df_filtered['t_initial_sort']
    pairwise = df_filtered['t_comm_pairwise']
    elbow = df_filtered['t_elbow_sort']
    other = df_filtered['t_total'] - (initial + pairwise + elbow)

    # Map p exponents to evenly spaced x positions
    unique_p = sorted(df_filtered['p'].unique())
    x_mapping = {p: i for i, p in enumerate(unique_p)}
    x = df_filtered['p'].map(x_mapping)

    plt.figure(figsize=(9, 6))
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

def export_timing_percentages(df, data_dir, target_sum=30):
    """
    Export timing percentages for rows where p + q == target_sum and s == q.
    """
    df_filtered = df[(df['p'] + df['q'] == target_sum) & (df['s'] == df['q'])]
    if df_filtered.empty:
        print(f"No matching rows found for p + q = {target_sum} and s == q (export). Skipping export.")
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

    export_columns = ['p', 'q', 's', 'total_time', 'initial_pct', 'pairwise_pct', 'elbow_pct', 'other_pct']
    export_df = df_percent[export_columns]

    os.makedirs(data_dir, exist_ok=True)
    export_path = os.path.join(data_dir, 'timing_percentages.dat')
    export_df.to_csv(export_path, index=False, float_format='%.2f')
    print(f"Filtered timing percentages exported at {export_path}")

def plot_total_time_vs_q(df, figures_dir):
    """
    Plot total execution time vs q for different p values, filtered with s == q.
    """
    df_filtered = df[df['s'] == df['q']]
    if df_filtered.empty:
        print("No matching rows found where s == q (total time vs q plot). Skipping plot.")
        return

    plt.figure(figsize=(9, 6))
    unique_p = sorted(df_filtered['p'].unique())

    # Collect all unique q values across filtered data for consistent ticks
    all_q = sorted(df_filtered['q'].unique())
    positions = list(range(len(all_q)))  # equally spaced positions for q values
    q_to_pos = {q: pos for q, pos in zip(all_q, positions)}

    for p_val in unique_p:
        df_p = df_filtered[df_filtered['p'] == p_val]
        df_p_sorted = df_p.sort_values('q')
        x = df_p_sorted['q'].map(q_to_pos)
        plt.plot(x, df_p_sorted['t_total'], marker='o', label=f'p={p_val}')

    ax = plt.gca()
    ax.set_xticks(positions)
    ax.set_xticklabels([str(2**q) for q in all_q])
    ax.set_xlabel('q (Array size factor 2^q)')

    plt.ylabel('Total Execution Time (seconds)')
    plt.title('Total Execution Time vs q (for different p), filtered s == q')
    plt.legend(title='p values')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()

    os.makedirs(figures_dir, exist_ok=True)
    save_path = os.path.join(figures_dir, 'total_time_vs_q.png')
    plt.savefig(save_path)
    plt.close()
    print(f"Total time vs q plot saved at {save_path}")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python plot_bitonic_timing.py <log_file_path> <figures_save_dir> <data_save_dir>")
        sys.exit(1)

    log_file = sys.argv[1]
    figures_dir = sys.argv[2]
    data_dir = sys.argv[3]

    columns = ['p', 'q', 's', 't_initial_sort', 't_comm_pairwise', 't_elbow_sort', 't_total']
    df = pd.read_csv(log_file, sep=r'\s+', names=columns, engine='python')

    # Now call each function with the full dataframe
    export_timing_percentages(df, data_dir)
    plot_stacked_timing(df, figures_dir)
    plot_total_time_vs_q(df, figures_dir)
