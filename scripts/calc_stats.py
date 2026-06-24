import pandas as pd

df = pd.read_csv('/mnt/c/low-latency-feed-handler/latency_telemetry.csv')

metrics = ['BookUpdate_ns', 'WireToBook_ns']
percentiles = [0.90, 0.95, 0.99, 0.999, 0.9999]

print("Latency Statistics (ns)\n")
for metric in metrics:
    print(f"--- {metric} ---")
    print(f"Min:    {df[metric].min():.0f}")
    print(f"Max:    {df[metric].max():.0f}")
    print(f"Mean:   {df[metric].mean():.0f}")
    print(f"Median: {df[metric].median():.0f}")
    
    quantiles = df[metric].quantile(percentiles)
    print(f"p90:    {quantiles[0.90]:.0f}")
    print(f"p95:    {quantiles[0.95]:.0f}")
    print(f"p99:    {quantiles[0.99]:.0f}")
    print(f"p99.9:  {quantiles[0.999]:.0f}")
    print(f"p99.99: {quantiles[0.9999]:.0f}")
    print()
