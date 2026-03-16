# Calibration anchors: add as many (letter, pwm) pairs as you have measured.
# The script interpolates linearly between them.
ANCHORS = [
    ('A', 2170),
    ('G', 1755),
    ('K', 1505),
    ('N', 1265),
    ('S', 910),
    ('W', 675),
    ('Z', 530),
]

# ---------------------------------------------------------------------------

def interpolate(anchors: list[tuple[str, int]]) -> dict[str, int]:
    # Sort by letter index
    points = sorted((ord(l) - ord('A'), pwm) for l, pwm in anchors)

    result = {}
    for i in range(26):
        # Find surrounding anchors
        lo = next((p for p in reversed(points) if p[0] <= i), points[0])
        hi = next((p for p in points if p[0] >= i), points[-1])

        if lo[0] == hi[0]:
            pwm = lo[1]
        else:
            t = (i - lo[0]) / (hi[0] - lo[0])
            pwm = lo[1] + t * (hi[1] - lo[1])

        result[chr(ord('A') + i)] = round(pwm)

    return result

values = interpolate(ANCHORS)

# Print as vocabulary.h entries ready to paste
print("// Anchors:", ", ".join(f"{l}={p}" for l, p in sorted(ANCHORS)))
print()
for letter, pwm in values.items():
    anchor_marker = " // <-- anchor" if any(l == letter for l, _ in ANCHORS) else ""
    print(f'    {{"{letter}", {pwm:5d}}},{anchor_marker}')
