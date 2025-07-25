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
    plt.title(f'Timing Breakdown (Array Size = $2^{{{target_sum}}}$)')
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

    plt.figure(figsize=(9, 6))

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
    ax.set_title(f'Pairwise Communication Time vs Number of Processes\n(Array size = 2^{target_sum})')
    plt.legend(title='Comm. Buffer Splits')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()

    os.makedirs(figures_dir, exist_ok=True)
    save_path = os.path.join(figures_dir, 'pairwise_comm_vs_procs_splits_sum.png')
    plt.savefig(save_path)
    plt.close()
    print(f"Pairwise communication time plot saved at {save_path}")


def plot_total_time_vs_procs_by_depth(df, figures_dir, target_sum):
    """
    Plot total execution time vs number of processes (2^p) for different depth values,
    filtered by p + q == target_sum and s == q.
    """
    df_filtered = df[(df['p'] + df['q'] == target_sum) & (df['s'] == df['q'])]
    if df_filtered.empty:
        print(f"No matching rows found for p + q = {target_sum}, s == q (total time vs procs by depth). Skipping plot.")
        return

    plt.figure(figsize=(9, 6))
    unique_depths = sorted(df_filtered['depth'].unique())
    unique_p = sorted(df_filtered['p'].unique())
    x_mapping = {p: i for i, p in enumerate(unique_p)}
    x_labels = [str(2 ** p) for p in unique_p]

    for depth_val in unique_depths:
        df_depth = df_filtered[df_filtered['depth'] == depth_val]
        df_depth_sorted = df_depth.sort_values('p')
        x = df_depth_sorted['p'].map(x_mapping)
        plt.plot(x, df_depth_sorted['t_total'], marker='o', label=f'depth = {depth_val}')

    ax = plt.gca()
    ax.set_xticks(list(range(len(unique_p))))
    ax.set_xticklabels(x_labels)
    ax.set_xlabel('Number of Processes (2^p)')
    ax.set_ylabel('Total Execution Time (seconds)')
    ax.set_title(f'Total Time vs Number of Processes\n(p + q = {target_sum}, s = q)')
    plt.legend(title='Depth')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()

    os.makedirs(figures_dir, exist_ok=True)
    save_path = os.path.join(figures_dir, 'total_time_vs_procs_by_depth.png')
    plt.savefig(save_path)
    plt.close()
    print(f"Total time vs processes (by depth) plot saved at {save_path}")


def export_speedups_vs_depth(df, data_dir, target_sum):
    """
    Export a table with columns:
    p, q, initial_speedup_depth1, total_speedup_depth1, initial_speedup_depth2, total_speedup_depth2
    for rows where p + q == target_sum and s == q.
    Speedup is relative to depth == 0.
    """
    df_filtered = df[(df['p'] + df['q'] == target_sum) & (df['s'] == df['q'])]
    if df_filtered.empty:
        print(f"No matching rows found for p + q = {target_sum}, s == q (speedup export). Skipping export.")
        return

    base_df = df_filtered[df_filtered['depth'] == 0]
    if base_df.empty:
        print(f"No baseline (depth == 0) rows found. Cannot compute speedups.")
        return

    # Prepare list of rows for output
    rows = []
    unique_pq = base_df[['p', 'q']].drop_duplicates()

    for _, pq_row in unique_pq.iterrows():
        p, q = pq_row['p'], pq_row['q']

        base_row = base_df[(base_df['p'] == p) & (base_df['q'] == q)].iloc[0]

        row_data = {
            'p': int(p),
            'q': int(q),
            'initial_speedup_depth1': np.nan,
            'total_speedup_depth1': np.nan,
            'initial_speedup_depth2': np.nan,
            'total_speedup_depth2': np.nan
        }

        for depth in [1, 2]:
            depth_row = df_filtered[(df_filtered['p'] == p) & (df_filtered['q'] == q) & (df_filtered['depth'] == depth)]
            if not depth_row.empty:
                depth_row = depth_row.iloc[0]
                row_data[f'initial_speedup_depth{depth}'] = base_row['t_initial_sort'] / depth_row['t_initial_sort']
                row_data[f'total_speedup_depth{depth}'] = base_row['t_total'] / depth_row['t_total']

        rows.append(row_data)

    export_df = pd.DataFrame(rows)

    os.makedirs(data_dir, exist_ok=True)
    export_path = os.path.join(data_dir, 'speedups_vs_depth.dat')
    export_df.to_csv(export_path, index=False, float_format='%.2f')
    print(f"Speedups table (depth 1 & 2) exported at {export_path}")


def plot_total_time_vs_procs_by_q(df, figures_dir):
    """
    Plot total execution time vs number of processes (2^p) for different q values,
    filtered by s == q and depth == 0.
    """
    df_filtered = df[(df['s'] == df['q']) & (df['depth'] == 0)]
    if df_filtered.empty:
        print("No matching rows found where s == q and depth == 0 (total time vs procs by q). Skipping plot.")
        return

    plt.figure(figsize=(9, 6))

    unique_q = sorted(df_filtered['q'].unique())
    unique_p = sorted(df_filtered['p'].unique())
    x_mapping = {p: i for i, p in enumerate(unique_p)}
    x_labels = [str(2**p) for p in unique_p]

    for q_val in unique_q:
        df_q = df_filtered[df_filtered['q'] == q_val].sort_values('p')
        x = df_q['p'].map(x_mapping)
        plt.plot(x, df_q['t_total'], marker='o', label=f'q = {q_val}')

    ax = plt.gca()
    ax.set_xticks(list(range(len(unique_p))))
    ax.set_xticklabels(x_labels)
    ax.set_xlabel('Number of Processes (2^p)')
    ax.set_ylabel('Total Execution Time (seconds, log2 scale)')
    ax.set_yscale('log', base=2)
    plt.title('Total Execution Time vs Number of Processes by q\n(s = q, depth = 0)')
    plt.legend(title='q values')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()

    os.makedirs(figures_dir, exist_ok=True)
    save_path = os.path.join(figures_dir, 'total_time_vs_procs_by_q.png')
    plt.savefig(save_path)
    plt.close()
    print(f"Total time vs processes (by q) plot saved at {save_path}")


def export_speedups_by_p(df, data_dir, output_filename='speedups_by_p.dat'):
    """
    Export a table of speedups by increasing p for fixed p+q sums.
    Conditions: s == q and depth == 0.

    Columns:
    - 'sum': p + q
    - Other columns: speedup for each p (relative to base p for the sum), or '-' if missing.
    """
    df_filtered = df[(df['s'] == df['q']) & (df['depth'] == 0)]
    if df_filtered.empty:
        print("No matching rows found where s == q and depth == 0 (speedups by p). Skipping export.")
        return

    # Group by p+q and compute speedups
    df_filtered = df_filtered.copy()
    df_filtered['sum'] = df_filtered['p'] + df_filtered['q']
    grouped = df_filtered.groupby('sum')

    all_p_vals = sorted(df_filtered['p'].unique())
    col_headers = ['sum'] + [f'p={p}' for p in all_p_vals]

    rows = []

    for sum_val, group in grouped:
        group = group.set_index('p')
        p_vals = sorted(group.index.unique())
        base_p = min(p_vals)
        base_time = group.loc[base_p]['t_total']
        row = [sum_val]

        for p in all_p_vals:
            if p in group.index:
                speedup = base_time / group.loc[p]['t_total']
                row.append(f"{speedup:.2f}")
            else:
                row.append('-')
        rows.append(row)

    # Save to file
    os.makedirs(data_dir, exist_ok=True)
    export_path = os.path.join(data_dir, output_filename)
    with open(export_path, 'w') as f:
        f.write('\t'.join(col_headers) + '\n')
        for row in rows:
            f.write('\t'.join(map(str, row)) + '\n')

    print(f"Speedups by increasing p exported at {export_path}")


def export_speedups_by_s(df, data_dir, target_sum, output_filename='speedups_by_s.dat'):
    """
    For each p where p + q == target_sum and depth == 0:
    - Extract row with max s (s = q) as baseline.
    - Compare pairwise comm time and total time to other s values (less than q).
    - Export table:
      p | pairwise_time_max_s | pairwise_speedup_s<q... | total_time_max_s | total_speedup_s<q...
    """
    df_filtered = df[(df['p'] + df['q'] == target_sum) & (df['depth'] == 0)]
    if df_filtered.empty:
        print(f"No matching rows found for p + q = {target_sum}, depth == 0 (speedups by s). Skipping export.")
        return

    grouped = df_filtered.groupby('p')
    rows = []

    # Determine all possible s values less than q (ordered descending)
    all_s_diffs = sorted(
        df_filtered[df_filtered['s'] != df_filtered['q']]['q'] - df_filtered[df_filtered['s'] != df_filtered['q']]['s']
    )
    all_s_diffs = sorted(set(all_s_diffs))  # To remove duplicates and sort
    speedup_headers = [f'pairwise_speedup_s_minus_{d}' for d in all_s_diffs] + \
                      [f'total_speedup_s_minus_{d}' for d in all_s_diffs]
    headers = ['p', 'pairwise_time_max_s'] + speedup_headers[:len(all_s_diffs)] + \
              ['total_time_max_s'] + speedup_headers[len(all_s_diffs):]

    for p_val, group in grouped:
        # Get row with max s = q
        baseline_row = group[group['s'] == group['q']]
        if baseline_row.empty:
            continue  # Skip if no max s baseline

        baseline_row = baseline_row.iloc[0]
        baseline_pairwise = baseline_row['t_comm_pairwise']
        baseline_total = baseline_row['t_total']
        q_val = baseline_row['q']

        row = [p_val, f"{baseline_pairwise:.6f}"]

        # Compute pairwise speedups
        for diff in all_s_diffs:
            s_val = q_val - diff
            alt_row = group[group['s'] == s_val]
            if not alt_row.empty:
                alt_row = alt_row.iloc[0]
                speedup = baseline_pairwise / alt_row['t_comm_pairwise']
                row.append(f"{speedup:.2f}")
            else:
                row.append('-')

        row.append(f"{baseline_total:.6f}")

        # Compute total speedups
        for diff in all_s_diffs:
            s_val = q_val - diff
            alt_row = group[group['s'] == s_val]
            if not alt_row.empty:
                alt_row = alt_row.iloc[0]
                speedup = baseline_total / alt_row['t_total']
                row.append(f"{speedup:.2f}")
            else:
                row.append('-')

        rows.append(row)

    # Save to file
    os.makedirs(data_dir, exist_ok=True)
    export_path = os.path.join(data_dir, output_filename)
    with open(export_path, 'w') as f:
        f.write('\t'.join(headers) + '\n')
        for row in rows:
            f.write('\t'.join(map(str, row)) + '\n')

    print(f"Speedups by decreasing s exported at {export_path}")


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
    plot_pairwise_comm_vs_procs(df, figures_dir, 27)
    plot_total_time_vs_procs_by_depth(df, figures_dir, 27)
    export_speedups_vs_depth(df, data_dir, 27)
    plot_total_time_vs_procs_by_q(df, figures_dir)
    export_speedups_by_p(df, data_dir)
    export_speedups_by_s(df, data_dir, 27)
