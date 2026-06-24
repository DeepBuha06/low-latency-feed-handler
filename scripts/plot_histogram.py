import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
import os

if not os.path.exists('latency_telemetry.csv'):
    print("Error: latency_telemetry.csv not found.")
    sys.exit(1)

df = pd.read_csv('latency_telemetry.csv')

# Drop initial warmup (first 10,000 packets)
if len(df) > 10000:
    df = df.iloc[10000:]

fig, axes = plt.subplots(1, 2, figsize=(16, 6))
fig.suptitle('HFT Feed Handler — Latency Profiles (Log-Log Scale)', fontsize=16, fontweight='bold', color='#2c3e50')

# Function to plot a log-log histogram cleanly
def plot_log_hist(ax, data, color, title):
    # Ensure no zeros for log space
    data = data[data > 0]
    if data.empty:
        return
    
    min_val = data.min()
    max_val = data.max()
    
    # Create logarithmically spaced bins
    bins = np.logspace(np.log10(min_val), np.log10(max_val), 100)
    
    ax.hist(data, bins=bins, color=color, alpha=0.9, edgecolor='black', linewidth=0.5)
    
    # Lines for p50, p99, p99.9
    p50 = data.median()
    p99 = data.quantile(0.99)
    p999 = data.quantile(0.999)
    
    ax.axvline(p50, color='#2c3e50', linestyle='-', linewidth=2, label=f'p50 = {p50:.0f} ns')
    ax.axvline(p99, color='#e67e22', linestyle='--', linewidth=2, label=f'p99 = {p99:.0f} ns')
    ax.axvline(p999, color='#c0392b', linestyle=':', linewidth=2, label=f'p99.9 = {p999:.0f} ns')
    
    ax.set_xscale('log')
    ax.set_yscale('log')
    
    ax.set_title(title, fontsize=14, pad=10)
    ax.set_xlabel('Latency in nanoseconds (Log Scale)', fontsize=12)
    ax.set_ylabel('Packet Count (Log Scale)', fontsize=12)
    ax.legend(fontsize=11)
    ax.grid(True, which="both", ls="-", alpha=0.2)
    
    # Format X ticks better
    from matplotlib.ticker import FuncFormatter
    ax.xaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:g}'.format(y)))

plot_log_hist(axes[0], df['BookUpdate_ns'], '#3498db', 'Order Book Update Latency (t4 − t3)')
plot_log_hist(axes[1], df['WireToBook_ns'], '#2ecc71', 'Wire-to-Book Latency (t4 − t1)')

plt.tight_layout()
plt.savefig('assets/latency_histogram.png', dpi=200, bbox_inches='tight')
print("Saved assets/latency_histogram.png")
