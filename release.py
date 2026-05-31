from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

ROOT = Path(__file__).resolve().parent
RELEASE_DIR = ROOT / "_release"
INCLUDE_DIR = ROOT / "include" / "InAPI"
FILES = [
    ROOT / "MIT-LICENSE.txt",
    ROOT / "makefile",
]

def ask_version():
    version = input("Version: ").strip()

    if not version:
        raise SystemExit("Version cannot be empty.")

    if any(symbol in version for symbol in '\\/:*?"<>|'):
        raise SystemExit('Version contains invalid filename characters: \\ / : * ? " < > |')

    return version

def add_file(zip_file: ZipFile, path: Path):
    zip_file.write(path, path.relative_to(ROOT).as_posix())

def add_directory(zip_file: ZipFile, directory: Path):
    for path in sorted(directory.rglob("*")):
        if path.is_file():
            add_file(zip_file, path)

def main():
    if not INCLUDE_DIR.is_dir():
        raise SystemExit(f"Missing directory: {INCLUDE_DIR}")

    for path in FILES:
        if not path.is_file():
            raise SystemExit(f"Missing file: {path}")

    version = ask_version()
    RELEASE_DIR.mkdir(exist_ok=True)

    archive_path = RELEASE_DIR / f"InAPI-{version}.zip"

    with ZipFile(archive_path, "w", ZIP_DEFLATED) as zip_file:
        add_directory(zip_file, INCLUDE_DIR)

        for path in FILES:
            add_file(zip_file, path)

    print(f"Created: {archive_path}")

if __name__ == "__main__":
    main()