import re
import matplotlib.pyplot as plt

logfile = "release_times.log"
pattern = re.compile(r"RELEASE2: Service (\d+) at (\d+)\.(\d+)")

service_times = {}

with open(logfile, 'r') as f:
    for line in f:
        match = pattern.search(line)
        if match:
            sid = int(match.group(1))
            sec = int(match.group(2))
            nsec = int(match.group(3))
            time_ms = sec * 1000 + nsec / 1e6
            service_times.setdefault(sid, []).append(time_ms)

# Plotting the SVG diagram
fig, ax = plt.subplots()
colors = ['tab:red', 'tab:blue', 'tab:green', 'tab:orange', 'tab:purple']

for idx, (sid, times) in enumerate(service_times.items()):
    ax.vlines(times, idx + 0.6, idx + 1.4, colors=colors[idx % len(colors)], label=f'Service {sid}')

ax.set_yticks(range(1, len(service_times)+1))
ax.set_yticklabels([f"Service {sid}" for sid in service_times])
ax.set_xlabel("Time (ms)")
ax.set_title("Service Release Timing Diagram")
ax.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("service_timing.svg")
