import os
import sys
from pathlib import Path

# Force UTF-8 stdout encoding for Windows terminal unicode support! 🏎️
if hasattr(sys.stdout, 'reconfigure'):
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass

# THE ULTRA-FAST NO-DEPENDENCY PROJECT PACKAGER 🏎️
# PACKAGES THE HOLY REPOSITORY INTO A SINGLE PLAIN TEXT FILE FOR ANTISEPTIC STORAGE

VERSION = "1.0.0"
PROJECT_NAME = "Calculator"

def generate_tree(dir_path, prefix=""):
    """Recursively generates a visual tree of the codebase."""
    try:
        entries = sorted(list(dir_path.iterdir()), key=lambda e: (e.is_file(), e.name))
    except Exception:
        return []
        
    tree = []
    
    # Filter out build artifacts, git configurations, vendor dirs, and python temp folders
    # DOGMA: If it's not source code WE WROTE, it doesn't go in the archive. 💀
    ignored = {".git", ".vs", "bin", "obj", "__pycache__", "Output", "DotNetPerf", 
               "build", "bench_runner", "test_runner", ".deepseek", "vcpkg", "packages",
               "node_modules", ".gemini", ".vscode", ".idea"}
    entries = [e for e in entries if e.name not in ignored]

    for i, entry in enumerate(entries):
        connector = "└── " if i == len(entries) - 1 else "├── "
        if entry.is_dir():
            tree.append(f"{prefix}{connector}{entry.name}/")
            extension = "    " if i == len(entries) - 1 else "│   "
            tree.extend(generate_tree(entry, prefix + extension))
        else:
            tree.append(f"{prefix}{connector}{entry.name}")
    return tree

def package_project():
    workspace = Path(__file__).resolve().parent.parent.parent
    script_dir = Path(__file__).resolve().parent
    output_dir = script_dir / "Output"
    output_dir.mkdir(exist_ok=True)
    output_file = output_dir / f"{PROJECT_NAME}_v{VERSION}.txt"

    # INTERACTIVE DASHBOARD WITH GORGEOUS HARDWARE STYLE ASCII
    print("\n" + "=" * 60)
    print(" [SKULL] BARE-METAL CALC ENGINE - ARCHIVE COMPRESSOR [SKULL]")
    print("=" * 60)
    print("Description:")
    print("  Packages the entire unmanaged code repository into a single")
    print("  contiguous text file with structural hierarchy tree mapping.")
    print("\nPayload Specs:")
    print(f"  Target Root: {workspace}")
    print(f"  Export Path: {output_file}")
    print("=" * 60)
    print("  [ENTER]  -> Fire compiler extraction payload")
    print("  [CTRL+C] -> Terminate operations and exit")
    print("=" * 60 + "\n")

    try:
        input("Press ENTER to execute pipeline... ")
    except KeyboardInterrupt:
        print("\n\n[ABORTED] Packaging operations cancelled cleanly. Exiting. [SKULL]\n")
        return

    print("\nInitiating system parsing...")

    ignored_dirs = {".git", ".vs", "bin", "obj", "__pycache__", "Output", "DotNetPerf", 
                    "build", ".deepseek", "vcpkg", "packages", "node_modules", 
                    ".gemini", ".vscode", ".idea"}
    ignored_exts = {".exe", ".bin", ".iso", ".o", ".pyc", ".png", ".jpg", ".zip", ".pdb", ".txt"}

    lines = []
    lines.append("=" * 80)
    lines.append(f"[BOX] PROJECT SUBMISSION: {PROJECT_NAME} v{VERSION}")
    lines.append(f"Generated on: {os.name} platform")
    lines.append("=" * 80)
    lines.append("\n[DIR] TREE STRUCTURE:")
    lines.extend(generate_tree(workspace))
    lines.append("\n" + "=" * 80 + "\n")

    for root, dirs, files in os.walk(workspace):
        # Skip ignored directories
        dirs[:] = [d for d in dirs if d not in ignored_dirs]
        
        for file in sorted(files):
            file_path = Path(root) / file
            if file_path.suffix.lower() in ignored_exts or file == Path(__file__).name:
                continue

            rel_path = file_path.relative_to(workspace)
            lines.append(f"[FILE] FILE: {rel_path}")
            lines.append("-" * 80)
            
            try:
                with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
                lines.append(content)
            except Exception as e:
                lines.append(f"[ERROR READING FILE: {e}]")
            
            lines.append("\n" + "=" * 80 + "\n")

    with open(output_file, "w", encoding="utf-8") as out:
        out.write("\n".join(lines))
        
    print(f"Pack successfully created!")
    print(f"Absolute Export Path: {output_file.resolve()}")
    print(f"Total size: {output_file.stat().st_size} bytes. [SUCCESS]\n")
    
    try:
        input("Press ENTER to exit... ")
    except KeyboardInterrupt:
        pass

if __name__ == "__main__":
    package_project()

