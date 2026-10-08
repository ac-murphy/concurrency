import pathlib
from pathlib import Path
from matplotlib.animation import FuncAnimation

import matplotlib
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import argparse
import json
import sys

matplotlib.use("QtAgg")

__frame__ = 0
__functions__ = {}
def register(func):
    __functions__[func.__name__] = func
    return func


def load_data(*, from_folder: Path, filename: str, n_cols: int = 0, dtype: np.dtype = None) -> np.ndarray | None:
    matches = list(from_folder.glob(filename + '.*'))
    if len(matches) > 1:
        print(f'Too many occurrences of "{filename}"')
        return None
    if len(matches) == 0:
        print(f'Folder missing "{filename}"')
        return None

    file_path = matches[0]
    match file_path.suffix:
        case '.csv':
            data = pd.read_csv(file_path, sep=',', usecols=[i for i in range(n_cols)])
            return data.to_numpy(dtype=dtype)
        case '.bin':
            data = np.fromfile(file_path, dtype=dtype).reshape(-1, n_cols)
            if n_cols == 1:
                data = [x[0] for x in data]
            return data
        case '.json':
            with open(from_folder/f'{filename}.json') as f:
                return json.load(f)
        case _:
            print(f'Unsupported extension "{file_path}"')
            return None


@register
def graph_1d(input_path: Path):
    x_values = load_data(from_folder=input_path, filename='x_values', n_cols=1, dtype=np.float32)
    y_values = load_data(from_folder=input_path, filename='y_values', n_cols=1, dtype=np.float32)

    plt.plot(x_values, y_values)


@register
def graph_1d_anim(input_path: Path):
    t_values = np.array(load_data(from_folder=input_path, filename='t_values', n_cols=1,             dtype=np.float32))
    x_values = np.array(load_data(from_folder=input_path, filename='x_values', n_cols=1,             dtype=np.float32))
    u_values =          load_data(from_folder=input_path, filename='u_values', n_cols=len(x_values), dtype=np.float32)

    T = len(u_values)
    X = len(u_values)
    print(t_values.shape, x_values.shape, u_values.shape)

    u_min = np.min(u_values)
    u_max = np.max(u_values)

    fig, ax = plt.subplots()
    line, = ax.plot(x_values, u_values[0, :])
    ax.set_ylim(u_min, u_max)

    def update(n):
        line.set_ydata(u_values[n, :])
        ax.set_title(f'n={n}')
        fig.canvas.draw_idle()

    def on_key(event):
        global __frame__

        if event.key == 'right':
            __frame__ = min(__frame__ + 1, T - 1)
            update(__frame__)
        elif event.key == 'left':
            __frame__ = max(__frame__ - 1, 0)
            update(__frame__)

    # anim = FuncAnimation(
    #     fig=fig,
    #     func=update,
    #     frames=T,
    #     interval=20,
    #     blit=True
    # )

    fig.canvas.mpl_connect('key_press_event', on_key)

    plt.show()


if __name__ == "__main__":
    # parse args:
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=Path)
    args = parser.parse_args()

    for i in range(0, len(list(args.input.iterdir()))):
        path: Path = args.input/str(i)

        if path.exists():
            metadata = load_data(from_folder=path, filename='metadata')
            func = metadata['type']
            __functions__[func](path)

    plt.show()



