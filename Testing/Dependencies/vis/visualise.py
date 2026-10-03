from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import matplotlib
import argparse
import json

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


if __name__ == "__main__":
    # parse args:
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=Path)
    args = parser.parse_args()

    metadata = load_data(from_folder=args.input, filename='metadata')
    func = metadata['type']
    __functions__[func](args.input)

    plt.show()
