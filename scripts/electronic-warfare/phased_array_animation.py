"""
Phased-array beam-steering animation in the EW dark style.

Animates an N-element linear array whose commanded steer angle sweeps from
-60° to +60°. Each frame renders the dark-theme "instantaneous field" pcolormesh
on the left and the polar beam pattern on the right, both taken from the
EW_scenarios notebook palette.

Outputs
-------
    doc/phased_array.gif  (centered half-width GIF for the README)
"""

from __future__ import annotations

import os
from pathlib import Path

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.animation import FuncAnimation, PillowWriter


REPO_ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = REPO_ROOT / "doc"
OUT_DIR.mkdir(exist_ok=True)

# --- dark EW palette (mirrors EW_scenarios cell-1) -----------------------
BG, PANEL, FG = "#05070b", "#0a0d14", "#c9cfda"
MUTED, GRIDC = "#6b7280", "#1b2130"
BLUE, ORANGE, CYAN, RED, GREEN = "#5aa9e6", "#e08a3c", "#3fd0c9", "#e0555c", "#7ddc7d"

FIELD = LinearSegmentedColormap.from_list("ew_field", [
    (0.00, "#eaf3ff"), (0.16, BLUE), (0.44, "#0a1622"), (0.50, BG),
    (0.56, "#211307"), (0.84, ORANGE), (1.00, "#fff3e2")])

plt.rcParams.update({
    "figure.dpi": 110, "font.size": 8.5, "axes.titlesize": 9,
    "figure.facecolor": BG, "savefig.facecolor": BG,
    "axes.facecolor": PANEL, "axes.edgecolor": GRIDC, "axes.labelcolor": FG,
    "text.color": FG, "xtick.color": MUTED, "ytick.color": MUTED,
    "grid.color": GRIDC, "axes.grid": True, "grid.alpha": 0.25,
    "axes.spines.top": False, "axes.spines.right": False,
    "legend.facecolor": PANEL, "legend.edgecolor": GRIDC, "legend.framealpha": 0.9,
})

# --- array / animation parameters ---------------------------------------
N_ELEM = 16
D_OVER_LAMBDA = 0.5
TAU_STEP = 24          # snapshot phase index, near peak
FRAMES = 30            # GIF frames
INTERVAL_MS = 100      # GIF tick
FLOOR_DB = -40.0
STEER_RANGE_DEG = 60.0  # sweep ±this angle

NX, NZ = 220, 170
X_SPAN = 9.0
Z_MAX = 13.0


def array_near(N, d, th0, tau, nx=NX, nz=NZ, span=X_SPAN, zmax=Z_MAX):
    """Snapshot of the instantaneous field from a tapered linear array."""
    n = np.arange(N) - (N - 1) / 2
    xe = n * d
    a = np.ones(N)                       # uniform taper
    beta = -2 * np.pi * d * n * np.sin(th0)
    X, Z = np.meshgrid(np.linspace(-span, span, nx),
                       np.linspace(0.05, zmax, nz))
    E = np.zeros_like(X)
    for i in range(N):
        R = np.hypot(X - xe[i], Z)
        E += a[i] * np.cos(2 * np.pi * (tau - R) + beta[i]) / np.sqrt(np.maximum(R, 0.25))
    return X, Z, E / np.max(np.abs(E)), xe


def array_far(N, d, th0):
    """Far-field array factor magnitude, isotropic elements."""
    n = np.arange(N) - (N - 1) / 2
    th = np.linspace(-np.pi / 2, np.pi / 2, 721)
    AF = np.exp(2j * np.pi * d * np.outer(np.sin(th) - np.sin(th0), n)) @ np.ones(N)
    return th, np.abs(AF)


def db(x, floor=FLOOR_DB):
    x = np.abs(np.asarray(x, float))
    m = x.max()
    if not np.isfinite(m) or m <= 0:
        return np.full(x.shape, floor)
    return np.clip(20 * np.log10(np.maximum(x, 1e-300) / m), floor, 0.0)


def make_figure():
    fig = plt.figure(figsize=(6.6, 2.45))    # half-width GIF
    gs = fig.add_gridspec(1, 2, width_ratios=[1.32, 1.0], wspace=0.22,
                          left=0.045, right=0.985, top=0.86, bottom=0.14)
    return fig, gs


def style_axes(ax, edge=GRIDC, lw=0.8):
    ax.set_facecolor(PANEL)
    for s in ax.spines.values():
        s.set_visible(True)
        s.set_color(edge)
        s.set_linewidth(lw)
    ax.tick_params(colors=MUTED, labelsize=6.5)


def main() -> None:
    fig, gs = make_figure()

    a0 = fig.add_subplot(gs[0])
    style_axes(a0, BLUE)
    pcm = a0.imshow(np.zeros((2, 2)), cmap=FIELD, vmin=-0.55, vmax=0.55,
                    aspect="auto", interpolation="bilinear", origin="lower",
                    extent=[-X_SPAN, X_SPAN, 0.05, Z_MAX])
    elem_dots, = a0.plot([], [], "s", ms=3.6, color="#f2f5fa", mec="none")
    steer_line, = a0.plot([], [], "-", color=CYAN, lw=1.2)
    a0.set_xlim(-X_SPAN, X_SPAN); a0.set_ylim(0, Z_MAX)
    a0.set_xlabel("cross-range  (wavelengths)", fontsize=7)
    a0.set_ylabel("range  (wavelengths)", fontsize=7)
    a0.grid(False)
    a0.set_title("instantaneous field — wavefronts add where the phase aligns",
                 fontsize=8, color=FG, pad=4)

    a1 = fig.add_subplot(gs[1], projection="polar")
    a1.set_facecolor(PANEL)
    a1.set_theta_zero_location("N"); a1.set_theta_direction(-1)
    a1.set_thetamin(-90); a1.set_thetamax(90)
    a1.set_ylim(FLOOR_DB, 0); a1.set_rlabel_position(268)
    a1.tick_params(colors=MUTED, labelsize=6.0)
    a1.grid(alpha=0.2, color=GRIDC)
    a1.set_title("beam pattern  (normalised, dB)", fontsize=8, color=FG, pad=12)
    beam_line, = a1.plot([], [], color=ORANGE, lw=1.4)
    beam_fill = None
    steer_mark, = a1.plot([], [], color="#f2f5fa", lw=1.0, ls="--")

    th_static, F_static = array_far(N_ELEM, D_OVER_LAMBDA, 0.0)

    def steer_for(frame: int) -> float:
        return -STEER_RANGE_DEG + (2 * STEER_RANGE_DEG) * frame / (FRAMES - 1)

    def update(frame: int):
        nonlocal beam_fill
        th0_deg = steer_for(frame)
        th0 = np.deg2rad(th0_deg)
        tau = TAU_STEP / 24.0

        X, Z, E, xe = array_near(N_ELEM, D_OVER_LAMBDA, th0, tau)
        pcm.set_data(E)
        pcm.set_extent([X.min(), X.max(), Z.min(), Z.max()])
        bx, by = [0, 13 * np.sin(th0)], [0, 13 * np.cos(th0)]
        steer_line.set_data(bx, by)
        elem_dots.set_data(xe, np.zeros_like(xe))

        th, F = array_far(N_ELEM, D_OVER_LAMBDA, th0)
        G = db(F)
        beam_line.set_data(th, G)
        if beam_fill is not None:
            beam_fill.remove()
        beam_fill = a1.fill_between(th, FLOOR_DB, G, color=ORANGE, alpha=0.18)
        steer_mark.set_data([th0, th0], [FLOOR_DB, 0])

        fig.suptitle(
            f"phased array — N={N_ELEM}   d/λ={D_OVER_LAMBDA}   "
            f"steer θ₀ = {th0_deg:+.1f}°",
            color=FG, fontsize=9, y=0.99)
        return pcm, beam_line, steer_line, steer_mark

    anim = FuncAnimation(fig, update, frames=FRAMES, interval=INTERVAL_MS,
                         blit=False)
    out = OUT_DIR / "phased_array.gif"
    anim.save(out, writer=PillowWriter(fps=12))
    plt.close(fig)
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
