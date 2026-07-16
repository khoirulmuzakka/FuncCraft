import os

import matplotlib.pyplot as plt
import numpy as np


def main():
    x = np.linspace(-100.0, 100.0, 181)
    y = np.linspace(-100.0, 100.0, 181)
    xx, yy = np.meshgrid(x, y)

    theta = np.deg2rad(32.0)
    c, s = np.cos(theta), np.sin(theta)
    x0, y0 = 25.0, -20.0
    xr = c * (xx - x0) - s * (yy - y0)
    yr = s * (xx - x0) + c * (yy - y0)
    affine = (xr / 28.0) ** 2 + (yr / 70.0) ** 2

    a = 45.0
    s1 = 2600.0
    s2 = 45.0
    folded = ((xx**2 - a**2) / s1) ** 2 + (yy / s2) ** 2

    panels = [
        ("Affine transformation", affine, [(x0, y0)]),
        ("Nonlinear folded transformation", folded, [(-a, 0.0), (a, 0.0)]),
    ]
    shown_panels = [(title, np.log1p(values), points) for title, values, points in panels]
    global_max = max(np.percentile(values, 99.0) for _, values, _ in shown_panels)

    fig = plt.figure(figsize=(10.5, 4.8), constrained_layout=True)
    for idx, (title, zz, points) in enumerate(shown_panels, start=1):
        ax = fig.add_subplot(1, 2, idx, projection="3d")
        ax.plot_surface(
            xx,
            yy,
            zz,
            cmap="viridis",
            linewidth=0,
            antialiased=True,
            rcount=100,
            ccount=100,
            vmin=0.0,
            vmax=global_max,
        )
        ax.contour(xx, yy, zz, zdir="z", offset=0.0, levels=12, cmap="viridis", linewidths=0.45)
        px = [p[0] for p in points]
        py = [p[1] for p in points]
        pz = [0.0 for _ in points]
        ax.scatter(px, py, pz, marker="*", color="white", edgecolor="black", s=90)
        ax.set_title(title, fontsize=10)
        ax.set_xlabel(r"$x_1$", fontsize=8)
        ax.set_ylabel(r"$x_2$", fontsize=8)
        ax.set_zlabel(r"$\log(1+f)$", fontsize=8)
        ax.set_xlim(-100.0, 100.0)
        ax.set_ylim(-100.0, 100.0)
        ax.set_zlim(0.0, global_max)
        ax.view_init(elev=28, azim=-135)
        ax.tick_params(labelsize=7)

    out_dir = os.path.join(os.path.dirname(__file__), "..", "figures")
    os.makedirs(out_dir, exist_ok=True)
    pdf_path = os.path.join(out_dir, "coordinate_transform_sphere_3d.pdf")
    png_path = os.path.join(out_dir, "coordinate_transform_sphere_3d.png")
    fig.savefig(pdf_path)
    fig.savefig(png_path, dpi=220)


if __name__ == "__main__":
    main()
