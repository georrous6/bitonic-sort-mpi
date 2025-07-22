import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

def read_timing_log(filepath):
    columns = ['p', 'q', 's', 't_initial_sort', 't_comm_pairwise', 't_elbow_sort', 't_total']
    try:
        df = pd.read_csv(filepath, sep=r'\s+', names=columns, engine='python')
        return df
    except Exception as e:
        print(f"Error reading log file: {e}")
        return None

if __name__ == "__main__":
    log_file = 'logs/bitonic_sort.log'
    save_path = '../docs/figures/bitonic_sort_timing.png'

    df = read_timing_log(log_file)

    if df is not None:
        df['proc_count'] = 2 ** df['p']
        df['array_size'] = 2 ** (df['p'] + df['q'])

        labels = [f'{2 ** p}' for p in df['p']]
        x = np.arange(len(df))

        initial = df['t_initial_sort']
        pairwise = df['t_comm_pairwise']
        elbow = df['t_elbow_sort']
        other = df['t_total'] - (initial + pairwise + elbow)

        plt.figure(figsize=(9, 6))
        plt.bar(x, initial, label='Initial Sort', color='#4CAF50')
        plt.bar(x, pairwise, bottom=initial, label='Pairwise Comm', color='#2196F3')
        plt.bar(x, elbow, bottom=initial + pairwise, label='Elbow Sort', color='#FFC107')
        plt.bar(x, other, bottom=initial + pairwise + elbow, label='Other', color='#9E9E9E')

        plt.xticks(x, labels)
        plt.xlabel('Number of Processes')
        plt.ylabel('Execution Time (seconds)')
        title_str = f'Total Array Size = 2^{int(df.iloc[0]["p"] + df.iloc[0]["q"])} elements'
        plt.title(title_str)
        plt.legend()
        plt.grid(axis='y', linestyle='--', alpha=0.7)
        plt.tight_layout()

        # Create directory if it doesn't exist
        os.makedirs(os.path.dirname(save_path), exist_ok=True)
        plt.savefig(save_path)
        print(f"Plot saved at {save_path}")

